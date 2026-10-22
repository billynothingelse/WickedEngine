#include "rhi/metal/metal_internal.h"
// Metal backend: GPU resource and pipeline creation

#ifdef __APPLE__

namespace Protean::Graphics
{
	using namespace MetalInternal;

	bool MetalDevice::CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = Protean::Allocator::make_shared<Buffer_Metal>();
		internal_state->allocationhandler = allocationhandler;
		buffer->internal_state = internal_state;
		buffer->type = GPUResource::Type_BUFFER;
		buffer->mapped_data = nullptr;
		buffer->mapped_size = 0;
		buffer->desc = *desc;
		
		const bool sparse = has_flag(desc->misc_flags, ResourceMiscFlag_SPARSE);
		const bool aliasing_storage = has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_BUFFER) || has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS) || has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS);
		
		MTL::ResourceOptions resource_options = {};
		if (desc->usage == Usage_DEFAULT)
		{
			if (has_flag(desc->misc_flags, ResourceMiscFlag_VIDEO_DECODE))
			{
				// VideoToolbox reads the compressed bitstream on the CPU.
				resource_options |= MTL::ResourceStorageModeShared;
			}
			else if (aliasing_storage)
			{
				// potentially used for sparse or other private requirement resource
				resource_options |= MTL::ResourceStorageModePrivate;
			}
			else if (CheckCapability(GraphicsDeviceCapability_CACHE_COHERENT_UMA))
			{
				resource_options |= MTL::ResourceStorageModeShared;
			}
			else
			{
				resource_options |= MTL::ResourceStorageModePrivate;
			}
		}
		else if (desc->usage == Usage_UPLOAD)
		{
			resource_options |= MTL::ResourceStorageModeShared;
			resource_options |= MTL::ResourceCPUCacheModeWriteCombined;
		}
		else if (desc->usage == Usage_READBACK)
		{
			resource_options |= MTL::ResourceStorageModeShared;
		}
		
		if (aliasing_storage)
		{
			// This is an aliasing storage:
			MTL::SizeAndAlign sizealign = device->heapBufferSizeAndAlign(desc->size, resource_options);
			NS::SharedPtr<MTL::HeapDescriptor> heap_desc = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
			heap_desc->setResourceOptions(resource_options);
			heap_desc->setSize(sizealign.size);
			heap_desc->setType(MTL::HeapTypePlacement);
			heap_desc->setMaxCompatiblePlacementSparsePageSize(sparse_page_size);
			NS::SharedPtr<MTL::Heap> heap = NS::TransferPtr(device->newHeap(heap_desc.get()));
			internal_state->buffer = NS::TransferPtr(heap->newBuffer(desc->size, resource_options, 0));
			internal_state->buffer->makeAliasable();
		}
		else if (alias != nullptr)
		{
			// This is an aliasing view:
			if (alias->IsBuffer())
			{
				auto alias_internal = to_internal<GPUBuffer>(alias);
				internal_state->buffer = NS::TransferPtr(alias_internal->buffer->heap()->newBuffer(desc->size, resource_options, alias_internal->buffer->heapOffset() + alias_offset));
			}
			else if (alias->IsTexture())
			{
				auto alias_internal = to_internal<Texture>(alias);
				internal_state->buffer = NS::TransferPtr(alias_internal->texture->heap()->newBuffer(desc->size, resource_options, alias_internal->texture->heapOffset() + alias_offset));
			}
		}
		else if (sparse)
		{
			// This is a placement sparse buffer:
			internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->size, resource_options, sparse_page_size));
		}
		else
		{
			// This is a standalone buffer:
			internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->size, resource_options));
		}
		
		allocationhandler->make_resident(internal_state->buffer.get());
		internal_state->gpu_address = internal_state->buffer->gpuAddress();
		if ((resource_options & MTL::ResourceStorageModePrivate) == 0)
		{
			buffer->mapped_data = internal_state->buffer->contents();
			buffer->mapped_size = internal_state->buffer->allocatedSize();
		}
		
		if (init_callback)
		{
			if (buffer->mapped_data == nullptr)
			{
				NS::SharedPtr<NS::AutoreleasePool> autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
				CopyAllocator::CopyCMD cmd = copyAllocator.allocate(desc->size);
				init_callback(cmd.mapped_data);
				cmd.encoder->copyFromBuffer(cmd.uploadbuffer.get(), 0, internal_state->buffer.get(), 0, desc->size);
				copyAllocator.submit(cmd);
			}
			else
			{
				init_callback(buffer->mapped_data);
			}
		}
		
		if (!has_flag(desc->misc_flags, ResourceMiscFlag_NO_DEFAULT_DESCRIPTORS))
		{
			if (has_flag(desc->bind_flags, BindFlag_SHADER_RESOURCE))
			{
				CreateSubresource(buffer, SubresourceType_SRV, 0);
			}
			if (has_flag(desc->bind_flags, BindFlag_UNORDERED_ACCESS))
			{
				CreateSubresource(buffer, SubresourceType_UAV, 0);
			}
		}
		
		return internal_state->buffer.get() != nullptr;
	}

	bool MetalDevice::CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = Protean::Allocator::make_shared<Texture_Metal>();
		internal_state->allocationhandler = allocationhandler;
		texture->internal_state = internal_state;
		texture->type = GPUResource::Type_TEXTURE;
		texture->mapped_data = nullptr;
		texture->mapped_size = 0;
		texture->mapped_subresources = nullptr;
		texture->mapped_subresource_count = 0;
		texture->sparse_properties = nullptr;
		texture->desc = *desc;
		
		wilog_assert(!IsFormatBlockCompressed(desc->format) || device->supportsBCTextureCompression(), "Block compressed textures are not supported by this device!");
		
		const bool sparse = has_flag(desc->misc_flags, ResourceMiscFlag_SPARSE);
		const bool aliasing_storage = has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_BUFFER) || has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS) || has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS);

		texture->desc.mip_levels = GetMipCount(texture->desc);
		
		NS::SharedPtr<MTL::TextureDescriptor> descriptor = _ConvertTextureDesc(texture->desc, device.get(), textureUlongAtomics);
		const bool nv12 = texture->desc.format == Format_NV12;
		if (nv12)
		{
			// Metal has no multi-planar NV12 format. Luma is R8 and chroma is a paired RG8 texture.
			descriptor->setPixelFormat(MTL::PixelFormatR8Unorm);
		}
		
		texture->desc.sample_count = (uint32_t)descriptor->sampleCount(); // might be overridden by device descriptor
		
		if (sparse)
		{
			descriptor->setPlacementSparsePageSize(sparse_page_size);
			texture->sparse_page_size = (uint32_t)device->sparseTileSizeInBytes(sparse_page_size);
			texture->sparse_properties = &internal_state->sparse_properties;
			const MTL::Size sparse_size = device->sparseTileSize(descriptor->textureType(), descriptor->pixelFormat(), descriptor->sampleCount(), sparse_page_size);
			internal_state->sparse_properties.tile_width = (uint32_t)sparse_size.width;
			internal_state->sparse_properties.tile_height = (uint32_t)sparse_size.height;
			internal_state->sparse_properties.tile_depth = (uint32_t)sparse_size.depth;
			const MTL::SizeAndAlign sizealign = device->heapTextureSizeAndAlign(descriptor.get());
			internal_state->sparse_properties.total_tile_count = uint32_t(sizealign.size / (uint64_t)texture->sparse_page_size);
		}
		
		if (!has_flag(desc->bind_flags, BindFlag_UNORDERED_ACCESS) && !has_flag(desc->bind_flags, BindFlag_RENDER_TARGET))
		{
			MTL::TextureSwizzleChannels swizzle = MTL::TextureSwizzleChannels::Default();
			swizzle.red = _ConvertComponentSwizzle(desc->swizzle.r);
			swizzle.green = _ConvertComponentSwizzle(desc->swizzle.g);
			swizzle.blue = _ConvertComponentSwizzle(desc->swizzle.b);
			swizzle.alpha = _ConvertComponentSwizzle(desc->swizzle.a);
			descriptor->setSwizzle(swizzle);
		}
		
		if (aliasing_storage)
		{
			// This is an aliasing storage:
			MTL::SizeAndAlign sizealign = device->heapTextureSizeAndAlign(descriptor.get());
			NS::SharedPtr<MTL::HeapDescriptor> heap_desc = NS::TransferPtr(MTL::HeapDescriptor::alloc()->init());
			heap_desc->setResourceOptions(descriptor->resourceOptions());
			heap_desc->setSize(sizealign.size);
			heap_desc->setType(MTL::HeapTypePlacement);
			heap_desc->setMaxCompatiblePlacementSparsePageSize(sparse_page_size);
			NS::SharedPtr<MTL::Heap> heap = NS::TransferPtr(device->newHeap(heap_desc.get()));
			internal_state->texture = NS::TransferPtr(heap->newTexture(descriptor.get(), 0));
			internal_state->texture->makeAliasable();
		}
		else if (alias != nullptr)
		{
			// This is an aliasing view:
			if (alias->IsBuffer())
			{
				auto alias_internal = to_internal<GPUBuffer>(alias);
				internal_state->texture = NS::TransferPtr(alias_internal->buffer->heap()->newTexture(descriptor.get(), alias_internal->buffer->heapOffset() + alias_offset));
			}
			else if (alias->IsTexture())
			{
				auto alias_internal = to_internal<Texture>(alias);
				internal_state->texture = NS::TransferPtr(alias_internal->texture->heap()->newTexture(descriptor.get(), alias_internal->texture->heapOffset() + alias_offset));
			}
		}
		else
		{
			if (desc->usage == Usage_READBACK || desc->usage == Usage_UPLOAD)
			{
				// Note: we are creating a buffer instead of linear image because linear image cannot have mips
				//	With a buffer, we can tightly pack mips linearly into a buffer so it won't have that limitation
				const size_t buffersize = ComputeTextureMemorySizeInBytes(*desc);
				internal_state->buffer = NS::TransferPtr(device->newBuffer(buffersize, descriptor->resourceOptions()));
				allocationhandler->make_resident(internal_state->buffer.get());
				
				texture->mapped_data = internal_state->buffer->contents();
				texture->mapped_size = buffersize;
				
				CreateTextureSubresourceDatas(*desc, texture->mapped_data, internal_state->mapped_subresources);
				texture->mapped_subresources = internal_state->mapped_subresources.data();
				texture->mapped_subresource_count = internal_state->mapped_subresources.size();
				assert(texture->mapped_subresources != nullptr);
			}
			else
			{
				internal_state->texture = NS::TransferPtr(device->newTexture(descriptor.get()));
			}
		}
		
		if (internal_state->texture.get() != nullptr)
		{
			allocationhandler->make_resident(internal_state->texture.get());
		}
		if (nv12 && internal_state->texture.get() != nullptr)
		{
			NS::SharedPtr<MTL::TextureDescriptor> chroma_desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
			chroma_desc->setTextureType(descriptor->textureType());
			chroma_desc->setWidth(std::max(1u, texture->desc.width / 2));
			chroma_desc->setHeight(std::max(1u, texture->desc.height / 2));
			chroma_desc->setDepth(1);
			chroma_desc->setArrayLength(descriptor->arrayLength());
			chroma_desc->setMipmapLevelCount(1);
			chroma_desc->setPixelFormat(MTL::PixelFormatRG8Unorm);
			chroma_desc->setSampleCount(1);
			chroma_desc->setResourceOptions(descriptor->resourceOptions());
			chroma_desc->setStorageMode(descriptor->storageMode());
			chroma_desc->setUsage(descriptor->usage() | MTL::TextureUsageShaderRead);
			internal_state->texture_chroma = NS::TransferPtr(device->newTexture(chroma_desc.get()));
			allocationhandler->make_resident(internal_state->texture_chroma.get());
			if (internal_state->texture_chroma.get() == nullptr)
				return false;
		}
		
		assert(internal_state->texture->isSparse() == sparse);
		
		if (initial_data != nullptr)
		{
			NS::SharedPtr<NS::AutoreleasePool> autorelease_pool; // scoped drain!
			CopyAllocator::CopyCMD cmd;
			uint8_t* upload_data = nullptr;
			if (internal_state->buffer.get() != nullptr)
			{
				// readback or upload, linear memory:
				upload_data = (uint8_t*)internal_state->buffer->contents();
			}
			else if (descriptor->storageMode() == MTL::StorageModePrivate)
			{
				autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
				cmd = copyAllocator.allocate(internal_state->texture->allocatedSize());
				upload_data = cmd.mapped_data;
			}
			
			const uint32_t data_stride = GetFormatStride(desc->format);
			uint32_t initDataIdx = 0;
			uint64_t src_offset = 0;
			for (uint32_t slice = 0; slice < desc->array_size; ++slice)
			{
				uint32_t width = desc->width;
				uint32_t height = desc->height;
				uint32_t depth = desc->depth;
				for (uint32_t mip = 0; mip < texture->desc.mip_levels; ++mip)
				{
					const SubresourceData& subresourceData = initial_data[initDataIdx++];
					const uint32_t block_size = GetFormatBlockSize(desc->format);
					const uint32_t num_blocks_x = std::max(1u, width / block_size);
					const uint32_t num_blocks_y = std::max(1u, height / block_size);
					const uint64_t datasize = data_stride * num_blocks_x * num_blocks_y * depth;
					MTL::Region region = {};
					region.size.width = width;
					region.size.height = height;
					region.size.depth = depth;
					if (internal_state->buffer.get() != nullptr)
					{
						// readback or upload, linear memory:
						std::memcpy(upload_data + src_offset, subresourceData.data_ptr, datasize);
					}
					else if (descriptor->storageMode() == MTL::StorageModePrivate)
					{
						std::memcpy(upload_data + src_offset, subresourceData.data_ptr, datasize);
						MTL::Origin origin = {};
						MTL::Size size = {};
						size.width = width;
						size.height = height;
						size.depth = depth;
						cmd.encoder->copyFromBuffer(cmd.uploadbuffer.get(), src_offset, subresourceData.row_pitch, subresourceData.slice_pitch, size, internal_state->texture.get(), slice, mip, origin);
						width = std::max(1u, width / 2);
						height = std::max(1u, height / 2);
					}
					else
					{
						internal_state->texture->replaceRegion(region, mip, slice, subresourceData.data_ptr, subresourceData.row_pitch, datasize);
						width = std::max(block_size, width / 2);
						height = std::max(block_size, height / 2);
					}
					depth = std::max(1u, depth / 2);
					src_offset += datasize;
				}
			}
			
			if (cmd.IsValid())
			{
				copyAllocator.submit(cmd);
			}
		}
		
		if (!has_flag(desc->misc_flags, ResourceMiscFlag_NO_DEFAULT_DESCRIPTORS))
		{
			if (has_flag(texture->desc.bind_flags, BindFlag_RENDER_TARGET))
			{
				CreateSubresource(texture, SubresourceType_RTV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag_DEPTH_STENCIL))
			{
				CreateSubresource(texture, SubresourceType_DSV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag_SHADER_RESOURCE))
			{
				CreateSubresource(texture, SubresourceType_SRV, 0, -1, 0, -1);
			}
			if (has_flag(texture->desc.bind_flags, BindFlag_UNORDERED_ACCESS))
			{
				CreateSubresource(texture, SubresourceType_UAV, 0, -1, 0, -1);
			}
		}
		
		return internal_state->texture.get() != nullptr || internal_state->buffer.get() != nullptr;
	}

	bool MetalDevice::CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader, const char* entrypoint) const
	{
		auto internal_state = Protean::Allocator::make_shared<Shader_Metal>();
		internal_state->allocationhandler = allocationhandler;
		shader->internal_state = internal_state;
		
		// Offline reflection gathered data that's required to bring HLSL shaders to Metal is stored tightly after shadercode data:
		if (
			stage == ShaderStage_VS ||
			stage == ShaderStage_GS ||
			stage == ShaderStage_CS ||
			stage == ShaderStage_MS ||
			stage == ShaderStage_AS
			)
		{
			shadercode_size -= sizeof(internal_state->additional_data);
			std::memcpy(&internal_state->additional_data, (uint8_t*)shadercode + shadercode_size, sizeof(internal_state->additional_data));
		}
		
		dispatch_data_t bytecodeData = dispatch_data_create(shadercode, shadercode_size, dispatch_get_main_queue(), nullptr);
		NS::Error* error = nullptr;
		internal_state->library = NS::TransferPtr(device->newLibrary(bytecodeData, &error));
		if(error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_assert(0, "%s", errDesc->utf8String());
			error->release();
		}
		assert(internal_state->library.get() != nullptr);
		
		NS::SharedPtr<NS::String> entry = NS::TransferPtr(NS::String::alloc()->init(entrypoint, NS::UTF8StringEncoding));
		NS::SharedPtr<MTL::FunctionConstantValues> constants = NS::TransferPtr(MTL::FunctionConstantValues::alloc()->init());
		
		if (stage == ShaderStage_HS || stage == ShaderStage_DS || stage == ShaderStage_GS)
		{
			// These will be used only by emulated pipeline creation, not native shader functions:
			return internal_state->library.get() != nullptr;
		}
		
		error = nullptr;
		internal_state->function = NS::TransferPtr(internal_state->library->newFunction(entry.get(), constants.get(), &error));
		if(error != nullptr)
		{
			NS::String* errDesc = error->localizedDescription();
			wilog_error("%s", errDesc->utf8String());
			error->release();
		}
		assert(internal_state->function.get() != nullptr);
		
		if (stage == ShaderStage_CS)
		{
			error = nullptr;
			internal_state->compute_pipeline = NS::TransferPtr(device->newComputePipelineState(internal_state->function.get(), &error));
			if (error != nullptr)
			{
				NS::String* errDesc = error->localizedDescription();
				wilog_error("%s", errDesc->utf8String());
				assert(0);
				error->release();
			}
			
			return internal_state->compute_pipeline.get() != nullptr;
		}

		return internal_state->function.get() != nullptr;
	}

	bool MetalDevice::CreateSampler(const SamplerDesc* desc, Sampler* sampler) const
	{
		auto internal_state = Protean::Allocator::make_shared<Sampler_Metal>();
		internal_state->allocationhandler = allocationhandler;
		sampler->internal_state = internal_state;
		sampler->desc = *desc;
		
		NS::SharedPtr<MTL::SamplerDescriptor> descriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
		descriptor->setSupportArgumentBuffers(true);
		MTL::SamplerMipFilter mip_filter = {};
		switch (desc->filter)
		{
			case Filter_MIN_MAG_MIP_POINT:
			case Filter_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_MIN_MAG_LINEAR_MIP_POINT:
			case Filter_COMPARISON_MIN_MAG_MIP_POINT:
			case Filter_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			case Filter_MINIMUM_MIN_MAG_MIP_POINT:
			case Filter_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			case Filter_MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
				mip_filter = MTL::SamplerMipFilterNearest;
				break;
			default:
				mip_filter = MTL::SamplerMipFilterLinear;
				break;
		}
		descriptor->setMipFilter(mip_filter);
		MTL::SamplerMinMagFilter min_filter = {};
		switch (desc->filter)
		{
			case Filter_MIN_MAG_MIP_POINT:
			case Filter_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_MIN_POINT_MAG_MIP_LINEAR:
			case Filter_COMPARISON_MIN_MAG_MIP_POINT:
			case Filter_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			case Filter_MINIMUM_MIN_MAG_MIP_POINT:
			case Filter_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			case Filter_MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
				min_filter = MTL::SamplerMinMagFilterNearest;
				break;
			default:
				min_filter = MTL::SamplerMinMagFilterLinear;
				break;
		}
		descriptor->setMinFilter(min_filter);
		MTL::SamplerMinMagFilter mag_filter = {};
		switch (desc->filter)
		{
			case Filter_MIN_MAG_MIP_POINT:
			case Filter_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter_COMPARISON_MIN_MAG_MIP_POINT:
			case Filter_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter_MINIMUM_MIN_MAG_MIP_POINT:
			case Filter_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter_MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
				mag_filter = MTL::SamplerMinMagFilterNearest;
				break;
			default:
				mag_filter = MTL::SamplerMinMagFilterLinear;
				break;
		}
		descriptor->setMagFilter(mag_filter);
		MTL::SamplerReductionMode reduction_mode = {};
		switch (desc->filter)
		{
			case Filter_MINIMUM_MIN_MAG_MIP_POINT:
			case Filter_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			case Filter_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			case Filter_MINIMUM_MIN_MAG_MIP_LINEAR:
			case Filter_MINIMUM_ANISOTROPIC:
				reduction_mode = MTL::SamplerReductionModeMinimum;
				break;
			case Filter_MAXIMUM_MIN_MAG_MIP_POINT:
			case Filter_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			case Filter_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			case Filter_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			case Filter_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			case Filter_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			case Filter_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			case Filter_MAXIMUM_MIN_MAG_MIP_LINEAR:
			case Filter_MAXIMUM_ANISOTROPIC:
				reduction_mode = MTL::SamplerReductionModeMaximum;
				break;
			default:
				reduction_mode = MTL::SamplerReductionModeWeightedAverage;
				break;
		}
		descriptor->setReductionMode(reduction_mode);
		descriptor->setCompareFunction(_ConvertCompareFunction(desc->comparison_func));
		descriptor->setLodMinClamp(desc->min_lod);
		descriptor->setLodMaxClamp(desc->max_lod);
		descriptor->setMaxAnisotropy(clamp(desc->max_anisotropy, 1u, 16u));
		descriptor->setLodBias(desc->mip_lod_bias);
		MTL::SamplerBorderColor border_color = {};
		switch (desc->border_color)
		{
			case SamplerBorderColor_TRANSPARENT_BLACK:
				border_color = MTL::SamplerBorderColorTransparentBlack;
				break;
			case SamplerBorderColor_OPAQUE_BLACK:
				border_color = MTL::SamplerBorderColorOpaqueBlack;
				break;
			case SamplerBorderColor_OPAQUE_WHITE:
				border_color = MTL::SamplerBorderColorOpaqueWhite;
				break;
		}
		descriptor->setBorderColor(border_color);
		descriptor->setSAddressMode(_ConvertAddressMode(desc->address_u));
		descriptor->setTAddressMode(_ConvertAddressMode(desc->address_v));
		descriptor->setRAddressMode(_ConvertAddressMode(desc->address_w));
		internal_state->sampler = NS::TransferPtr(device->newSamplerState(descriptor.get()));
		internal_state->index = allocationhandler->allocate_sampler_index();
		internal_state->entry = create_entry(internal_state->sampler.get());
		std::memcpy(descriptor_heap_sam_data + internal_state->index, &internal_state->entry, sizeof(internal_state->entry));

		return internal_state->sampler.get() != nullptr;
	}

	bool MetalDevice::CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const
	{
		auto internal_state = Protean::Allocator::make_shared<QueryHeap_Metal>();
		internal_state->allocationhandler = allocationhandler;
		queryheap->internal_state = internal_state;
		queryheap->desc = *desc;
		
		switch (desc->type)
		{
			case GpuQueryType_OCCLUSION:
			case GpuQueryType_OCCLUSION_BINARY:
				internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->query_count * sizeof(uint64_t), MTL::ResourceStorageModePrivate));
				break;
				
			case GpuQueryType_TIMESTAMP:
			{
				NS::SharedPtr<MTL4::CounterHeapDescriptor> descriptor = NS::TransferPtr(MTL4::CounterHeapDescriptor::alloc()->init());
				descriptor->setType(MTL4::CounterHeapTypeTimestamp);
				descriptor->setCount(desc->query_count);
				NS::Error* error = nullptr;
				internal_state->counter_heap = NS::TransferPtr(device->newCounterHeap(descriptor.get(), &error));
				if (error != nullptr)
				{
					NS::String* errDesc = error->localizedDescription();
					wilog_assert(0, "%s", errDesc->utf8String());
					error->release();
				}
				internal_state->buffer = NS::TransferPtr(device->newBuffer(desc->query_count * sizeof(uint64_t), MTL::ResourceStorageModeShared));
			}
			break;
				
			default:
				break;
		}
		
		return internal_state->buffer.get() != nullptr;
	}

	bool MetalDevice::CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info) const
	{
		auto internal_state = Protean::Allocator::make_shared<PipelineState_Metal>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;
		pso->desc = *desc;

		const DepthStencilState& dss_desc = pso->desc.dss != nullptr ? *pso->desc.dss : default_depthstencilstate;
		const BlendState& bs_desc = pso->desc.bs != nullptr ? *pso->desc.bs : default_blendstate;
		
		if (desc->vs != nullptr)
		{
			internal_state->descriptor = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
			auto shader_internal = to_internal(desc->vs);
			internal_state->descriptor->setVertexFunction(shader_internal->function.get());
			internal_state->needs_draw_params = shader_internal->additional_data.needs_draw_params;
			internal_state->gs_desc.vertexLibrary = shader_internal->library.get();
			internal_state->gs_desc.vertexFunctionName = "main";
			internal_state->gs_desc.stageInLibrary = shader_internal->library.get();
			internal_state->gs_desc.pipelineConfig.gsVertexSizeInBytes = shader_internal->additional_data.vertex_output_size_in_bytes;
		}
		if (desc->ds != nullptr)
		{
			return false; // TODO
		}
		if (desc->hs != nullptr)
		{
			return false; // TODO
		}
		if (desc->gs != nullptr)
		{
			internal_state->ms_descriptor = NS::TransferPtr(MTL::MeshRenderPipelineDescriptor::alloc()->init());
			auto shader_internal = to_internal(desc->gs);
			internal_state->gs_desc.basePipelineDescriptor = internal_state->ms_descriptor.get();
			internal_state->gs_desc.geometryLibrary = shader_internal->library.get();
			internal_state->gs_desc.geometryFunctionName = "main";
			internal_state->gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup = shader_internal->additional_data.max_input_primitives_per_mesh_threadgroup;
		}
		if (desc->ms != nullptr)
		{
			internal_state->ms_descriptor = NS::TransferPtr(MTL::MeshRenderPipelineDescriptor::alloc()->init());
			auto shader_internal = to_internal(desc->ms);
			internal_state->ms_descriptor->setMeshFunction(shader_internal->function.get());
			internal_state->numthreads_ms = shader_internal->additional_data.numthreads;
		}
		if (desc->as != nullptr)
		{
			auto shader_internal = to_internal(desc->as);
			internal_state->ms_descriptor->setObjectFunction(shader_internal->function.get());
			internal_state->numthreads_as = shader_internal->additional_data.numthreads;
		}
		if (desc->ps != nullptr)
		{
			auto shader_internal = to_internal(desc->ps);
			if (internal_state->descriptor.get() != nullptr)
				internal_state->descriptor->setFragmentFunction(shader_internal->function.get());
			else
				internal_state->ms_descriptor->setFragmentFunction(shader_internal->function.get());
			internal_state->gs_desc.fragmentLibrary = shader_internal->library.get();
			internal_state->gs_desc.fragmentFunctionName = "main";
		}
		
		NS::SharedPtr<MTL::VertexDescriptor> vertex_descriptor;
		if (desc->il != nullptr && internal_state->descriptor.get() != nullptr)
		{
			vertex_descriptor = NS::TransferPtr(MTL::VertexDescriptor::alloc()->init());
			uint64_t input_slot_strides[32] = {};
			for (size_t i = 0; i < desc->il->elements.size(); ++i)
			{
				const InputLayout::Element& element = desc->il->elements[i];
				input_slot_strides[element.input_slot] += GetFormatStride(element.format);
			}
			uint64_t offset = 0;
			for (size_t i = 0; i < desc->il->elements.size(); ++i)
			{
				const InputLayout::Element& element = desc->il->elements[i];
				const uint64_t element_stride = GetFormatStride(element.format);
				const uint64_t input_slot_stride = input_slot_strides[element.input_slot];
				MTL::VertexBufferLayoutDescriptor* layout = vertex_descriptor->layouts()->object(kIRVertexBufferBindPoint + i);
				layout->setStride(input_slot_stride);
				layout->setStepFunction(element.input_slot_class == InputClassification_PER_VERTEX_DATA ? MTL::VertexStepFunctionPerVertex : MTL::VertexStepFunctionPerInstance);
				layout->setStepRate(1);
				MTL::VertexAttributeDescriptor* attribute = vertex_descriptor->attributes()->object(kIRStageInAttributeStartIndex + i);
				attribute->setFormat(_ConvertVertexFormat(element.format));
				attribute->setOffset(element.aligned_byte_offset == InputLayout::APPEND_ALIGNED_ELEMENT ? offset : element.aligned_byte_offset);
				attribute->setBufferIndex(kIRVertexBufferBindPoint + i);
				if (element.aligned_byte_offset != InputLayout::APPEND_ALIGNED_ELEMENT)
				{
					offset = element.aligned_byte_offset;
				}
				offset += element_stride;
			}
			internal_state->descriptor->setVertexDescriptor(vertex_descriptor.get());
		}
		
		if (internal_state->descriptor.get() != nullptr)
		{
			switch (desc->pt)
			{
				case PrimitiveTopology_TRIANGLELIST:
				case PrimitiveTopology_TRIANGLESTRIP:
				case PrimitiveTopology_PATCHLIST:
					internal_state->descriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassTriangle);
					break;
				case PrimitiveTopology_LINELIST:
				case PrimitiveTopology_LINESTRIP:
					internal_state->descriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassLine);
					break;
				case PrimitiveTopology_POINTLIST:
					internal_state->descriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClassPoint);
					break;
				default:
					break;
			}
		}
		
		if (renderpass_info != nullptr)
		{
			// When renderpass_info is provided, it will be a completely precompiled pipeline state only useable by that renderpass type:
			NS::SharedPtr<MTL::RenderPipelineColorAttachmentDescriptor> attachments[8] = {
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
				NS::TransferPtr(MTL::RenderPipelineColorAttachmentDescriptor::alloc()->init()),
			};
			if (internal_state->descriptor.get() != nullptr)
				internal_state->descriptor->setAlphaToCoverageEnabled(bs_desc.alpha_to_coverage_enable);
			else
				internal_state->ms_descriptor->setAlphaToCoverageEnabled(bs_desc.alpha_to_coverage_enable);
			for (uint32_t i = 0; i < renderpass_info->rt_count; ++i)
			{
				MTL::RenderPipelineColorAttachmentDescriptor& attachment = *attachments[i].get();
				attachment.setPixelFormat(_ConvertPixelFormat(renderpass_info->rt_formats[i]));
				
				const BlendState::RenderTargetBlendState& bs_rt = bs_desc.render_target[i];
				MTL::ColorWriteMask color_write_mask = {};
				if (has_flag(bs_rt.render_target_write_mask, ColorWrite_ENABLE_RED))
				{
					color_write_mask |= MTL::ColorWriteMaskRed;
				}
				if (has_flag(bs_rt.render_target_write_mask, ColorWrite_ENABLE_GREEN))
				{
					color_write_mask |= MTL::ColorWriteMaskGreen;
				}
				if (has_flag(bs_rt.render_target_write_mask, ColorWrite_ENABLE_BLUE))
				{
					color_write_mask |= MTL::ColorWriteMaskBlue;
				}
				if (has_flag(bs_rt.render_target_write_mask, ColorWrite_ENABLE_ALPHA))
				{
					color_write_mask |= MTL::ColorWriteMaskAlpha;
				}
				attachment.setWriteMask(color_write_mask);
				attachment.setBlendingEnabled(bs_rt.blend_enable);
				attachment.setRgbBlendOperation(_ConvertBlendOp(bs_rt.blend_op));
				attachment.setAlphaBlendOperation(_ConvertBlendOp(bs_rt.blend_op_alpha));
				attachment.setSourceRGBBlendFactor(_ConvertBlendFactor(bs_rt.src_blend));
				attachment.setSourceAlphaBlendFactor(_ConvertBlendFactor(bs_rt.src_blend_alpha));
				attachment.setDestinationRGBBlendFactor(_ConvertBlendFactor(bs_rt.dest_blend));
				attachment.setDestinationAlphaBlendFactor(_ConvertBlendFactor(bs_rt.dest_blend_alpha));
				if (internal_state->descriptor.get() != nullptr)
					internal_state->descriptor->colorAttachments()->setObject(&attachment, i);
				else
					internal_state->ms_descriptor->colorAttachments()->setObject(&attachment, i);
			}
			if (internal_state->descriptor.get() != nullptr)
			{
				internal_state->descriptor->setDepthAttachmentPixelFormat(_ConvertPixelFormat(renderpass_info->ds_format));
				if (IsFormatStencilSupport(renderpass_info->ds_format))
				{
					internal_state->descriptor->setStencilAttachmentPixelFormat(_ConvertPixelFormat(renderpass_info->ds_format));
				}
			}
			else
			{
				internal_state->ms_descriptor->setDepthAttachmentPixelFormat(_ConvertPixelFormat(renderpass_info->ds_format));
				if (IsFormatStencilSupport(renderpass_info->ds_format))
				{
					internal_state->ms_descriptor->setStencilAttachmentPixelFormat(_ConvertPixelFormat(renderpass_info->ds_format));
				}
			}
			
			uint32_t sample_count = renderpass_info->sample_count;
			while (sample_count > 1 && !device->supportsTextureSampleCount(sample_count))
			{
				sample_count /= 2;
			}
			if (internal_state->descriptor.get() != nullptr)
				internal_state->descriptor->setRasterSampleCount(sample_count);
			else
				internal_state->ms_descriptor->setRasterSampleCount(sample_count);
			
			NS::SharedPtr<MTL::DepthStencilDescriptor> depth_stencil_desc = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
			if (dss_desc.depth_enable && renderpass_info->ds_format != Format_UNKNOWN)
			{
				depth_stencil_desc->setDepthCompareFunction(_ConvertCompareFunction(dss_desc.depth_func));
				depth_stencil_desc->setDepthWriteEnabled(dss_desc.depth_write_mask == DepthWriteMask_ALL);
			}
			NS::SharedPtr<MTL::StencilDescriptor> stencil_front;
			NS::SharedPtr<MTL::StencilDescriptor> stencil_back;
			if (dss_desc.stencil_enable && IsFormatStencilSupport(renderpass_info->ds_format))
			{
				stencil_front = NS::TransferPtr(MTL::StencilDescriptor::alloc()->init());
				stencil_back = NS::TransferPtr(MTL::StencilDescriptor::alloc()->init());
				stencil_front->setReadMask(dss_desc.stencil_read_mask);
				stencil_front->setWriteMask(dss_desc.stencil_write_mask);
				stencil_front->setStencilCompareFunction(_ConvertCompareFunction(dss_desc.front_face.stencil_func));
				stencil_front->setStencilFailureOperation(_ConvertStencilOperation(dss_desc.front_face.stencil_fail_op));
				stencil_front->setDepthFailureOperation(_ConvertStencilOperation(dss_desc.front_face.stencil_depth_fail_op));
				stencil_front->setDepthStencilPassOperation(_ConvertStencilOperation(dss_desc.front_face.stencil_pass_op));
				stencil_back->setReadMask(dss_desc.stencil_read_mask);
				stencil_back->setWriteMask(dss_desc.stencil_write_mask);
				stencil_back->setStencilCompareFunction(_ConvertCompareFunction(dss_desc.back_face.stencil_func));
				stencil_back->setStencilFailureOperation(_ConvertStencilOperation(dss_desc.back_face.stencil_fail_op));
				stencil_back->setDepthFailureOperation(_ConvertStencilOperation(dss_desc.back_face.stencil_depth_fail_op));
				stencil_back->setDepthStencilPassOperation(_ConvertStencilOperation(dss_desc.back_face.stencil_pass_op));
				depth_stencil_desc->setFrontFaceStencil(stencil_front.get());
				depth_stencil_desc->setBackFaceStencil(stencil_back.get());
			}
			internal_state->depth_stencil_state = NS::TransferPtr(device->newDepthStencilState(depth_stencil_desc.get()));
			
			MTL::AutoreleasedRenderPipelineReflection* reflection = nullptr;
			NS::Error* error = nullptr;
			if (internal_state->gs_desc.basePipelineDescriptor != nullptr)
				internal_state->render_pipeline = NS::TransferPtr(IRRuntimeNewGeometryEmulationPipeline(device.get(), &internal_state->gs_desc, &error));
			else if (internal_state->descriptor.get() != nullptr)
				internal_state->render_pipeline = NS::TransferPtr(device->newRenderPipelineState(internal_state->descriptor.get(), &error));
			else
				internal_state->render_pipeline = NS::TransferPtr(device->newRenderPipelineState(internal_state->ms_descriptor.get(), MTL::PipelineOptionNone, reflection, &error));
			if (error != nullptr)
			{
				NS::String* errDesc = error->localizedDescription();
				wilog_assert(0, "%s", errDesc->utf8String());
				error->release();
			}
			
			return internal_state->render_pipeline.get() != nullptr;
		}
		
		// If we get here, this pipeline state is not complete, but it will be reuseable by different render passes (and compiled just in time at runtime)
		return true;
	}

	static NS::SharedPtr<MTL4::AccelerationStructureDescriptor> mtl_acceleration_structure_descriptor(const RaytracingAccelerationStructureDesc* desc)
	{
		NS::SharedPtr<MTL4::AccelerationStructureDescriptor> descriptor;
		
		NS::SharedPtr<NS::Array> object_array;
		Protean::vector<NS::SharedPtr<MTL4::AccelerationStructureGeometryDescriptor>> geometry_descs;
		Protean::vector<MTL4::AccelerationStructureGeometryDescriptor*> geometry_descs_raw;
		
		if (desc->type == RaytracingAccelerationStructureDesc::Type_TOPLEVEL)
		{
			NS::SharedPtr<MTL4::InstanceAccelerationStructureDescriptor> instance_descriptor = NS::TransferPtr(MTL4::InstanceAccelerationStructureDescriptor::alloc()->init());
			instance_descriptor->setInstanceCount(desc->top_level.count);
			auto buffer_internal = to_internal(&desc->top_level.instance_buffer);
			instance_descriptor->setInstanceDescriptorBuffer({buffer_internal->gpu_address + desc->top_level.offset, buffer_internal->buffer->length()});
			instance_descriptor->setInstanceDescriptorStride(sizeof(MTL::IndirectAccelerationStructureInstanceDescriptor));
			instance_descriptor->setInstanceDescriptorType(MTL::AccelerationStructureInstanceDescriptorTypeIndirect);
			instance_descriptor->setInstanceTransformationMatrixLayout(MTL::MatrixLayoutRowMajor);
			descriptor = instance_descriptor;
		}
		else
		{
			auto primitive_descriptor = NS::TransferPtr(MTL4::PrimitiveAccelerationStructureDescriptor::alloc()->init());
			geometry_descs.reserve(desc->bottom_level.geometries.size());
			
			for (auto& x : desc->bottom_level.geometries)
			{
				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_TRIANGLES)
				{
					NS::SharedPtr<MTL4::AccelerationStructureTriangleGeometryDescriptor> geo = NS::TransferPtr(MTL4::AccelerationStructureTriangleGeometryDescriptor::alloc()->init());
					geometry_descs.push_back(geo);
					geo->setAllowDuplicateIntersectionFunctionInvocation((x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION) == 0);
					geo->setOpaque(x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE);
					auto ib_internal = to_internal(&x.triangles.index_buffer);
					auto vb_internal = to_internal(&x.triangles.vertex_buffer);
					geo->setIndexType(x.triangles.index_format == IndexBufferFormat_UINT32 ? MTL::IndexTypeUInt32 : MTL::IndexTypeUInt16);
					const uint64_t index_byteoffset = x.triangles.index_offset * GetIndexStride(x.triangles.index_format);
					geo->setIndexBuffer({ib_internal->gpu_address + index_byteoffset, ib_internal->buffer->length()});
					geo->setVertexFormat(_ConvertAttributeFormat(x.triangles.vertex_format));
					geo->setVertexBuffer({vb_internal->gpu_address + x.triangles.vertex_byte_offset, vb_internal->buffer->length()});
					geo->setVertexStride(x.triangles.vertex_stride);
					geo->setTriangleCount(x.triangles.index_count / 3);
					if (x.triangles.transform_3x4_buffer.IsValid())
					{
						auto transform_internal = to_internal(&x.triangles.transform_3x4_buffer);
						geo->setTransformationMatrixBuffer({transform_internal->gpu_address + x.triangles.transform_3x4_buffer_offset, transform_internal->buffer->length()});
						geo->setTransformationMatrixLayout(MTL::MatrixLayoutRowMajor);
					}
				}
				else
				{
					NS::SharedPtr<MTL4::AccelerationStructureBoundingBoxGeometryDescriptor> geo = NS::TransferPtr(MTL4::AccelerationStructureBoundingBoxGeometryDescriptor::alloc()->init());
					geometry_descs.push_back(geo);
					geo->setOpaque(x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE);
					geo->setBoundingBoxCount(x.aabbs.count);
					auto buffer_internal = to_internal(&x.aabbs.aabb_buffer);
					geo->setBoundingBoxBuffer({buffer_internal->gpu_address + x.aabbs.offset, buffer_internal->buffer->length()});
					geo->setBoundingBoxStride(x.aabbs.stride);
				}
			}
			
			geometry_descs_raw.reserve(geometry_descs.size());
			for (auto& x : geometry_descs)
			{
				geometry_descs_raw.push_back(x.get());
			}
			object_array = NS::TransferPtr(NS::Array::array((NS::Object**)geometry_descs_raw.data(), geometry_descs_raw.size())->retain());
			primitive_descriptor->setGeometryDescriptors(object_array.get());
			descriptor = primitive_descriptor;
		}
		
		MTL::AccelerationStructureUsage usage = MTL::AccelerationStructureUsageNone;
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			usage |= MTL::AccelerationStructureUsageRefit;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			usage |= MTL::AccelerationStructureUsagePreferFastBuild;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			usage |= MTL::AccelerationStructureUsagePreferFastIntersection;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			usage |= MTL::AccelerationStructureUsageMinimizeMemory;
		}
		descriptor->setUsage(usage);
		
		return descriptor;
	}

	bool MetalDevice::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const
	{
		auto internal_state = Protean::Allocator::make_shared<BVH_Metal>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::Type_RAYTRACING_ACCELERATION_STRUCTURE;
		bvh->desc = *desc;
		bvh->size = 0;
		
		NS::SharedPtr<MTL4::AccelerationStructureDescriptor> descriptor = mtl_acceleration_structure_descriptor(desc);
		
		const MTL::AccelerationStructureSizes size = device->accelerationStructureSizes(descriptor.get());
		internal_state->acceleration_structure = NS::TransferPtr(device->newAccelerationStructure(size.accelerationStructureSize));
		internal_state->scratch = NS::TransferPtr(device->newBuffer(std::max(size.buildScratchBufferSize, size.refitScratchBufferSize), MTL::ResourceStorageModePrivate));

		allocationhandler->make_resident(internal_state->acceleration_structure.get());
		allocationhandler->make_resident(internal_state->scratch.get());
		
		internal_state->resourceid = internal_state->acceleration_structure->gpuResourceID();
		
		if (desc->type == RaytracingAccelerationStructureDesc::Type_TOPLEVEL)
		{
			internal_state->tlas_header_instancecontributions = NS::TransferPtr(device->newBuffer(sizeof(IRRaytracingAccelerationStructureGPUHeader) + sizeof(uint32_t) * desc->top_level.count, MTL::ResourceStorageModeShared));
			allocationhandler->make_resident(internal_state->tlas_header_instancecontributions.get());
			Protean::vector<uint32_t> instance_contributions(desc->top_level.count);
			std::fill(instance_contributions.begin(), instance_contributions.end(), 0);
			IRRaytracingAccelerationStructureGPUHeader* header = (IRRaytracingAccelerationStructureGPUHeader*)internal_state->tlas_header_instancecontributions->contents();
			uint8_t* instancecontributions_gpudata = (uint8_t*)header + sizeof(IRRaytracingAccelerationStructureGPUHeader);
			MTL::GPUAddress header_gpuaddress = internal_state->tlas_header_instancecontributions->gpuAddress();
			MTL::GPUAddress instancecontributions_gpuaddress = header_gpuaddress + sizeof(IRRaytracingAccelerationStructureGPUHeader);
			IRRaytracingSetAccelerationStructure((uint8_t*)header, internal_state->resourceid, instancecontributions_gpudata, instancecontributions_gpuaddress, instance_contributions.data(), desc->top_level.count);
			IRDescriptorTableSetAccelerationStructure(&internal_state->tlas_entry, header_gpuaddress);
			internal_state->tlas_descriptor_index = allocationhandler->allocate_resource_index();
			std::memcpy(descriptor_heap_res_data + internal_state->tlas_descriptor_index, &internal_state->tlas_entry, sizeof(internal_state->tlas_entry));
		}
		
		return internal_state->acceleration_structure.get() != nullptr;
	}

	bool MetalDevice::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const
	{
		// TODO
		return false;
	}

	bool MetalDevice::CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const
	{
		if (desc == nullptr || video_decoder == nullptr)
			return false;
		if (desc->profile != VideoProfile_H264 || !CheckCapability(GraphicsDeviceCapability_VIDEO_DECODE_H264))
			return false;
		if (desc->format != Format_NV12 || desc->width == 0 || desc->height == 0)
			return false;
		if (desc->sps_count == 0 || desc->pps_count == 0 || desc->sps_datas == nullptr || desc->pps_datas == nullptr)
			return false;
		const h264::SPS* sps_array = (const h264::SPS*)desc->sps_datas;
		const h264::PPS* pps_array = (const h264::PPS*)desc->pps_datas;
		if (sps_array->bit_depth_luma_minus8 != 0 || sps_array->bit_depth_chroma_minus8 != 0 || sps_array->chroma_format_idc > 1)
		{
			wilog_error("Metal H264 decoder supports 8-bit 4:2:0 only");
			return false;
		}

		constexpr size_t kMaxParameterSets = 32;
		if (desc->sps_count + desc->pps_count > kMaxParameterSets)
			return false;

		unsigned char parameter_bytes[4096];
		int parameter_used = 0;
		size_t parameter_offsets[kMaxParameterSets];
		size_t parameter_sizes[kMaxParameterSets];
		size_t parameter_count = 0;
		for (size_t i = 0; i < desc->sps_count; ++i)
		{
			const int written = h264::write_sps(&sps_array[i], parameter_bytes + parameter_used, (int)sizeof(parameter_bytes) - parameter_used);
			if (written <= 0)
			{
				wilog_error("Metal H264 decoder could not write SPS");
				return false;
			}
			parameter_offsets[parameter_count] = (size_t)parameter_used;
			parameter_sizes[parameter_count] = (size_t)written;
			parameter_used += written;
			++parameter_count;
		}
		for (size_t i = 0; i < desc->pps_count; ++i)
		{
			const int written = h264::write_pps(&pps_array[i], parameter_bytes + parameter_used, (int)sizeof(parameter_bytes) - parameter_used);
			if (written <= 0)
			{
				wilog_error("Metal H264 decoder could not write PPS");
				return false;
			}
			parameter_offsets[parameter_count] = (size_t)parameter_used;
			parameter_sizes[parameter_count] = (size_t)written;
			parameter_used += written;
			++parameter_count;
		}
		const uint8_t* parameter_sets[kMaxParameterSets];
		for (size_t i = 0; i < parameter_count; ++i)
			parameter_sets[i] = parameter_bytes + parameter_offsets[i];
		CMVideoFormatDescriptionRef format_desc = nullptr;
		const OSStatus format_status = CMVideoFormatDescriptionCreateFromH264ParameterSets(
			kCFAllocatorDefault,
			parameter_count,
			parameter_sets,
			parameter_sizes,
			4,
			&format_desc
		);
		if (format_status != noErr || format_desc == nullptr)
		{
			if (format_desc != nullptr)
				CFRelease(format_desc);
			wilog_error("CMVideoFormatDescriptionCreateFromH264ParameterSets failed (%d)", (int)format_status);
			return false;
		}

		// Age 0 disables eviction. Retained CVMetalTextures keep each surface alive until the GPU copy retires.
		CVMetalTextureCacheRef texture_cache = nullptr;
		double texture_age = 0;
		CFNumberRef age_num = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &texture_age);
		const void* cache_keys[] = { kCVMetalTextureCacheMaximumTextureAgeKey };
		const void* cache_values[] = { age_num };
		CFDictionaryRef cache_attrs = age_num != nullptr ? CFDictionaryCreate(kCFAllocatorDefault, cache_keys, cache_values, 1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks) : nullptr;
		const CVReturn cache_status = CVMetalTextureCacheCreate(kCFAllocatorDefault, cache_attrs, device.get(), nullptr, &texture_cache);
		if (cache_attrs != nullptr)
			CFRelease(cache_attrs);
		if (age_num != nullptr)
			CFRelease(age_num);
		if (cache_status != kCVReturnSuccess || texture_cache == nullptr)
		{
			if (texture_cache != nullptr)
				CFRelease((CFTypeRef)texture_cache);
			CFRelease(format_desc);
			wilog_error("CVMetalTextureCacheCreate failed (%d)", (int)cache_status);
			return false;
		}

		auto internal_state = Protean::Allocator::make_shared<VideoDecoder_Metal>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->format_desc = format_desc;
		internal_state->texture_cache = texture_cache;
		internal_state->sample_bytes.reserve((size_t)desc->width * (size_t)desc->height);
		internal_state->pending_copies.reserve(GraphicsDevice::GetBufferCount() + 1);
		video_decoder->internal_state = internal_state;
		video_decoder->desc = *desc;
		video_decoder->support = VideoDecoderSupportFlags_DPB_AND_OUTPUT_COINCIDE;
		return true;
	}

	int MetalDevice::CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change, const ImageAspect* aspect, const Swizzle* swizzle, float min_lod_clamp) const
	{
		auto internal_state = to_internal(texture);

		Format format = texture->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}

		MTL::Texture* src_texture = internal_state->texture.get();
		if (texture->GetDesc().format == Format_NV12)
		{
			const bool chroma = (aspect != nullptr && *aspect == ImageAspect_CHROMINANCE) || format == Format_R8G8_UNORM;
			if (chroma)
			{
				src_texture = internal_state->texture_chroma.get();
				if (format == Format_NV12)
					format = Format_R8G8_UNORM;
			}
			else if (format == Format_NV12)
			{
				format = Format_R8_UNORM;
			}
		}
		
		const MTL::PixelFormat pixelformat = _ConvertPixelFormat(format);
		assert(pixelformat != MTL::PixelFormatInvalid);
		assert(src_texture != nullptr);
		
		sliceCount = std::min(sliceCount, texture->desc.array_size - firstSlice);
		mipCount = std::min(mipCount, texture->desc.mip_levels - firstMip);
		
		MTL::TextureType texture_type = src_texture->textureType();
		if (type != SubresourceType_SRV &&
			(texture_type == MTL::TextureTypeCube || texture_type == MTL::TextureTypeCubeArray)
			)
		{
			texture_type = MTL::TextureType2DArray;
		}
		
		NS::SharedPtr<MTL::TextureViewDescriptor> view_desc;
		
		switch (type) {
			case SubresourceType_SRV:
			case SubresourceType_UAV:
				view_desc = NS::TransferPtr(MTL::TextureViewDescriptor::alloc()->init());
				view_desc->setLevelRange({firstMip, mipCount});
				view_desc->setSliceRange({firstSlice, sliceCount});
				view_desc->setPixelFormat(pixelformat);
				view_desc->setTextureType(texture_type);
				break;
			default:
				break;
		};
		
		switch (type) {
			case SubresourceType_SRV:
				{
					MTL::TextureSwizzleChannels mtlswizzle = MTL::TextureSwizzleChannels::Default();
					Swizzle requested_swizzle = texture->desc.swizzle;
					if (swizzle != nullptr)
					{
						requested_swizzle = *swizzle;
					}
					mtlswizzle.red = _ConvertComponentSwizzle(requested_swizzle.r);
					mtlswizzle.green = _ConvertComponentSwizzle(requested_swizzle.g);
					mtlswizzle.blue = _ConvertComponentSwizzle(requested_swizzle.b);
					mtlswizzle.alpha = _ConvertComponentSwizzle(requested_swizzle.a);
					view_desc->setSwizzle(mtlswizzle);
					if (!internal_state->srv.IsValid())
					{
						auto& subresource = internal_state->srv;
						subresource.index = allocationhandler->allocate_resource_index();
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = texture_view_pool->setTextureView(src_texture, view_desc.get(), subresource.index);
						subresource.entry = create_entry(resourceID, min_lod_clamp);
#else
						subresource.view = NS::TransferPtr(src_texture->newTextureView(view_desc.get()));
						subresource.entry = create_entry(subresource.view.get());
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_srv.emplace_back();
						subresource.index = allocationhandler->allocate_resource_index();
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = texture_view_pool->setTextureView(src_texture, view_desc.get(), subresource.index);
						subresource.entry = create_entry(resourceID, min_lod_clamp);
#else
						subresource.view = NS::TransferPtr(src_texture->newTextureView(view_desc.get()));
						subresource.entry = create_entry(subresource.view.get());
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return (int)internal_state->subresources_srv.size() - 1;
					}
				}
				break;
				
			case SubresourceType_UAV:
				{
					if (!internal_state->uav.IsValid())
					{
						auto& subresource = internal_state->uav;
						subresource.index = allocationhandler->allocate_resource_index();
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = texture_view_pool->setTextureView(src_texture, view_desc.get(), subresource.index);
						subresource.entry = create_entry(resourceID, min_lod_clamp);
#else
						subresource.view = NS::TransferPtr(src_texture->newTextureView(view_desc.get()));
						subresource.entry = create_entry(subresource.view.get());
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_uav.emplace_back();
						subresource.index = allocationhandler->allocate_resource_index();
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = texture_view_pool->setTextureView(src_texture, view_desc.get(), subresource.index);
						subresource.entry = create_entry(resourceID, min_lod_clamp);
#else
						subresource.view = NS::TransferPtr(src_texture->newTextureView(view_desc.get()));
						subresource.entry = create_entry(subresource.view.get());
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return (int)internal_state->subresources_uav.size() - 1;
					}
				}
				break;
				
			case SubresourceType_RTV:
				{
					if (!internal_state->rtv.IsValid())
					{
						auto& subresource = internal_state->rtv;
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_rtv.emplace_back();
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return (int)internal_state->subresources_rtv.size() - 1;
					}
				}
				break;
				
			case SubresourceType_DSV:
				{
					if (!internal_state->dsv.IsValid())
					{
						auto& subresource = internal_state->dsv;
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_dsv.emplace_back();
						subresource.firstMip = firstMip;
						subresource.mipCount = mipCount;
						subresource.firstSlice = firstSlice;
						subresource.sliceCount = sliceCount;
						return (int)internal_state->subresources_dsv.size() - 1;
					}
				}
				break;
				
			default:
				break;
		}

		return -1;
	}

	int MetalDevice::CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size, const Format* format_change, const uint32_t* structuredbuffer_stride_change) const
	{
		auto internal_state = to_internal(buffer);
		NS::SharedPtr<MTL::TextureDescriptor> texture_descriptor;
		
		size = std::min(size, buffer->desc.size - offset);
		
		Format format = buffer->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		
		const bool structured = has_flag(buffer->desc.misc_flags, ResourceMiscFlag_BUFFER_STRUCTURED) || (structuredbuffer_stride_change != nullptr);
		
		const MTL::PixelFormat pixelformat = _ConvertPixelFormat(format);
		if (pixelformat != MTL::PixelFormatInvalid)
		{
			texture_descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
			texture_descriptor->setPixelFormat(pixelformat);
			texture_descriptor->setTextureType(MTL::TextureTypeTextureBuffer);
			texture_descriptor->setResourceOptions(internal_state->buffer->resourceOptions());
			texture_descriptor->setWidth(size / GetFormatStride(format));
			MTL::TextureUsage texture_usage = MTL::TextureUsageShaderRead | MTL::TextureUsagePixelFormatView;
			if (type == SubresourceType_UAV)
			{
				texture_usage |= MTL::TextureUsageShaderWrite;
				switch (texture_descriptor->pixelFormat())
				{
					case MTL::PixelFormatR32Uint:
					case MTL::PixelFormatR32Sint:
					case MTL::PixelFormatRG32Uint:
						texture_usage |= MTL::TextureUsageShaderAtomic;
						break;
					default:
						break;
				}
			}
			texture_descriptor->setUsage(texture_usage);
		}
		
		switch (type) {
			case SubresourceType_SRV:
				{
					if (!internal_state->srv.IsValid())
					{
						auto& subresource = internal_state->srv;
						subresource.index = allocationhandler->allocate_resource_index();
						subresource.offset = offset;
						subresource.size = size;
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = {};
						if (texture_descriptor.get() != nullptr)
						{
							resourceID = texture_view_pool->setTextureViewFromBuffer(internal_state->buffer.get(), texture_descriptor.get(), offset, size, subresource.index);
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, resourceID, format, structured);
#else
						if (texture_descriptor.get() != nullptr)
						{
							subresource.view = NS::TransferPtr(internal_state->buffer->newTexture(texture_descriptor.get(), offset, size));
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, subresource.view.get(), format, structured);
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_srv.emplace_back();
						subresource.index = allocationhandler->allocate_resource_index();
						subresource.offset = offset;
						subresource.size = size;
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = {};
						if (texture_descriptor.get() != nullptr)
						{
							resourceID = texture_view_pool->setTextureViewFromBuffer(internal_state->buffer.get(), texture_descriptor.get(), offset, size, subresource.index);
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, resourceID, format, structured);
#else
						if (texture_descriptor.get() != nullptr)
						{
							subresource.view = NS::TransferPtr(internal_state->buffer->newTexture(texture_descriptor.get(), offset, size));
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, subresource.view.get(), format, structured);
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						return (int)internal_state->subresources_srv.size() - 1;
					}
				}
				break;
			case SubresourceType_UAV:
				{
					if (!internal_state->uav.IsValid())
					{
						auto& subresource = internal_state->uav;
						subresource.index = allocationhandler->allocate_resource_index();
						subresource.offset = offset;
						subresource.size = size;
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = {};
						if (texture_descriptor.get() != nullptr)
						{
							resourceID = texture_view_pool->setTextureViewFromBuffer(internal_state->buffer.get(), texture_descriptor.get(), offset, size, subresource.index);
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, resourceID, format, structured);
#else
						if (texture_descriptor.get() != nullptr)
						{
							subresource.view = NS::TransferPtr(internal_state->buffer->newTexture(texture_descriptor.get(), offset, size));
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, subresource.view.get(), format, structured);
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						return -1;
					}
					else
					{
						auto& subresource = internal_state->subresources_uav.emplace_back();
						subresource.index = allocationhandler->allocate_resource_index();
						subresource.offset = offset;
						subresource.size = size;
#ifdef USE_TEXTURE_VIEW_POOL
						MTL::ResourceID resourceID = {};
						if (texture_descriptor.get() != nullptr)
						{
							resourceID = texture_view_pool->setTextureViewFromBuffer(internal_state->buffer.get(), texture_descriptor.get(), offset, size, subresource.index);
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, resourceID, format, structured);
#else
						if (texture_descriptor.get() != nullptr)
						{
							subresource.view = NS::TransferPtr(internal_state->buffer->newTexture(texture_descriptor.get(), offset, size));
						}
						subresource.entry = create_entry(internal_state->buffer.get(), size, offset, subresource.view.get(), format, structured);
#endif // USE_TEXTURE_VIEW_POOL
						std::memcpy(descriptor_heap_res_data + subresource.index, &subresource.entry, sizeof(subresource.entry));
						return (int)internal_state->subresources_uav.size() - 1;
					}
				}
				break;
			default:
				break;
		}
		
		return -1;
	}

	void MetalDevice::DeleteSubresources(GPUResource* resource)
	{
		std::scoped_lock lck(allocationhandler->destroylocker);
		if (resource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(resource);
			internal_state->destroy_subresources();
		}
		else if (resource->IsBuffer())
		{
			auto internal_state = to_internal<GPUBuffer>(resource);
			internal_state->destroy_subresources();
		}
	}

	int MetalDevice::GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource) const
	{
		if (resource == nullptr || !resource->IsValid())
			return -1;

		if (resource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(resource);
			switch (type)
			{
				case SubresourceType_SRV:
					return subresource < 0 ? internal_state->srv.index : internal_state->subresources_srv[subresource].index;
				case SubresourceType_UAV:
					return subresource < 0 ? internal_state->uav.index : internal_state->subresources_uav[subresource].index;
				default:
					break;
			}
		}
		else if (resource->IsBuffer())
		{
			auto internal_state = to_internal<GPUBuffer>(resource);
			switch (type)
			{
				case SubresourceType_SRV:
					return subresource < 0 ? internal_state->srv.index : internal_state->subresources_srv[subresource].index;
				case SubresourceType_UAV:
					return subresource < 0 ? internal_state->uav.index : internal_state->subresources_uav[subresource].index;
				default:
					break;
			}
		}
		else if (resource->IsAccelerationStructure())
		{
			auto internal_state = to_internal<RaytracingAccelerationStructure>(resource);
			return internal_state->tlas_descriptor_index;
		}
		return -1;
	}

	int MetalDevice::GetDescriptorIndex(const Sampler* sampler) const
	{
		if (sampler == nullptr || !sampler->IsValid())
			return -1;
		
		auto internal_state = to_internal(sampler);
		return internal_state->index;
	}

	void MetalDevice::WriteShadingRateValue(ShadingRate rate, void* dest) const
	{
		// TODO
	}

	void MetalDevice::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const
	{
		MTL::IndirectAccelerationStructureInstanceDescriptor descriptor = {};
		if (instance != nullptr)
		{
			descriptor.options = MTL::AccelerationStructureInstanceOptionNone;
			if (instance->flags & RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_CULL_DISABLE)
			{
				descriptor.options |= MTL::AccelerationStructureInstanceOptionDisableTriangleCulling;
			}
			if (instance->flags & RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE)
			{
				descriptor.options |= MTL::AccelerationStructureInstanceOptionTriangleFrontFacingWindingCounterClockwise;
			}
			if (instance->flags & RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_FORCE_OPAQUE)
			{
				descriptor.options |= MTL::AccelerationStructureInstanceOptionOpaque;
			}
			if (instance->flags & RaytracingAccelerationStructureDesc::TopLevel::Instance::FLAG_FORCE_NON_OPAQUE)
			{
				descriptor.options |= MTL::AccelerationStructureInstanceOptionNonOpaque;
			}
			descriptor.mask = instance->instance_mask;
			descriptor.userID = instance->instance_id;
			descriptor.intersectionFunctionTableOffset = 0;
			auto blas_internal = to_internal<RaytracingAccelerationStructure>(instance->bottom_level);
			descriptor.accelerationStructureID = blas_internal->resourceid;
			std::memcpy(&descriptor.transformationMatrix, instance->transform, sizeof(descriptor.transformationMatrix)); // MatrixLayoutRowMajor instance setup!
		}
		std::memcpy(dest, &descriptor, sizeof(descriptor)); // force memcpy into potentially write combined cache memory
	}

	void MetalDevice::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
		// TODO
	}

	void MetalDevice::SetName(GPUResource* pResource, const char* name) const
	{
		if (pResource == nullptr || !pResource->IsValid())
			return;
		NS::SharedPtr<NS::String> str = NS::TransferPtr(NS::String::alloc()->init(name, NS::UTF8StringEncoding));
		if (pResource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(pResource);
			internal_state->texture->setLabel(str.get());
			if (internal_state->texture_chroma.get() != nullptr)
				internal_state->texture_chroma->setLabel(str.get());
		}
		else if (pResource->IsBuffer())
		{
			auto internal_state = to_internal<GPUBuffer>(pResource);
			internal_state->buffer->setLabel(str.get());
		}
		else if (pResource->IsAccelerationStructure())
		{
			auto internal_state = to_internal<RaytracingAccelerationStructure>(pResource);
			internal_state->acceleration_structure->setLabel(str.get());
		}
	}

	void MetalDevice::SetName(Shader* shader, const char* name) const
	{
		if (shader == nullptr || !shader->IsValid())
			return;
		NS::SharedPtr<NS::String> str = NS::TransferPtr(NS::String::alloc()->init(name, NS::UTF8StringEncoding));
		auto internal_state = to_internal(shader);
		internal_state->library->setLabel(str.get());
	}

	SizeAlignment MetalDevice::GetDeviceTextureMemoryRequirements(const TextureDesc* desc) const
	{
		NS::SharedPtr<MTL::TextureDescriptor> descriptor = _ConvertTextureDesc(*desc, device.get(), textureUlongAtomics);
		MTL::SizeAndAlign sizealign = device->heapTextureSizeAndAlign(descriptor.get());
		SizeAlignment ret;
		ret.size = sizealign.size;
		ret.alignment = sizealign.align;
		return ret;
	}

	void MetalDevice::SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count)
	{
		MTL4::CommandQueue* commandqueue = queues[queue].queue.get();
		for (uint32_t i = 0; i < command_count; ++i)
		{
			const SparseUpdateCommand& command = commands[i];
			auto tilepool_internal = to_internal<GPUBuffer>(command.tile_pool);
			MTL::Heap* heap = tilepool_internal->buffer->heap();
			assert(heap != nullptr);
			
			if (command.sparse_resource->IsTexture())
			{
				auto sparse_internal = to_internal<Texture>(command.sparse_resource);
				Protean::vector<MTL4::UpdateSparseTextureMappingOperation> operations;
				operations.reserve(command.num_resource_regions);
				for (uint32_t j = 0; j < command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& coordinate = command.coordinates[j];
					const SparseRegionSize& size = command.sizes[j];
					auto& op = operations.emplace_back();
					op.mode = command.range_flags[j] == TileRangeFlags_Null ? MTL::SparseTextureMappingModeUnmap : MTL::SparseTextureMappingModeMap;
					op.heapOffset = command.range_start_offsets[j];
					op.textureLevel = coordinate.mip;
					op.textureSlice = coordinate.slice;
					op.textureRegion.origin.x = coordinate.x;
					op.textureRegion.origin.y = coordinate.y;
					op.textureRegion.origin.z = coordinate.z;
					op.textureRegion.size.width = size.width;
					op.textureRegion.size.height = size.height;
					op.textureRegion.size.depth = size.depth;
				}
				commandqueue->updateTextureMappings(sparse_internal->texture.get(), heap, operations.data(), operations.size());
			}
			else if (command.sparse_resource->IsBuffer())
			{
				auto sparse_internal = to_internal<GPUBuffer>(command.sparse_resource);
				Protean::vector<MTL4::UpdateSparseBufferMappingOperation> operations;
				operations.reserve(command.num_resource_regions);
				for (uint32_t j = 0; j < command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& coordinate = command.coordinates[j];
					const SparseRegionSize& size = command.sizes[j];
					MTL4::UpdateSparseBufferMappingOperation op = {
						command.range_flags[j] == TileRangeFlags_Null ? MTL::SparseTextureMappingModeUnmap : MTL::SparseTextureMappingModeMap,
						{coordinate.x, size.width},
						command.range_start_offsets[j]
					};
					operations.push_back(op);
				}
				commandqueue->updateBufferMappings(sparse_internal->buffer.get(), heap, operations.data(), operations.size());
			}
		}
		
#if 0
		NS::SharedPtr<MTL::SharedEvent> event = NS::TransferPtr(device->newSharedEvent());
		event->setSignaledValue(0);
		commandqueue->signalEvent(event.get(), 1);
		event->waitUntilSignaledValue(1, ~0ull);
#endif
	}

	void MetalDevice::CopyBufferAsync(const GPUBufferCopyCommand* commands, uint32_t command_count, const char* name) const
	{
		NS::SharedPtr<NS::AutoreleasePool> autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init()); // scoped drain!
		
		CopyAllocator::CopyCMD cmd = copyAllocator.allocate(0);
		
		for (uint32_t i = 0; i < command_count; ++i)
		{
			const GPUBufferCopyCommand& command = commands[i];
			auto dst_internal = to_internal(command.dst);
			auto src_internal = to_internal(command.src);
			cmd.encoder->copyFromBuffer(src_internal->buffer.get(), command.src_offset, dst_internal->buffer.get(), command.dst_offset, command.size);
		}
		
		copyAllocator.submit(cmd, false);
	}
}

#endif // __APPLE__
