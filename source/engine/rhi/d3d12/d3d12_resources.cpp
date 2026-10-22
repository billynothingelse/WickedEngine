#include "rhi/d3d12/d3d12_internal.h"
// D3D12 backend: GPU resource and pipeline creation

#ifdef WICKEDENGINE_BUILD_DX12

namespace Protean::Graphics
{
	using namespace D3D12Internal;

	bool D3D12Device::CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = Protean::Allocator::make_shared<Resource_DX12>();
		internal_state->allocationhandler = allocationhandler;
		buffer->internal_state = internal_state;
		buffer->type = GPUResource::Type_BUFFER;
		buffer->mapped_data = nullptr;
		buffer->mapped_size = 0;
		buffer->desc = *desc;

		HRESULT hr = E_FAIL;

		UINT64 alignedSize = desc->size;
		if (has_flag(desc->bind_flags, BindFlag_CONSTANT_BUFFER))
		{
			alignedSize = align(alignedSize, (UINT64)D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		}

		D3D12_RESOURCE_DESC resourceDesc;
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.Width = alignedSize;
		resourceDesc.Height = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.Alignment = 0;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (has_flag(desc->bind_flags, BindFlag_UNORDERED_ACCESS))
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!has_flag(desc->bind_flags, BindFlag_SHADER_RESOURCE) && !has_flag(desc->misc_flags, ResourceMiscFlag_RAY_TRACING))
		{
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}

		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		if (desc->usage == Usage_READBACK)
		{
			allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
			resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
		else if (desc->usage == Usage_UPLOAD)
		{
			allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
			resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
		}

#ifdef PLATFORM_XBOX
		Protean::Graphics::Xbox::ApplyBufferCreationFlags(*desc, resourceDesc.Flags, allocationDesc.ExtraHeapFlags);
#endif // PLATFORM_XBOX

		if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_BUFFER) ||
			has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS) ||
			has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS))
		{
			// Aliasing memory pool must not be a committed resource because that uses implicit heap which returns nullptr,
			//	thus it cannot be offsetted. This is why we create custom allocation here which will never be committed resource
			//	(since it has no resource)
			D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = device->GetResourceAllocationInfo(0, 1, &resourceDesc);

			// D3D12MA ValidateAllocateMemoryParameters requires this, wasn't always true on Xbox:
			allocationInfo.SizeInBytes = align(allocationInfo.SizeInBytes, (UINT64)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			allocationInfo.Alignment = std::max(allocationInfo.Alignment, (UINT64)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			if (resource_heap_tier >= D3D12_RESOURCE_HEAP_TIER_2)
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_BUFFER))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			}

			hr = dx12_check(allocationhandler->allocator->AllocateMemory(
				&allocationDesc,
				&allocationInfo,
				&internal_state->allocation
			));

			if (allocationDesc.ExtraHeapFlags == D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS || allocationDesc.ExtraHeapFlags == D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES)
			{
				hr = dx12_check(device->CreatePlacedResource(
					internal_state->allocation->GetHeap(),
					internal_state->allocation->GetOffset(),
					&resourceDesc,
					resourceState,
					nullptr,
					PPV_ARGS(internal_state->resource)
				));
			}
		}
		else if (has_flag(desc->misc_flags, ResourceMiscFlag_SPARSE))
		{
			hr = dx12_check(device->CreateReservedResource(
				&resourceDesc,
				resourceState,
				nullptr,
				PPV_ARGS(internal_state->resource)
			));

			buffer->sparse_page_size = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;
		}
		else
		{
			if (alias == nullptr)
			{
				hr = dx12_check(allocationhandler->allocator->CreateResource(
					&allocationDesc,
					&resourceDesc,
					resourceState,
					nullptr,
					&internal_state->allocation,
					PPV_ARGS(internal_state->resource)
				));
			}
			else
			{
				// Aliasing: https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/resource_aliasing.html
				auto alias_internal = to_internal(alias);
				hr = dx12_check(allocationhandler->allocator->CreateAliasingResource(
					alias_internal->allocation.Get(),
					alias_offset,
					&resourceDesc,
					resourceState,
					nullptr,
					PPV_ARGS(internal_state->resource)
				));
			}

		}

		if (!SUCCEEDED(hr))
			return false;

		if (internal_state->resource != nullptr)
		{
			internal_state->gpu_address = internal_state->resource->GetGPUVirtualAddress();
		}

		if (desc->usage == Usage_READBACK)
		{
			hr = dx12_check(internal_state->resource->Map(0, nullptr, &buffer->mapped_data));
			buffer->mapped_size = static_cast<uint32_t>(desc->size);
		}
		else if (desc->usage == Usage_UPLOAD)
		{
			D3D12_RANGE read_range = {};
			hr = dx12_check(internal_state->resource->Map(0, &read_range, &buffer->mapped_data));
			buffer->mapped_size = static_cast<uint32_t>(desc->size);
		}

		// Issue data copy on request:
		if (init_callback != nullptr)
		{
			CopyAllocator::CopyCMD cmd;
			void* mapped_data = nullptr;
			if (desc->usage == Usage_UPLOAD)
			{
				mapped_data = buffer->mapped_data;
			}
			else
			{
				cmd = copyAllocator.allocate(desc->size);
				mapped_data = cmd.uploadbuffer.mapped_data;
			}

			init_callback(mapped_data);

			if (cmd.IsValid())
			{
				cmd.commandList->CopyBufferRegion(
					internal_state->resource.Get(),
					0,
					to_internal(&cmd.uploadbuffer)->resource.Get(),
					0,
					desc->size
				);
				copyAllocator.submit(cmd);
			}
		}


		// Create resource views if needed
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

		if (
			has_flag(desc->bind_flags, BindFlag_UNORDERED_ACCESS) &&
			(
				!has_flag(desc->misc_flags, ResourceMiscFlag_BUFFER_RAW) ||
				has_flag(desc->misc_flags, ResourceMiscFlag_NO_DEFAULT_DESCRIPTORS)
			)
		)
		{
			// Create raw buffer if doesn't exist for ClearUAV:
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
			uav_desc.Buffer.NumElements = uint32_t(desc->size / sizeof(uint32_t));
			internal_state->uav_raw.init(this, uav_desc, internal_state->resource.Get());
		}

		return SUCCEEDED(hr);
	}

	bool D3D12Device::CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = Protean::Allocator::make_shared<Resource_DX12>();
		internal_state->allocationhandler = allocationhandler;
		texture->internal_state = internal_state;
		texture->type = GPUResource::Type_TEXTURE;
		texture->mapped_data = nullptr;
		texture->mapped_size = 0;
		texture->mapped_subresources = nullptr;
		texture->mapped_subresource_count = 0;
		texture->sparse_properties = nullptr;
		texture->desc = *desc;

		texture->desc.mip_levels = GetMipCount(texture->desc);

		HRESULT hr = E_FAIL;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		D3D12_RESOURCE_DESC resourcedesc = _ConvertTextureDesc(texture->desc, casting_fully_typed_formats);

		if (
			has_flag(texture->desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE) ||
			has_flag(texture->desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE_DPB_ONLY)
			)
		{
			// Nvidia GPU will crash if placed resource is used for video decode operation!
			allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;
		}

		D3D12_CLEAR_VALUE optimizedClearValue = {};
		optimizedClearValue.Color[0] = texture->desc.clear.color[0];
		optimizedClearValue.Color[1] = texture->desc.clear.color[1];
		optimizedClearValue.Color[2] = texture->desc.clear.color[2];
		optimizedClearValue.Color[3] = texture->desc.clear.color[3];
		optimizedClearValue.DepthStencil.Depth = texture->desc.clear.depth_stencil.depth;
		optimizedClearValue.DepthStencil.Stencil = texture->desc.clear.depth_stencil.stencil;
		optimizedClearValue.Format = _ConvertFormat(texture->desc.format); // _ConvertFormat again because _ConvertTextureDesc might change it to _TYPELESS
		const bool useClearValue = has_flag(desc->bind_flags, BindFlag_RENDER_TARGET) || has_flag(desc->bind_flags, BindFlag_DEPTH_STENCIL);

		D3D12_RESOURCE_STATES resourceState = _ParseResourceState(texture->desc.layout);

		if (initial_data != nullptr || (resourcedesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS))
		{
			resourceState = D3D12_RESOURCE_STATE_COMMON;
		}

		internal_state->total_size = 0;
		internal_state->footprints.resize(GetTextureSubresourceCount(texture->desc));
		internal_state->rowSizesInBytes.resize(internal_state->footprints.size());
		internal_state->numRows.resize(internal_state->footprints.size());
		device->GetCopyableFootprints(
			&resourcedesc,
			0,
			(UINT)internal_state->footprints.size(),
			0,
			internal_state->footprints.data(),
			internal_state->numRows.data(),
			internal_state->rowSizesInBytes.data(),
			&internal_state->total_size
		);

		if (texture->desc.usage == Usage_READBACK || texture->desc.usage == Usage_UPLOAD)
		{
			resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			resourcedesc.Width = internal_state->total_size;
			resourcedesc.Height = 1;
			resourcedesc.DepthOrArraySize = 1;
			resourcedesc.MipLevels = 1;
			resourcedesc.Format = DXGI_FORMAT_UNKNOWN;
			resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			resourcedesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			if (texture->desc.usage == Usage_READBACK)
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_READBACK;
				resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
			}
			else if(texture->desc.usage == Usage_UPLOAD)
			{
				allocationDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
				resourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
			}
		}

#ifndef PLATFORM_XBOX // Xbox is detected as UMA, but the UMA texture upload path is disabled for now
		if (
			initial_data != nullptr &&
			allocationDesc.HeapType == D3D12_HEAP_TYPE_DEFAULT &&
			CheckCapability(GraphicsDeviceCapability_CACHE_COHERENT_UMA)
			)
		{
			// UMA: custom pool is like DEFAULT heap + CPU Write Combine
			//	It will be used with WriteToSubresource to avoid GPU copy from UPLOAD to DEAFULT
			allocationDesc.CustomPool = allocationhandler->uma_pool.Get();
		}
#endif // PLATFORM_XBOX

		if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_BUFFER) ||
			has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS) ||
			has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS))
		{
			// Aliasing memory pool must not be a committed resource because that uses implicit heap which returns nullptr,
			//	thus it cannot be offsetted. This is why we create custom allocation here which will never be committed resource
			//	(since it has no resource)
			D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = device->GetResourceAllocationInfo(0, 1, &resourcedesc);

			// D3D12MA ValidateAllocateMemoryParameters requires this, wasn't always true on Xbox:
			allocationInfo.SizeInBytes = align(allocationInfo.SizeInBytes, (UINT64)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			allocationInfo.Alignment = std::max(allocationInfo.Alignment, (UINT64)D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			if (resource_heap_tier >= D3D12_RESOURCE_HEAP_TIER_2)
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_BUFFER))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
			}
			else if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS))
			{
				allocationDesc.ExtraHeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
			}

			hr = dx12_check(allocationhandler->allocator->AllocateMemory(
				&allocationDesc,
				&allocationInfo,
				&internal_state->allocation
			));

			hr = dx12_check(device->CreatePlacedResource(
				internal_state->allocation->GetHeap(),
				internal_state->allocation->GetOffset(),
				&resourcedesc,
				resourceState,
				useClearValue ? &optimizedClearValue : nullptr,
				PPV_ARGS(internal_state->resource)
			));
		}
		else if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_SPARSE))
		{
			resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
			hr = dx12_check(device->CreateReservedResource(
				&resourcedesc,
				resourceState,
				useClearValue ? &optimizedClearValue : nullptr,
				PPV_ARGS(internal_state->resource)
			));
			texture->sparse_page_size = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

			UINT num_tiles_for_entire_resource = 0;
			D3D12_PACKED_MIP_INFO packed_mip_info = {};
			D3D12_TILE_SHAPE tile_shape = {};
			UINT num_subresource_tilings = 0;

			device->GetResourceTiling(
				internal_state->resource.Get(),
				&num_tiles_for_entire_resource,
				&packed_mip_info,
				&tile_shape,
				&num_subresource_tilings,
				0,
				nullptr
			);

			SparseTextureProperties& sparse = internal_state->sparse_texture_properties;
			texture->sparse_properties = &sparse;
			sparse.tile_width = tile_shape.WidthInTexels;
			sparse.tile_height = tile_shape.HeightInTexels;
			sparse.tile_depth = tile_shape.DepthInTexels;
			sparse.total_tile_count = num_tiles_for_entire_resource;
			sparse.packed_mip_start = packed_mip_info.NumStandardMips;
			sparse.packed_mip_count = packed_mip_info.NumPackedMips;
			sparse.packed_mip_tile_offset = packed_mip_info.StartTileIndexInOverallResource;
			sparse.packed_mip_tile_count = packed_mip_info.NumTilesForPackedMips;
		}
		else
		{
			if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_SHARED))
			{
				// Dedicated memory
				allocationDesc.Flags = D3D12MA::ALLOCATION_FLAG_COMMITTED;

				// What about D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER and D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER?
				allocationDesc.ExtraHeapFlags |= D3D12_HEAP_FLAG_SHARED;
			}

			if (alias == nullptr)
			{
				hr = dx12_check(allocationhandler->allocator->CreateResource(
					&allocationDesc,
					&resourcedesc,
					resourceState,
					useClearValue ? &optimizedClearValue : nullptr,
					&internal_state->allocation,
					PPV_ARGS(internal_state->resource)
				));
			}
			else
			{
				// Aliasing: https://gpuopen-librariesandsdks.github.io/D3D12MemoryAllocator/html/resource_aliasing.html
				auto alias_internal = to_internal(alias);
				hr = dx12_check(allocationhandler->allocator->CreateAliasingResource(
					alias_internal->allocation.Get(),
					alias_offset,
					&resourcedesc,
					resourceState,
					useClearValue ? &optimizedClearValue : nullptr,
					PPV_ARGS(internal_state->resource)
				));
			}
		}

		if (texture->desc.usage == Usage_READBACK)
		{
			hr = dx12_check(internal_state->resource->Map(0, nullptr, &texture->mapped_data));
		}
		else if(texture->desc.usage == Usage_UPLOAD)
		{
			D3D12_RANGE read_range = {};
			hr = dx12_check(internal_state->resource->Map(0, &read_range, &texture->mapped_data));
		}
		else if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_SHARED))
		{
			hr = dx12_check(allocationhandler->device->CreateSharedHandle(
				internal_state->resource.Get(),
				nullptr,
				GENERIC_ALL,
				nullptr,
				&texture->shared_handle
			));
		}

		if (texture->mapped_data != nullptr)
		{
			texture->mapped_size = internal_state->total_size;
			internal_state->mapped_subresources.resize(internal_state->footprints.size());
			for (size_t i = 0; i < internal_state->footprints.size(); ++i)
			{
				internal_state->mapped_subresources[i].data_ptr = (uint8_t*)texture->mapped_data + internal_state->footprints[i].Offset;
				internal_state->mapped_subresources[i].row_pitch = internal_state->footprints[i].Footprint.RowPitch;
				internal_state->mapped_subresources[i].slice_pitch = internal_state->footprints[i].Footprint.RowPitch * internal_state->footprints[i].Footprint.Height;
			}
			texture->mapped_subresources = internal_state->mapped_subresources.data();
			texture->mapped_subresource_count = internal_state->mapped_subresources.size();
		}

		// Issue data copy on request:
		if (initial_data != nullptr)
		{
			if (allocationDesc.CustomPool != nullptr && allocationDesc.CustomPool == allocationhandler->uma_pool.Get())
			{
				// UMA direct texture write path:
				for (size_t i = 0; i < internal_state->footprints.size(); ++i)
				{
					const SubresourceData& data = initial_data[i];

					hr = dx12_check(internal_state->resource->WriteToSubresource(
						(UINT)i,
						nullptr,
						data.data_ptr,
						data.row_pitch,
						data.slice_pitch
					));

				}
			}
			else
			{
				// Discrete GPU texture upload path:
				CopyAllocator::CopyCMD cmd;
				void* mapped_data = nullptr;
				if (texture->mapped_data == nullptr)
				{
					cmd = copyAllocator.allocate(internal_state->total_size);
					mapped_data = cmd.uploadbuffer.mapped_data;
				}
				else
				{
					mapped_data = texture->mapped_data;
				}

				for (size_t i = 0; i < internal_state->footprints.size(); ++i)
				{
					D3D12_SUBRESOURCE_DATA data = _ConvertSubresourceData(initial_data[i]);

					if (internal_state->rowSizesInBytes[i] > (SIZE_T)-1)
						continue;
					D3D12_MEMCPY_DEST DestData = {};
					DestData.pData = (void*)((UINT64)mapped_data + internal_state->footprints[i].Offset);
					DestData.RowPitch = (SIZE_T)internal_state->footprints[i].Footprint.RowPitch;
					DestData.SlicePitch = (SIZE_T)internal_state->footprints[i].Footprint.RowPitch * (SIZE_T)internal_state->numRows[i];
					MemcpySubresource(&DestData, &data, (SIZE_T)internal_state->rowSizesInBytes[i], internal_state->numRows[i], internal_state->footprints[i].Footprint.Depth);

					if (cmd.IsValid())
					{
						CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state->resource.Get(), UINT(i));
						CD3DX12_TEXTURE_COPY_LOCATION Src(to_internal(&cmd.uploadbuffer)->resource.Get(), internal_state->footprints[i]);
						cmd.commandList->CopyTextureRegion(
							&Dst,
							0,
							0,
							0,
							&Src,
							nullptr
						);
					}
				}

				if (cmd.IsValid())
				{
					copyAllocator.submit(cmd);
				}
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

		return SUCCEEDED(hr);
	}

	bool D3D12Device::CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader, const char* entrypoint) const
	{
		auto internal_state = Protean::Allocator::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		shader->internal_state = internal_state;

		internal_state->shadercode.resize(shadercode_size);
		std::memcpy(internal_state->shadercode.data(), shadercode, shadercode_size);
		shader->stage = stage;

		HRESULT hr = dx12_check((internal_state->shadercode.empty() ? E_FAIL : S_OK));

		hr = dx12_check(D3D12CreateVersionedRootSignatureDeserializer(
			internal_state->shadercode.data(),
			internal_state->shadercode.size(),
			PPV_ARGS(internal_state->rootsig_deserializer)
		));
		if (SUCCEEDED(hr))
		{
			hr = internal_state->rootsig_deserializer->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_1, &internal_state->rootsig_desc);
			if (SUCCEEDED(hr))
			{
				assert(internal_state->rootsig_desc->Version == D3D_ROOT_SIGNATURE_VERSION_1_1);

				hr = dx12_check(device->CreateRootSignature(
					0,
					internal_state->shadercode.data(),
					internal_state->shadercode.size(),
					PPV_ARGS(internal_state->rootSignature)
				));
			}
		}

		if (stage == ShaderStage_CS || stage == ShaderStage_LIB)
		{
			assert(internal_state->rootSignature != nullptr);
			assert(internal_state->rootsig_desc != nullptr);
			internal_state->rootsig_optimizer.init(*internal_state->rootsig_desc);
		}

		if (stage == ShaderStage_CS)
		{
			struct PSO_STREAM
			{
				CD3DX12_PIPELINE_STATE_STREAM_CS CS;
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE ROOTSIG;
			} stream;

			stream.CS = { internal_state->shadercode.data(), internal_state->shadercode.size() };
			stream.ROOTSIG = internal_state->rootSignature.Get();

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
			streamDesc.pPipelineStateSubobjectStream = &stream;
			streamDesc.SizeInBytes = sizeof(stream);

			hr = dx12_check(device->CreatePipelineState(&streamDesc, PPV_ARGS(internal_state->resource)));
		}

		if (stage == ShaderStage_VS)
		{
			std::scoped_lock lck(multidraw_signature_locker);
			MultiDrawSignature& cached = multidraw_signatures[internal_state->rootSignature.Get()];

			if (cached.drawInstancedIndirectCountCommandSignature)
			{
				internal_state->drawInstancedIndirectCountCommandSignature = cached.drawInstancedIndirectCountCommandSignature;
			}
			else
			{
				D3D12_COMMAND_SIGNATURE_DESC cmd_desc = {};
				D3D12_INDIRECT_ARGUMENT_DESC drawInstancedCountArgs[2] = {};
				drawInstancedCountArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
				drawInstancedCountArgs[0].Constant.RootParameterIndex = internal_state->rootsig_optimizer.PUSH;
				drawInstancedCountArgs[0].Constant.Num32BitValuesToSet = 1;
				drawInstancedCountArgs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;
				cmd_desc.ByteStride = sizeof(uint32_t) + sizeof(D3D12_DRAW_ARGUMENTS);
				cmd_desc.NumArgumentDescs = arraysize(drawInstancedCountArgs);
				cmd_desc.pArgumentDescs = drawInstancedCountArgs;
				dx12_check(device->CreateCommandSignature(&cmd_desc, internal_state->rootSignature.Get(), PPV_ARGS(internal_state->drawInstancedIndirectCountCommandSignature)));
			}

			if (cached.drawIndexedInstancedIndirectCountCommandSignature)
			{
				internal_state->drawIndexedInstancedIndirectCountCommandSignature = cached.drawIndexedInstancedIndirectCountCommandSignature;
			}
			else
			{
				D3D12_COMMAND_SIGNATURE_DESC cmd_desc = {};
				D3D12_INDIRECT_ARGUMENT_DESC drawIndexedInstancedCountArgs[2] = {};
				drawIndexedInstancedCountArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
				drawIndexedInstancedCountArgs[0].Constant.RootParameterIndex = internal_state->rootsig_optimizer.PUSH;
				drawIndexedInstancedCountArgs[0].Constant.Num32BitValuesToSet = 1;
				drawIndexedInstancedCountArgs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
				cmd_desc.ByteStride = sizeof(uint32_t) + sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
				cmd_desc.NumArgumentDescs = arraysize(drawIndexedInstancedCountArgs);
				cmd_desc.pArgumentDescs = drawIndexedInstancedCountArgs;
				dx12_check(device->CreateCommandSignature(&cmd_desc, internal_state->rootSignature.Get(), PPV_ARGS(internal_state->drawIndexedInstancedIndirectCountCommandSignature)));
			}
		}
		else if (stage == ShaderStage_MS)
		{
			std::scoped_lock lck(multidraw_signature_locker);
			MultiDrawSignature& cached = multidraw_signatures[internal_state->rootSignature.Get()];

			if (cached.dispatchMeshIndirectCountCommandSignature)
			{
				internal_state->dispatchMeshIndirectCountCommandSignature = cached.dispatchMeshIndirectCountCommandSignature;
			}
			else
			{
				D3D12_COMMAND_SIGNATURE_DESC cmd_desc = {};
				D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshCountArgs[2] = {};
				dispatchMeshCountArgs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
				dispatchMeshCountArgs[0].Constant.RootParameterIndex = internal_state->rootsig_optimizer.PUSH;
				dispatchMeshCountArgs[0].Constant.Num32BitValuesToSet = 1;
#ifdef PLATFORM_XBOX
				Protean::Graphics::Xbox::FillDispatchMeshIndirectArgumentDesc(dispatchMeshCountArgs[1], cmd_desc);
#else
				dispatchMeshCountArgs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;
				cmd_desc.ByteStride = sizeof(D3D12_DISPATCH_MESH_ARGUMENTS);
#endif // PLATFORM_XBOX
				cmd_desc.ByteStride += sizeof(uint32_t);
				cmd_desc.NumArgumentDescs = arraysize(dispatchMeshCountArgs);
				cmd_desc.pArgumentDescs = dispatchMeshCountArgs;
				dx12_check(device->CreateCommandSignature(&cmd_desc, internal_state->rootSignature.Get(), PPV_ARGS(internal_state->dispatchMeshIndirectCountCommandSignature)));
			}
		}

		return SUCCEEDED(hr);
	}

	bool D3D12Device::CreateSampler(const SamplerDesc* desc, Sampler* sampler) const
	{
		auto internal_state = Protean::Allocator::make_shared<Sampler_DX12>();
		internal_state->allocationhandler = allocationhandler;
		sampler->internal_state = internal_state;

		D3D12_SAMPLER_DESC samplerdesc;
		samplerdesc.Filter = _ConvertFilter(desc->filter);
		samplerdesc.AddressU = _ConvertTextureAddressMode(desc->address_u);
		samplerdesc.AddressV = _ConvertTextureAddressMode(desc->address_v);
		samplerdesc.AddressW = _ConvertTextureAddressMode(desc->address_w);
		samplerdesc.MipLODBias = desc->mip_lod_bias;
		samplerdesc.MaxAnisotropy = desc->max_anisotropy;
		samplerdesc.ComparisonFunc = _ConvertComparisonFunc(desc->comparison_func);
		switch (desc->border_color)
		{
		case SamplerBorderColor_OPAQUE_BLACK:
			samplerdesc.BorderColor[0] = 0.0f;
			samplerdesc.BorderColor[1] = 0.0f;
			samplerdesc.BorderColor[2] = 0.0f;
			samplerdesc.BorderColor[3] = 1.0f;
			break;

		case SamplerBorderColor_OPAQUE_WHITE:
			samplerdesc.BorderColor[0] = 1.0f;
			samplerdesc.BorderColor[1] = 1.0f;
			samplerdesc.BorderColor[2] = 1.0f;
			samplerdesc.BorderColor[3] = 1.0f;
			break;

		default:
			samplerdesc.BorderColor[0] = 0.0f;
			samplerdesc.BorderColor[1] = 0.0f;
			samplerdesc.BorderColor[2] = 0.0f;
			samplerdesc.BorderColor[3] = 0.0f;
			break;
		}
		samplerdesc.MinLOD = desc->min_lod;
		samplerdesc.MaxLOD = desc->max_lod;

		sampler->desc = *desc;

		internal_state->descriptor.init(this, samplerdesc);

		return true;
	}

	bool D3D12Device::CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const
	{
		auto internal_state = Protean::Allocator::make_shared<QueryHeap_DX12>();
		internal_state->allocationhandler = allocationhandler;
		queryheap->internal_state = internal_state;
		queryheap->desc = *desc;

		D3D12_QUERY_HEAP_DESC queryheapdesc = {};
		queryheapdesc.Count = desc->query_count;

		switch (desc->type)
		{
		default:
		case GpuQueryType_TIMESTAMP:
			queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
			break;
		case GpuQueryType_OCCLUSION:
		case GpuQueryType_OCCLUSION_BINARY:
			queryheapdesc.Type = D3D12_QUERY_HEAP_TYPE_OCCLUSION;
			break;
		}

		HRESULT hr = dx12_check(allocationhandler->device->CreateQueryHeap(&queryheapdesc, PPV_ARGS(internal_state->heap)));

		return SUCCEEDED(hr);
	}

	bool D3D12Device::CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info) const
	{
		auto internal_state = Protean::Allocator::make_shared<PipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;
		pso->desc = *desc;

		const RasterizerState& rs_desc = pso->desc.rs != nullptr ? *pso->desc.rs : default_rasterizerstate;
		const DepthStencilState& dss_desc = pso->desc.dss != nullptr ? *pso->desc.dss : default_depthstencilstate;
		const BlendState& bs_desc = pso->desc.bs != nullptr ? *pso->desc.bs : default_blendstate;

		auto& stream = internal_state->stream;
		//stream.stream1.Flags |= D3D12_PIPELINE_STATE_FLAG_DYNAMIC_DEPTH_BIAS; // doesn't work on windows 10
		if (pso->desc.vs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.vs);
			stream.stream1.VS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				internal_state->rootsig_desc_lifetime_extender = pso->desc.vs->internal_state;
				internal_state->drawInstancedIndirectCountCommandSignature = shader_internal->drawInstancedIndirectCountCommandSignature;
				internal_state->drawIndexedInstancedIndirectCountCommandSignature = shader_internal->drawIndexedInstancedIndirectCountCommandSignature;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.hs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.hs);
			stream.stream1.HS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				internal_state->rootsig_desc_lifetime_extender = pso->desc.hs->internal_state;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.ds != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ds);
			stream.stream1.DS = { shader_internal->shadercode.data(),shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				internal_state->rootsig_desc_lifetime_extender = pso->desc.ds->internal_state;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.gs != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.gs);
			stream.stream1.GS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				internal_state->rootsig_desc_lifetime_extender = pso->desc.gs->internal_state;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.ps != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ps);
			stream.stream1.PS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				internal_state->rootsig_desc_lifetime_extender = pso->desc.ps->internal_state;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}

		if (pso->desc.ms != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.ms);
			stream.stream2.MS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				internal_state->rootsig_desc_lifetime_extender = pso->desc.ms->internal_state;
				internal_state->dispatchMeshIndirectCountCommandSignature = shader_internal->dispatchMeshIndirectCountCommandSignature;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}
		if (pso->desc.as != nullptr)
		{
			auto shader_internal = to_internal(pso->desc.as);
			stream.stream2.AS = { shader_internal->shadercode.data(), shader_internal->shadercode.size() };
			if (internal_state->rootSignature == nullptr)
			{
				internal_state->rootSignature = shader_internal->rootSignature;
				internal_state->rootsig_desc = shader_internal->rootsig_desc;
				internal_state->rootsig_desc_lifetime_extender = pso->desc.as->internal_state;
				stream.stream1.ROOTSIG = internal_state->rootSignature.Get();
			}
		}

		assert(internal_state->rootSignature != nullptr);
		assert(internal_state->rootsig_desc != nullptr);
		internal_state->rootsig_optimizer.init(*internal_state->rootsig_desc);

		CD3DX12_RASTERIZER_DESC rs = {};
		rs.FillMode = _ConvertFillMode(rs_desc.fill_mode);
		rs.CullMode = _ConvertCullMode(rs_desc.cull_mode);
		rs.FrontCounterClockwise = rs_desc.front_counter_clockwise;
		rs.DepthBias = rs_desc.depth_bias;
		rs.DepthBiasClamp = rs_desc.depth_bias_clamp;
		rs.SlopeScaledDepthBias = rs_desc.slope_scaled_depth_bias;
		rs.DepthClipEnable = rs_desc.depth_clip_enable;
		rs.MultisampleEnable = rs_desc.multisample_enable;
		rs.AntialiasedLineEnable = rs_desc.antialiased_line_enable;
		rs.ConservativeRaster = ((CheckCapability(GraphicsDeviceCapability_CONSERVATIVE_RASTERIZATION) && rs_desc.conservative_rasterization_enable) ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);
		rs.ForcedSampleCount = rs_desc.forced_sample_count;
		stream.stream1.RS = rs;

		CD3DX12_DEPTH_STENCIL_DESC1 dss = {};
		dss.DepthEnable = dss_desc.depth_enable;
		dss.DepthWriteMask = _ConvertDepthWriteMask(dss_desc.depth_write_mask);
		dss.DepthFunc = _ConvertComparisonFunc(dss_desc.depth_func);
		dss.StencilEnable = dss_desc.stencil_enable;
		dss.StencilReadMask = dss_desc.stencil_read_mask;
		dss.StencilWriteMask = dss_desc.stencil_write_mask;
		dss.FrontFace.StencilDepthFailOp = _ConvertStencilOp(dss_desc.front_face.stencil_depth_fail_op);
		dss.FrontFace.StencilFailOp = _ConvertStencilOp(dss_desc.front_face.stencil_fail_op);
		dss.FrontFace.StencilFunc = _ConvertComparisonFunc(dss_desc.front_face.stencil_func);
		dss.FrontFace.StencilPassOp = _ConvertStencilOp(dss_desc.front_face.stencil_pass_op);
		dss.BackFace.StencilDepthFailOp = _ConvertStencilOp(dss_desc.back_face.stencil_depth_fail_op);
		dss.BackFace.StencilFailOp = _ConvertStencilOp(dss_desc.back_face.stencil_fail_op);
		dss.BackFace.StencilFunc = _ConvertComparisonFunc(dss_desc.back_face.stencil_func);
		dss.BackFace.StencilPassOp = _ConvertStencilOp(dss_desc.back_face.stencil_pass_op);
		if (CheckCapability(GraphicsDeviceCapability_DEPTH_BOUNDS_TEST))
		{
			dss.DepthBoundsTestEnable = dss_desc.depth_bounds_test_enable;
		}
		else
		{
			dss.DepthBoundsTestEnable = FALSE;
		}
		stream.stream1.DSS = dss;

		CD3DX12_BLEND_DESC bd = {};
		bd.AlphaToCoverageEnable = bs_desc.alpha_to_coverage_enable;
		bd.IndependentBlendEnable = bs_desc.independent_blend_enable;
		for (int i = 0; i < 8; ++i)
		{
			bd.RenderTarget[i].BlendEnable = bs_desc.render_target[i].blend_enable;
			bd.RenderTarget[i].SrcBlend = _ConvertBlend(bs_desc.render_target[i].src_blend);
			bd.RenderTarget[i].DestBlend = _ConvertBlend(bs_desc.render_target[i].dest_blend);
			bd.RenderTarget[i].BlendOp = _ConvertBlendOp(bs_desc.render_target[i].blend_op);
			bd.RenderTarget[i].SrcBlendAlpha = _ConvertAlphaBlend(bs_desc.render_target[i].src_blend_alpha);
			bd.RenderTarget[i].DestBlendAlpha = _ConvertAlphaBlend(bs_desc.render_target[i].dest_blend_alpha);
			bd.RenderTarget[i].BlendOpAlpha = _ConvertBlendOp(bs_desc.render_target[i].blend_op_alpha);
			bd.RenderTarget[i].RenderTargetWriteMask = _ParseColorWriteMask(bs_desc.render_target[i].render_target_write_mask);
		}
		stream.stream1.BD = bd;

		D3D12_INPUT_ELEMENT_DESC elements[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {};
		D3D12_INPUT_LAYOUT_DESC il = {};
		if (pso->desc.il != nullptr)
		{
			il.NumElements = std::min((uint32_t)pso->desc.il->elements.size(), (uint32_t)D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT);
			for (uint32_t i = 0; i < il.NumElements; ++i)
			{
				elements[i].SemanticName = pso->desc.il->elements[i].semantic_name.c_str();
				elements[i].SemanticIndex = pso->desc.il->elements[i].semantic_index;
				elements[i].Format = _ConvertFormat(pso->desc.il->elements[i].format);
				elements[i].InputSlot = pso->desc.il->elements[i].input_slot;
				elements[i].AlignedByteOffset = pso->desc.il->elements[i].aligned_byte_offset;
				if (elements[i].AlignedByteOffset == InputLayout::APPEND_ALIGNED_ELEMENT)
					elements[i].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
				elements[i].InputSlotClass = _ConvertInputClassification(pso->desc.il->elements[i].input_slot_class);
				elements[i].InstanceDataStepRate = 0;
				if (elements[i].InputSlotClass == D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA)
				{
					elements[i].InstanceDataStepRate = 1;
				}
			}
			il.pInputElementDescs = elements;
		}
		stream.stream1.IL = il;

		stream.stream1.SampleMask = pso->desc.sample_mask;

		internal_state->primitiveTopology = _ConvertPrimitiveTopology(desc->pt, desc->patch_control_points);
		switch (pso->desc.pt)
		{
		case PrimitiveTopology_POINTLIST:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
			break;
		case PrimitiveTopology_LINELIST:
		case PrimitiveTopology_LINESTRIP:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
			break;
		case PrimitiveTopology_TRIANGLELIST:
		case PrimitiveTopology_TRIANGLESTRIP:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			break;
		case PrimitiveTopology_PATCHLIST:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
			break;
		default:
			stream.stream1.PT = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
			break;
		}

		stream.stream1.STRIP = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;

		if (renderpass_info != nullptr)
		{
			DXGI_FORMAT DSFormat = _ConvertFormat(renderpass_info->ds_format);
			D3D12_RT_FORMAT_ARRAY formats = {};
			formats.NumRenderTargets = renderpass_info->rt_count;
			for (uint32_t i = 0; i < renderpass_info->rt_count; ++i)
			{
				formats.RTFormats[i] = _ConvertFormat(renderpass_info->rt_formats[i]);
			}
			DXGI_SAMPLE_DESC sampleDesc = {};
			sampleDesc.Count = renderpass_info->sample_count;
			sampleDesc.Quality = 0;

			stream.stream1.DSFormat = DSFormat;
			stream.stream1.Formats = formats;
			stream.stream1.SampleDesc = sampleDesc;

			D3D12_PIPELINE_STATE_STREAM_DESC streamDesc = {};
			streamDesc.pPipelineStateSubobjectStream = &stream;
			streamDesc.SizeInBytes = sizeof(stream.stream1);
			if (CheckCapability(GraphicsDeviceCapability_MESH_SHADER))
			{
				streamDesc.SizeInBytes += sizeof(stream.stream2);
			}

			dx12_check(device->CreatePipelineState(&streamDesc, PPV_ARGS(internal_state->resource)));
		}

		return true;
	}

	bool D3D12Device::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const
	{
		auto internal_state = Protean::Allocator::make_shared<BVH_DX12>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::Type_RAYTRACING_ACCELERATION_STRUCTURE;
		bvh->desc = *desc;
		bvh->size = 0;

		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD;
		}
		if (desc->flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			internal_state->desc.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_MINIMIZE_MEMORY;
		}


		switch (desc->type)
		{
		case RaytracingAccelerationStructureDesc::Type_BOTTOMLEVEL:
		{
			internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
			internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

			for (auto& x : desc->bottom_level.geometries)
			{
				auto& geometry = internal_state->geometries.emplace_back();
				geometry = {};

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_TRIANGLES)
				{
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
					geometry.Triangles.VertexBuffer.StartAddress = to_internal(&x.triangles.vertex_buffer)->gpu_address + (D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertex_byte_offset;
					geometry.Triangles.VertexBuffer.StrideInBytes = (UINT64)x.triangles.vertex_stride;
					geometry.Triangles.VertexCount = x.triangles.vertex_count;
					geometry.Triangles.VertexFormat = _ConvertFormat(x.triangles.vertex_format);
					geometry.Triangles.IndexFormat = (x.triangles.index_format == IndexBufferFormat_UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
					geometry.Triangles.IndexBuffer = to_internal(&x.triangles.index_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.index_offset * (x.triangles.index_format == IndexBufferFormat_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));
					geometry.Triangles.IndexCount = x.triangles.index_count;

					if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.Triangles.Transform3x4 = to_internal(&x.triangles.transform_3x4_buffer)->gpu_address +
							(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform_3x4_buffer_offset;
					}
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_PROCEDURAL_AABBS)
				{
					geometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
					geometry.AABBs.AABBs.StartAddress = to_internal(&x.aabbs.aabb_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
					geometry.AABBs.AABBs.StrideInBytes = (UINT64)x.aabbs.stride;
					geometry.AABBs.AABBCount = x.aabbs.count;
				}
			}

			internal_state->desc.pGeometryDescs = internal_state->geometries.data();
			internal_state->desc.NumDescs = (UINT)internal_state->geometries.size();
		}
		break;
		case RaytracingAccelerationStructureDesc::Type_TOPLEVEL:
		{
			internal_state->desc.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
			internal_state->desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

			internal_state->desc.InstanceDescs = to_internal(&desc->top_level.instance_buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->top_level.offset;
			internal_state->desc.NumDescs = (UINT)desc->top_level.count;
		}
		break;
		}

		device->GetRaytracingAccelerationStructurePrebuildInfo(&internal_state->desc, &internal_state->info);


		UINT64 alignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
		UINT64 alignedSize = align(internal_state->info.ResultDataMaxSizeInBytes, alignment);

		D3D12_RESOURCE_DESC resourcedesc;
		resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourcedesc.Format = DXGI_FORMAT_UNKNOWN;
		resourcedesc.Width = (UINT64)alignedSize;
		resourcedesc.Height = 1;
		resourcedesc.MipLevels = 1;
		resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourcedesc.DepthOrArraySize = 1;
		resourcedesc.Alignment = 0;
		resourcedesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		resourcedesc.SampleDesc.Count = 1;
		resourcedesc.SampleDesc.Quality = 0;

		D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

		D3D12MA::ALLOCATION_DESC allocationDesc = {};
		allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

		dx12_check(allocationhandler->allocator->CreateResource(
			&allocationDesc,
			&resourcedesc,
			resourceState,
			nullptr,
			&internal_state->allocation,
			PPV_ARGS(internal_state->resource)
		));


		internal_state->gpu_address = internal_state->resource->GetGPUVirtualAddress();

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
		srv_desc.RaytracingAccelerationStructure.Location = internal_state->gpu_address;

		if (desc->type == RaytracingAccelerationStructureDesc::Type_TOPLEVEL)
		{
			internal_state->srv.init(this, srv_desc, nullptr);
		}

		GPUBufferDesc scratch_desc;
		scratch_desc.size = (uint32_t)std::max(internal_state->info.ScratchDataSizeInBytes, internal_state->info.UpdateScratchDataSizeInBytes);

		bvh->size = alignedSize + scratch_desc.size;

		return CreateBuffer(&scratch_desc, nullptr, &internal_state->scratch);
	}

	bool D3D12Device::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const
	{
		auto internal_state = Protean::Allocator::make_shared<RTPipelineState_DX12>();
		internal_state->allocationhandler = allocationhandler;
		rtpso->internal_state = internal_state;
		rtpso->desc = *desc;

		D3D12_STATE_OBJECT_DESC stateobjectdesc = {};
		stateobjectdesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

		Protean::vector<D3D12_STATE_SUBOBJECT> subobjects;

		D3D12_RAYTRACING_PIPELINE_CONFIG pipeline_config = {};
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
			pipeline_config.MaxTraceRecursionDepth = desc->max_trace_recursion_depth;
			subobject.pDesc = &pipeline_config;
		}

		D3D12_RAYTRACING_SHADER_CONFIG shader_config = {};
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
			shader_config.MaxAttributeSizeInBytes = desc->max_attribute_size_in_bytes;
			shader_config.MaxPayloadSizeInBytes = desc->max_payload_size_in_bytes;
			subobject.pDesc = &shader_config;
		}

		D3D12_GLOBAL_ROOT_SIGNATURE global_rootsig = {};
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			auto shader_internal = to_internal(desc->shader_libraries.front().shader);
			global_rootsig.pGlobalRootSignature = shader_internal->rootSignature.Get();
			subobject.pDesc = &global_rootsig;
		}

		internal_state->exports.reserve(desc->shader_libraries.size());
		internal_state->library_descs.reserve(desc->shader_libraries.size());
		for(auto& x : desc->shader_libraries)
		{
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
			auto& library_desc = internal_state->library_descs.emplace_back();
			library_desc = {};
			auto shader_internal = to_internal(x.shader);
			library_desc.DXILLibrary.pShaderBytecode = shader_internal->shadercode.data();
			library_desc.DXILLibrary.BytecodeLength = shader_internal->shadercode.size();
			library_desc.NumExports = 1;

			D3D12_EXPORT_DESC& export_desc = internal_state->exports.emplace_back();
			Protean::Helper::StringConvert(x.function_name, internal_state->export_strings.emplace_back());
			export_desc.Name = internal_state->export_strings.back().c_str();
			library_desc.pExports = &export_desc;

			subobject.pDesc = &library_desc;
		}

		internal_state->hitgroup_descs.reserve(desc->hit_groups.size());
		for (auto& x : desc->hit_groups)
		{
			Protean::Helper::StringConvert(x.name, internal_state->group_strings.emplace_back());

			if (x.type == ShaderHitGroup::Type_GENERAL)
				continue;
			auto& subobject = subobjects.emplace_back();
			subobject = {};
			subobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			auto& hitgroup_desc = internal_state->hitgroup_descs.emplace_back();
			hitgroup_desc = {};
			switch (x.type)
			{
			default:
			case ShaderHitGroup::Type_TRIANGLES:
				hitgroup_desc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
				break;
			case ShaderHitGroup::Type_PROCEDURAL:
				hitgroup_desc.Type = D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE;
				break;
			}
			if (!x.name.empty())
			{
				hitgroup_desc.HitGroupExport = internal_state->group_strings.back().c_str();
			}
			if (x.closest_hit_shader != ~0)
			{
				hitgroup_desc.ClosestHitShaderImport = internal_state->exports[x.closest_hit_shader].Name;
			}
			if (x.any_hit_shader != ~0)
			{
				hitgroup_desc.AnyHitShaderImport = internal_state->exports[x.any_hit_shader].Name;
			}
			if (x.intersection_shader != ~0)
			{
				hitgroup_desc.IntersectionShaderImport = internal_state->exports[x.intersection_shader].Name;
			}
			subobject.pDesc = &hitgroup_desc;
		}

		stateobjectdesc.NumSubobjects = (UINT)subobjects.size();
		stateobjectdesc.pSubobjects = subobjects.data();

		dx12_check(device->CreateStateObject(&stateobjectdesc, PPV_ARGS(internal_state->resource)));

		HRESULT hr = dx12_check(internal_state->resource.As(&internal_state->stateObjectProperties));

		return SUCCEEDED(hr);
	}

	bool D3D12Device::CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const
	{
		if (video_device == nullptr)
			return false;

		D3D12_VIDEO_DECODER_DESC decoder_desc = {};
		switch (desc->profile)
		{
		case VideoProfile_H264:
			if (!CheckCapability(GraphicsDeviceCapability_VIDEO_DECODE_H264))
				return false;
			decoder_desc.Configuration.DecodeProfile = D3D12_VIDEO_DECODE_PROFILE_H264;
			decoder_desc.Configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
			break;
		case VideoProfile_H265:
			if (!CheckCapability(GraphicsDeviceCapability_VIDEO_DECODE_H265))
				return false;
			decoder_desc.Configuration.DecodeProfile = D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN;
			decoder_desc.Configuration.InterlaceType = D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE_NONE;
			break;
		default:
			assert(0); // not implemented
			return false;
		}

		D3D12_FEATURE_DATA_VIDEO_DECODE_FORMAT_COUNT video_decode_format_count = {};
		video_decode_format_count.Configuration = decoder_desc.Configuration;
		dx12_check(video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_FORMAT_COUNT, &video_decode_format_count, sizeof(video_decode_format_count)));
		Protean::vector<DXGI_FORMAT> formats(video_decode_format_count.FormatCount);
		D3D12_FEATURE_DATA_VIDEO_DECODE_FORMATS video_decode_formats = {};
		video_decode_formats.Configuration = decoder_desc.Configuration;
		video_decode_formats.FormatCount = video_decode_format_count.FormatCount;
		video_decode_formats.pOutputFormats = formats.data();
		dx12_check(video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_FORMATS, &video_decode_formats, sizeof(video_decode_formats)));
		D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT video_decode_support = {};
		video_decode_support.Configuration = decoder_desc.Configuration;
		video_decode_support.DecodeFormat = _ConvertFormat(desc->format);
		bool format_valid = false;
		for (auto& x : formats)
		{
			if (x == video_decode_support.DecodeFormat)
			{
				format_valid = true;
				break;
			}
		}
		if (!format_valid)
			return false;
		video_decode_support.Width = desc->width;
		video_decode_support.Height = desc->height;
		video_decode_support.BitRate = desc->bit_rate;
		video_decode_support.FrameRate = { 0, 1 };
		dx12_check(video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT, &video_decode_support, sizeof(video_decode_support)));

		if (video_decode_support.DecodeTier < D3D12_VIDEO_DECODE_TIER_1)
			return false;

		video_decoder->support = {};
		if (video_decode_support.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_REFERENCE_ONLY_ALLOCATIONS_REQUIRED)
		{
			video_decoder->support |= VideoDecoderSupportFlags_DPB_AND_OUTPUT_DISTINCT;
		}
		else
		{
			video_decoder->support |= VideoDecoderSupportFlags_DPB_AND_OUTPUT_COINCIDE;
		}
		if (video_decode_support.DecodeTier >= D3D12_VIDEO_DECODE_TIER_2)
		{
			video_decoder->support |= VideoDecoderSupportFlags_DPB_INDIVIDUAL_TEXTURES_SUPPORTED;
		}

		auto internal_state = Protean::Allocator::make_shared<VideoDecoder_DX12>();
		internal_state->allocationhandler = allocationhandler;
		video_decoder->internal_state = internal_state;
		video_decoder->desc = *desc;

		D3D12_VIDEO_DECODER_HEAP_DESC heap_desc = {};
		heap_desc.Configuration = decoder_desc.Configuration;
		heap_desc.DecodeWidth = video_decode_support.Width;
		heap_desc.DecodeHeight = video_decode_support.Height;
		heap_desc.Format = video_decode_support.DecodeFormat;
		heap_desc.FrameRate = { 0,1 };
		heap_desc.BitRate = 0;
		heap_desc.MaxDecodePictureBufferCount = desc->num_dpb_slots;

		if (video_decode_support.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_HEIGHT_ALIGNMENT_MULTIPLE_32_REQUIRED)
		{
			heap_desc.DecodeWidth = align(video_decode_support.Width, 32u);
			heap_desc.DecodeHeight = align(video_decode_support.Height, 32u);
		}

#if 0
		D3D12_FEATURE_DATA_VIDEO_DECODER_HEAP_SIZE video_decoder_heap_size = {};
		video_decoder_heap_size.VideoDecoderHeapDesc = heap_desc;
		dx12_check(video_device->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODER_HEAP_SIZE, &video_decoder_heap_size, sizeof(video_decoder_heap_size)));
#endif

		dx12_check(video_device->CreateVideoDecoderHeap(&heap_desc, PPV_ARGS(internal_state->decoder_heap)));
		HRESULT hr = dx12_check(video_device->CreateVideoDecoder(&decoder_desc, PPV_ARGS(internal_state->decoder)));
		return SUCCEEDED(hr);
	}

	int D3D12Device::CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change, const ImageAspect* aspect, const Swizzle* swizzle, float min_lod_clamp) const
	{
		auto internal_state = to_internal(texture);

		Format format = texture->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		UINT plane = 0;
		if (aspect != nullptr)
		{
			plane = _ImageAspectToPlane(*aspect);
		}

		switch (type)
		{
		case SubresourceType_SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = _ConvertSwizzle(swizzle == nullptr ? texture->desc.swizzle : *swizzle);

			// Try to resolve resource format:
			switch (format)
			{
			case Format_D16_UNORM:
				srv_desc.Format = DXGI_FORMAT_R16_UNORM;
				break;
			case Format_D32_FLOAT:
				srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
				break;
			case Format_D24_UNORM_S8_UINT:
				srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				break;
			case Format_D32_FLOAT_S8X24_UINT:
				srv_desc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				break;
			case Format_NV12:
				srv_desc.Format = DXGI_FORMAT_R8_UNORM;
				break;
			default:
				srv_desc.Format = _ConvertFormat(format);
				break;
			}

			if (texture->desc.type == TextureDesc::Type_TEXTURE_1D)
			{
				if (texture->desc.array_size > 1)
				{
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
					srv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					srv_desc.Texture1DArray.ArraySize = sliceCount;
					srv_desc.Texture1DArray.MostDetailedMip = firstMip;
					srv_desc.Texture1DArray.MipLevels = mipCount;
				}
				else
				{
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
					srv_desc.Texture1D.MostDetailedMip = firstMip;
					srv_desc.Texture1D.MipLevels = mipCount;
				}
			}
			else if (texture->desc.type == TextureDesc::Type_TEXTURE_2D)
			{
				if (texture->desc.array_size > 1)
				{
					if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_TEXTURECUBE))
					{
						if (texture->desc.array_size > 6)
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
							srv_desc.TextureCubeArray.First2DArrayFace = firstSlice;
							srv_desc.TextureCubeArray.NumCubes = std::min(texture->desc.array_size, sliceCount) / 6;
							srv_desc.TextureCubeArray.MostDetailedMip = firstMip;
							srv_desc.TextureCubeArray.MipLevels = mipCount;
							srv_desc.TextureCubeArray.ResourceMinLODClamp = min_lod_clamp;
						}
						else
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
							srv_desc.TextureCube.MostDetailedMip = firstMip;
							srv_desc.TextureCube.MipLevels = mipCount;
							srv_desc.TextureCube.ResourceMinLODClamp = min_lod_clamp;
						}
					}
					else
					{
						if (texture->desc.sample_count > 1)
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
							srv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
							srv_desc.Texture2DMSArray.ArraySize = sliceCount;
						}
						else
						{
							srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
							srv_desc.Texture2DArray.FirstArraySlice = firstSlice;
							srv_desc.Texture2DArray.ArraySize = sliceCount;
							srv_desc.Texture2DArray.MostDetailedMip = firstMip;
							srv_desc.Texture2DArray.MipLevels = mipCount;
							srv_desc.Texture2DArray.PlaneSlice = plane;
							srv_desc.Texture2DArray.ResourceMinLODClamp = min_lod_clamp;
						}
					}
				}
				else
				{
					if (texture->desc.sample_count > 1)
					{
						srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
						srv_desc.Texture2D.MostDetailedMip = firstMip;
						srv_desc.Texture2D.MipLevels = mipCount;
						srv_desc.Texture2D.PlaneSlice = plane;
						srv_desc.Texture2D.ResourceMinLODClamp = min_lod_clamp;
					}
				}
			}
			else if (texture->desc.type == TextureDesc::Type_TEXTURE_3D)
			{
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
				srv_desc.Texture3D.MostDetailedMip = firstMip;
				srv_desc.Texture3D.MipLevels = mipCount;
				srv_desc.Texture3D.ResourceMinLODClamp = min_lod_clamp;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, srv_desc, internal_state->resource.Get());
			descriptor.firstMip = firstMip;
			descriptor.mipCount = mipCount;
			descriptor.firstSlice = firstSlice;
			descriptor.sliceCount = sliceCount;

			if (!internal_state->srv.IsValid())
			{
				internal_state->srv = descriptor;
				return -1;
			}
			internal_state->subresources_srv.push_back(descriptor);
			return int(internal_state->subresources_srv.size() - 1);
		}
		break;
		case SubresourceType_UAV:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.Format = _ConvertFormat(format);

			if (texture->desc.type == TextureDesc::Type_TEXTURE_1D)
			{
				if (texture->desc.array_size > 1)
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
					uav_desc.Texture1DArray.FirstArraySlice = firstSlice;
					uav_desc.Texture1DArray.ArraySize = sliceCount;
					uav_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
					uav_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::Type_TEXTURE_2D)
			{
				if (texture->desc.array_size > 1)
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					uav_desc.Texture2DArray.FirstArraySlice = firstSlice;
					uav_desc.Texture2DArray.ArraySize = sliceCount;
					uav_desc.Texture2DArray.MipSlice = firstMip;
				}
				else
				{
					uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					uav_desc.Texture2D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::Type_TEXTURE_3D)
			{
				uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
				uav_desc.Texture3D.MipSlice = firstMip;
				uav_desc.Texture3D.FirstWSlice = 0;
				uav_desc.Texture3D.WSize = -1;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, uav_desc, internal_state->resource.Get());
			descriptor.firstMip = firstMip;
			descriptor.mipCount = mipCount;
			descriptor.firstSlice = firstSlice;
			descriptor.sliceCount = sliceCount;

			if (!internal_state->uav.IsValid())
			{
				internal_state->uav = descriptor;
				return -1;
			}
			internal_state->subresources_uav.push_back(descriptor);
			return int(internal_state->subresources_uav.size() - 1);
		}
		break;
		case SubresourceType_RTV:
		{
			D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
			rtv_desc.Format = _ConvertFormat(format);

			if (texture->desc.type == TextureDesc::Type_TEXTURE_1D)
			{
				if (texture->desc.array_size > 1)
				{
					rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
					rtv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					rtv_desc.Texture1DArray.ArraySize = sliceCount;
					rtv_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
					rtv_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::Type_TEXTURE_2D)
			{
				if (texture->desc.array_size > 1)
				{
					if (texture->desc.sample_count > 1)
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
						rtv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
						rtv_desc.Texture2DMSArray.ArraySize = sliceCount;
					}
					else
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
						rtv_desc.Texture2DArray.FirstArraySlice = firstSlice;
						rtv_desc.Texture2DArray.ArraySize = sliceCount;
						rtv_desc.Texture2DArray.MipSlice = firstMip;
					}
				}
				else
				{
					if (texture->desc.sample_count > 1)
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
						rtv_desc.Texture2D.MipSlice = firstMip;
					}
				}
			}
			else if (texture->desc.type == TextureDesc::Type_TEXTURE_3D)
			{
				rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
				rtv_desc.Texture3D.MipSlice = firstMip;
				rtv_desc.Texture3D.FirstWSlice = 0;
				rtv_desc.Texture3D.WSize = -1;
			}

			SingleDescriptor descriptor;
			descriptor.init(this, rtv_desc, internal_state->resource.Get());
			descriptor.firstMip = firstMip;
			descriptor.mipCount = mipCount;
			descriptor.firstSlice = firstSlice;
			descriptor.sliceCount = sliceCount;

			if (!internal_state->rtv.IsValid())
			{
				internal_state->rtv = descriptor;
				return -1;
			}
			internal_state->subresources_rtv.push_back(descriptor);
			return int(internal_state->subresources_rtv.size() - 1);
		}
		break;
		case SubresourceType_DSV:
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
			dsv_desc.Format = _ConvertFormat(format);

			if (texture->desc.type == TextureDesc::Type_TEXTURE_1D)
			{
				if (texture->desc.array_size > 1)
				{
					dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
					dsv_desc.Texture1DArray.FirstArraySlice = firstSlice;
					dsv_desc.Texture1DArray.ArraySize = sliceCount;
					dsv_desc.Texture1DArray.MipSlice = firstMip;
				}
				else
				{
					dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
					dsv_desc.Texture1D.MipSlice = firstMip;
				}
			}
			else if (texture->desc.type == TextureDesc::Type_TEXTURE_2D)
			{
				if (texture->desc.array_size > 1)
				{
					if (texture->desc.sample_count > 1)
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
						dsv_desc.Texture2DMSArray.FirstArraySlice = firstSlice;
						dsv_desc.Texture2DMSArray.ArraySize = sliceCount;
					}
					else
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
						dsv_desc.Texture2DArray.FirstArraySlice = firstSlice;
						dsv_desc.Texture2DArray.ArraySize = sliceCount;
						dsv_desc.Texture2DArray.MipSlice = firstMip;
					}
				}
				else
				{
					if (texture->desc.sample_count > 1)
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
						dsv_desc.Texture2D.MipSlice = firstMip;
					}
				}
			}

			SingleDescriptor descriptor;
			descriptor.init(this, dsv_desc, internal_state->resource.Get());
			descriptor.firstMip = firstMip;
			descriptor.mipCount = mipCount;
			descriptor.firstSlice = firstSlice;
			descriptor.sliceCount = sliceCount;

			if (!internal_state->dsv.IsValid())
			{
				internal_state->dsv = descriptor;
				return -1;
			}
			internal_state->subresources_dsv.push_back(descriptor);
			return int(internal_state->subresources_dsv.size() - 1);
		}
		break;
		default:
			break;
		}
		return -1;
	}

	int D3D12Device::CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size, const Format* format_change, const uint32_t* structuredbuffer_stride_change) const
	{
		auto internal_state = to_internal(buffer);
		const GPUBufferDesc& desc = buffer->GetDesc();

		Format format = desc.format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}
		if (type == SubresourceType_UAV)
		{
			// RW resource can't be SRGB
			format = GetFormatNonSRGB(format);
		}

		switch (type)
		{

		case SubresourceType_SRV:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
			srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

			if (format == Format_UNKNOWN)
			{
				if (has_flag(desc.misc_flags, ResourceMiscFlag_BUFFER_STRUCTURED) || (structuredbuffer_stride_change != nullptr))
				{
					// This is a Structured Buffer
					uint32_t stride = desc.stride;
					if (structuredbuffer_stride_change != nullptr)
					{
						stride = *structuredbuffer_stride_change;
					}
					assert(is_aligned(offset, (uint64_t)stride)); // structured buffer offset must be aligned to structure stride!
					srv_desc.Format = DXGI_FORMAT_UNKNOWN;
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					srv_desc.Buffer.FirstElement = UINT(offset / stride);
					srv_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / stride);
					srv_desc.Buffer.StructureByteStride = stride;
				}
				else
				{
					// This is a Raw Buffer
					assert(has_flag(desc.misc_flags, ResourceMiscFlag_BUFFER_RAW));
					srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
					srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
					srv_desc.Buffer.FirstElement = UINT(offset / sizeof(uint32_t));
					srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
					srv_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / sizeof(uint32_t));
				}
			}
			else
			{
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(format);
				srv_desc.Format = _ConvertFormat(format);
				srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srv_desc.Buffer.FirstElement = UINT(offset / stride);
				srv_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / stride);
			}

			SingleDescriptor descriptor;
			descriptor.init(this, srv_desc, internal_state->resource.Get());
			descriptor.buffer_offset = offset;

			if (!internal_state->srv.IsValid())
			{
				internal_state->srv = descriptor;
				return -1;
			}
			internal_state->subresources_srv.push_back(descriptor);
			return int(internal_state->subresources_srv.size() - 1);
		}
		break;
		case SubresourceType_UAV:
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
			uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;

			if (format == Format_UNKNOWN)
			{
				if (has_flag(desc.misc_flags, ResourceMiscFlag_BUFFER_STRUCTURED) || (structuredbuffer_stride_change != nullptr))
				{
					// This is a Structured Buffer
					uint32_t stride = desc.stride;
					if (structuredbuffer_stride_change != nullptr)
					{
						stride = *structuredbuffer_stride_change;
					}
					assert(is_aligned(offset, (uint64_t)stride)); // structured buffer offset must be aligned to structure stride!
					uav_desc.Format = DXGI_FORMAT_UNKNOWN;
					uav_desc.Buffer.FirstElement = UINT(offset / stride);
					uav_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / stride);
					uav_desc.Buffer.StructureByteStride = stride;
				}
				else
				{
					// This is a Raw Buffer
					assert(has_flag(desc.misc_flags, ResourceMiscFlag_BUFFER_RAW));
					uav_desc.Format = DXGI_FORMAT_R32_TYPELESS;
					uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
					uav_desc.Buffer.FirstElement = UINT(offset / sizeof(uint32_t));
					uav_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / sizeof(uint32_t));
				}
			}
			else
			{
				// This is a Typed Buffer
				uint32_t stride = GetFormatStride(format);
				uav_desc.Format = _ConvertFormat(format);
				uav_desc.Buffer.FirstElement = UINT(offset / stride);
				uav_desc.Buffer.NumElements = UINT(std::min(size, desc.size - offset) / stride);
			}

			SingleDescriptor descriptor;
			descriptor.init(this, uav_desc, internal_state->resource.Get());

			if (!internal_state->uav.IsValid())
			{
				internal_state->uav = descriptor;
				return -1;
			}
			internal_state->subresources_uav.push_back(descriptor);
			return int(internal_state->subresources_uav.size() - 1);
		}
		break;
		default:
			assert(0);
			break;
		}

		return -1;
	}

	void D3D12Device::DeleteSubresources(GPUResource* resource)
	{
		auto internal_state = to_internal(resource);
		internal_state->allocationhandler->destroylocker.lock();
		internal_state->destroy_subresources();
		internal_state->allocationhandler->destroylocker.unlock();
	}

	int D3D12Device::GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource) const
	{
		if (resource == nullptr || !resource->IsValid())
			return -1;

		const auto internal_state = to_internal(resource);

		switch (type)
		{
		default:
		case SubresourceType_SRV:
			if (subresource < 0)
			{
				return internal_state->srv.index;
			}
			else
			{
				if (subresource >= (int)internal_state->subresources_srv.size())
					return -1;
				return internal_state->subresources_srv[subresource].index;
			}
			break;
		case SubresourceType_UAV:
			if (subresource < 0)
			{
				return internal_state->uav.index;
			}
			else
			{
				if (subresource >= (int)internal_state->subresources_uav.size())
					return -1;
				return internal_state->subresources_uav[subresource].index;
			}
			break;
		}

		return -1;
	}

	int D3D12Device::GetDescriptorIndex(const Sampler* sampler) const
	{
		if (sampler == nullptr || !sampler->IsValid())
			return -1;

		auto internal_state = to_internal(sampler);
		return internal_state->descriptor.index;
	}

	void D3D12Device::WriteShadingRateValue(ShadingRate rate, void* dest) const
	{
		D3D12_SHADING_RATE _rate = _ConvertShadingRate(rate);
		if (!additionalShadingRatesSupported)
		{
			_rate = std::min(_rate, D3D12_SHADING_RATE_2X2);
		}
		*(uint8_t*)dest = _rate;
	}

	void D3D12Device::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const
	{
		D3D12_RAYTRACING_INSTANCE_DESC tmp = {};
		if (instance != nullptr)
		{
			auto internal_state = to_internal(instance->bottom_level);
			tmp.AccelerationStructure = internal_state->gpu_address;
			std::memcpy(tmp.Transform, &instance->transform, sizeof(tmp.Transform));
			tmp.InstanceID = instance->instance_id;
			tmp.InstanceMask = instance->instance_mask;
			tmp.InstanceContributionToHitGroupIndex = instance->instance_contribution_to_hit_group_index;
			tmp.Flags = instance->flags;
		}
		std::memcpy(dest, &tmp, sizeof(tmp)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
	}

	void D3D12Device::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
		auto internal_state = to_internal(rtpso);

		const void* identifier = internal_state->stateObjectProperties->GetShaderIdentifier(internal_state->group_strings[group_index].c_str());
		std::memcpy(dest, identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	}

	void D3D12Device::SetName(GPUResource* pResource, const char* name) const
	{
		wchar_t text[256];
		if (Protean::Helper::StringConvert(name, text, arraysize(text)) > 0)
		{
			auto internal_state = to_internal(pResource);
			if (internal_state->resource != nullptr)
			{
				internal_state->resource->SetName(text);
			}
		}
	}

	SizeAlignment D3D12Device::GetDeviceTextureMemoryRequirements(const TextureDesc* desc) const
	{
		D3D12_RESOURCE_DESC resourcedesc = _ConvertTextureDesc(*desc, casting_fully_typed_formats);
		D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = device->GetResourceAllocationInfo(0, 1, &resourcedesc);
		SizeAlignment ret;
		ret.size = allocationInfo.SizeInBytes;
		ret.alignment = allocationInfo.Alignment;
		return ret;
	}

	void D3D12Device::SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count)
	{
#ifdef PLATFORM_XBOX
		queue = QUEUE_GRAPHICS; // looks like non-graphics queue is a problem for sparse update on xbox (TODO: investigate)
#endif // PLATFORM_XBOX

		CommandQueue& q = queues[queue];

		thread_local Protean::vector<D3D12_TILED_RESOURCE_COORDINATE> tiled_resource_coordinates;
		thread_local Protean::vector<D3D12_TILE_REGION_SIZE> tiled_region_sizes;
		thread_local Protean::vector<D3D12_TILE_RANGE_FLAGS> tile_range_flags;
		thread_local Protean::vector<uint32_t> range_start_offsets;

		for (uint32_t c = 0; c < command_count; ++c)
		{
			const SparseUpdateCommand& command = commands[c];
			auto internal_sparse_resource = to_internal(command.sparse_resource);
			uint32_t mip_count = 0;
			uint32_t array_size = 0;
			if (command.sparse_resource->IsTexture())
			{
				const Texture* sparse_texture = (const Texture*)command.sparse_resource;
				mip_count = sparse_texture->desc.mip_levels;
				array_size = sparse_texture->desc.array_size;
			}
			ID3D12Heap* heap = nullptr;
			uint32_t heap_page_offset = 0;
			if (command.tile_pool != nullptr)
			{
				auto internal_tile_pool = to_internal(command.tile_pool);
				heap = internal_tile_pool->allocation->GetHeap();
				heap_page_offset = uint32_t(internal_tile_pool->allocation->GetOffset() / D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES);
			}
			tiled_resource_coordinates.resize(command.num_resource_regions);
			tiled_region_sizes.resize(command.num_resource_regions);
			for (uint32_t i = 0; i < command.num_resource_regions; ++i)
			{
				const SparseResourceCoordinate& in_coordinate = command.coordinates[i];
				const SparseRegionSize& in_size = command.sizes[i];
				D3D12_TILED_RESOURCE_COORDINATE& out_coordinate = tiled_resource_coordinates[i];
				D3D12_TILE_REGION_SIZE& out_size = tiled_region_sizes[i];
				out_coordinate.X = in_coordinate.x;
				out_coordinate.Y = in_coordinate.y;
				out_coordinate.Z = in_coordinate.z;
				out_coordinate.Subresource = D3D12CalcSubresource(in_coordinate.mip, in_coordinate.slice, 0, mip_count, array_size);
				out_size.UseBox = in_coordinate.mip < internal_sparse_resource->sparse_texture_properties.packed_mip_start; // only for non packed mips
				out_size.Width = in_size.width;
				out_size.Height = in_size.height;
				out_size.Depth = in_size.depth;
				out_size.NumTiles = out_size.Width * out_size.Height * out_size.Depth;
			}
			tile_range_flags.resize(command.num_resource_regions);
			range_start_offsets.resize(command.num_resource_regions);
			for (uint32_t i = 0; i < command.num_resource_regions; ++i)
			{
				const TileRangeFlags& in_flags = command.range_flags[i];
				D3D12_TILE_RANGE_FLAGS& out_flags = tile_range_flags[i];
				switch (in_flags)
				{
				default:
				case TileRangeFlags_None:
					out_flags = D3D12_TILE_RANGE_FLAG_NONE;
					break;
				case TileRangeFlags_Null:
					out_flags = D3D12_TILE_RANGE_FLAG_NULL;
					break;
				}
				const uint32_t in_offset = command.range_start_offsets[i];
				uint32_t& out_offset = range_start_offsets[i];
				out_offset = heap_page_offset + in_offset;
			}

#ifdef PLATFORM_XBOX
			std::scoped_lock lock(queue_locker); // queue operations are not thread-safe on XBOX
#endif // PLATFORM_XBOX
			q.queue->UpdateTileMappings(
				internal_sparse_resource->resource.Get(),
				command.num_resource_regions,
				tiled_resource_coordinates.data(),
				tiled_region_sizes.data(),
				heap,
				command.num_resource_regions,
				tile_range_flags.data(),
				range_start_offsets.data(),
				command.range_tile_counts,
				D3D12_TILE_MAPPING_FLAG_NONE
			);
		}
	}

	void D3D12Device::CopyBufferAsync(const GPUBufferCopyCommand* commands, uint32_t command_count, const char* name) const
	{
		CopyAllocator::CopyCMD cmd = copyAllocator.allocate(0);
		if (name != nullptr)
		{
			wchar_t text[128];
			if (Protean::Helper::StringConvert(name, text, arraysize(text)) > 0)
			{
				PIXBeginEvent(cmd.commandList.Get(), 0xFF000000, text);
			}
		}
		for (uint32_t i = 0; i < command_count; ++i)
		{
			const GPUBufferCopyCommand& command = commands[i];
			auto dst_internal = to_internal(command.dst);
			auto src_internal = to_internal(command.src);
			cmd.commandList->CopyBufferRegion(dst_internal->resource.Get(), command.dst_offset, src_internal->resource.Get(), command.src_offset, command.size);
		}
		if (name != nullptr)
		{
			PIXEndEvent(cmd.commandList.Get());
		}
		copyAllocator.submit(cmd, false);
	}
}

#endif // WICKEDENGINE_BUILD_DX12
