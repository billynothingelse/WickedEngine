#include "rhi/vulkan/vulkan_internal.h"
// Vulkan backend: GPU resource and pipeline creation

#ifdef WICKEDENGINE_BUILD_VULKAN

namespace Protean::Graphics
{
	using namespace VulkanInternal;

	bool VulkanDevice::CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = Protean::Allocator::make_shared<Buffer_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		buffer->internal_state = internal_state;
		buffer->type = GPUResource::Type_BUFFER;
		buffer->mapped_data = nullptr;
		buffer->mapped_size = 0;
		buffer->desc = *desc;

		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = buffer->desc.size;
		bufferInfo.usage = 0;
		if (has_flag(buffer->desc.bind_flags, BindFlag_VERTEX_BUFFER))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.bind_flags, BindFlag_INDEX_BUFFER))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.bind_flags, BindFlag_CONSTANT_BUFFER))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			bufferInfo.size = align(bufferInfo.size + dynamic_cbv_maxsize, dynamic_cbv_maxsize); // hack: padding for descriptor writes!
		}
		if (has_flag(buffer->desc.bind_flags, BindFlag_SHADER_RESOURCE))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; // read only ByteAddressBuffer is also storage buffer
			bufferInfo.usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.bind_flags, BindFlag_UNORDERED_ACCESS))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag_BUFFER_RAW))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag_BUFFER_STRUCTURED))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag_INDIRECT_ARGS))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag_RAY_TRACING))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
			bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
			buffer->desc.alignment = std::max(buffer->desc.alignment, 16u);
			buffer->desc.alignment = std::max(buffer->desc.alignment, raytracing_properties.shaderGroupBaseAlignment);
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag_PREDICATION))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
		}
		if (features_1_2.bufferDeviceAddress == VK_TRUE)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		}
		if (has_flag(buffer->desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE))
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR;
		}
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VkVideoProfileListInfoKHR profile_list_info = {};
		profile_list_info.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR;
		if (has_flag(desc->misc_flags, ResourceMiscFlag_VIDEO_COMPATIBILITY_H264))
		{
			profile_list_info.pProfiles = &video_capability_h264.profile;
			profile_list_info.profileCount = 1;
			bufferInfo.pNext = &profile_list_info;
		}
		else if (has_flag(desc->misc_flags, ResourceMiscFlag_VIDEO_COMPATIBILITY_H265))
		{
			profile_list_info.pProfiles = &video_capability_h265.profile;
			profile_list_info.profileCount = 1;
			bufferInfo.pNext = &profile_list_info;
		}

		bufferInfo.flags = 0;

		if (families.size() > 1)
		{
			bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			bufferInfo.queueFamilyIndexCount = (uint32_t)families.size();
			bufferInfo.pQueueFamilyIndices = families.data();
		}
		else
		{
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		if (desc->usage == Usage_READBACK)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		}
		else if (desc->usage == Usage_UPLOAD)
		{
			bufferInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		}

		VkResult res;

		if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_BUFFER) ||
			has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS) ||
			has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS))
		{
			VkMemoryRequirements memory_requirements = {};
			memory_requirements.alignment = buffer->desc.alignment;
			if (memory_requirements.alignment == 0)
			{
				memory_requirements.alignment = GetMinOffsetAlignment(desc);
			}
			memory_requirements.size = align(desc->size, memory_requirements.alignment);
			memory_requirements.memoryTypeBits = ~0u;

			VmaAllocationCreateInfo create_info = {};
			if (desc->usage == Usage_READBACK)
			{
				create_info.usage = VMA_MEMORY_USAGE_GPU_TO_CPU;
				create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
			else if (desc->usage == Usage_UPLOAD)
			{
				create_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
				create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
			else
			{
				create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
			}

			create_info.flags |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;

			if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS) ||
				has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS))
			{
				create_info.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // workaround for potential image offset issue (varying formats, varying offset requirements)
			}

			VmaAllocationInfo allocation_info = {};

			res = vulkan_check(vmaAllocateMemory(
				allocationhandler->allocator,
				&memory_requirements,
				&create_info,
				&internal_state->allocation,
				&allocation_info
			));

			res = vulkan_check(vkCreateBuffer(
				device,
				&bufferInfo,
				nullptr,
				&internal_state->resource
			));

			res = vulkan_check(vkBindBufferMemory(
				device,
				internal_state->resource,
				GetAllocationMemory(internal_state->allocation),
				GetAllocationOffset(internal_state->allocation)
			));
		}
		else if (has_flag(desc->misc_flags, ResourceMiscFlag_SPARSE))
		{
			assert(CheckCapability(GraphicsDeviceCapability_SPARSE_BUFFER));
			bufferInfo.flags |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
			bufferInfo.flags |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
			bufferInfo.flags |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;

			res = vulkan_check(vkCreateBuffer(device, &bufferInfo, nullptr, &internal_state->resource));

			VkMemoryRequirements memory_requirements = {};
			vkGetBufferMemoryRequirements(
				device,
				internal_state->resource,
				&memory_requirements
			);
			buffer->sparse_page_size = (uint32_t)memory_requirements.alignment;
		}
		else
		{
			VmaAllocationCreateInfo create_info = {};
			if (desc->usage == Usage_READBACK)
			{
				create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
				create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
			else if (desc->usage == Usage_UPLOAD)
			{
				create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
				create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}
			else
			{
				create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			}

			if (desc->usage == Usage_DEFAULT && (bufferInfo.usage & VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR))
			{
				// NVidia video bitstream buffer workaround: DEFAULT usage resource which is not mapped producing incorrect result
				create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}

			if (alias == nullptr)
			{
				if (buffer->desc.alignment > 0)
				{
					res = vulkan_check(vmaCreateBufferWithAlignment(
						allocationhandler->allocator,
						&bufferInfo,
						&create_info,
						buffer->desc.alignment,
						&internal_state->resource,
						&internal_state->allocation,
						nullptr
					));
				}
				else
				{
					res = vulkan_check(vmaCreateBuffer(
						allocationhandler->allocator,
						&bufferInfo,
						&create_info,
						&internal_state->resource,
						&internal_state->allocation,
						nullptr
					));
				}
			}
			else
			{
				// Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
				if (alias->IsTexture())
				{
					auto alias_internal = to_internal<Texture>(alias);
					res = vulkan_check(vmaCreateAliasingBuffer2(
						allocationhandler->allocator,
						alias_internal->allocation,
						alias_offset,
						&bufferInfo,
						&internal_state->resource
					));
				}
				else
				{
					auto alias_internal = to_internal<GPUBuffer>(alias);
					res = vulkan_check(vmaCreateAliasingBuffer2(
						allocationhandler->allocator,
						alias_internal->allocation,
						alias_offset,
						&bufferInfo,
						&internal_state->resource
					));
				}
			}
		}

		if (desc->usage == Usage_READBACK || desc->usage == Usage_UPLOAD)
		{
			if (alias == nullptr)
			{
				buffer->mapped_data = GetAllocationMappedData(internal_state->allocation);
				buffer->mapped_size = GetAllocationSize(internal_state->allocation);
			}
			else
			{
				wilog_assert(alias->mapped_data != nullptr, "Aliased buffer created with mapping request, but the aliasing storage resource was not mapped!");
				buffer->mapped_data = (uint8_t*)alias->mapped_data + alias_offset;
				buffer->mapped_size = desc->size;
			}
		}

		if (bufferInfo.usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT)
		{
			VkBufferDeviceAddressInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
			info.buffer = internal_state->resource;
			internal_state->address = vkGetBufferDeviceAddress(device, &info);
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

			if(cmd.IsValid())
			{
				VkBufferCopy copyRegion = {};
				copyRegion.size = buffer->desc.size;
				copyRegion.srcOffset = 0;
				copyRegion.dstOffset = 0;

				vkCmdCopyBuffer(
					cmd.transferCommandBuffer,
					to_internal(&cmd.uploadbuffer)->resource,
					internal_state->resource,
					1,
					&copyRegion
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

		return res == VK_SUCCESS;
	}

	bool VulkanDevice::CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias, uint64_t alias_offset) const
	{
		auto internal_state = Protean::Allocator::make_shared<Texture_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->defaultLayout = _ConvertImageLayout(desc->layout);
		texture->internal_state = internal_state;
		texture->type = GPUResource::Type_TEXTURE;
		texture->mapped_data = nullptr;
		texture->mapped_size = 0;
		texture->mapped_subresources = nullptr;
		texture->mapped_subresource_count = 0;
		texture->sparse_properties = nullptr;
		texture->desc = *desc;

		texture->desc.mip_levels = GetMipCount(texture->desc);

		VkVideoProfileListInfoKHR profile_list_info = {};
		profile_list_info.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR;

		VkImageCreateInfo imageInfo = _ConvertTextureDesc(texture->desc, families.data(), (uint32_t)families.size(), profile_list_info, video_capability_h264.profile, video_capability_h265.profile);

		VkResult res = VK_SUCCESS;

		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_SPARSE))
		{
			assert(CheckCapability(GraphicsDeviceCapability_SPARSE_TEXTURE2D) || imageInfo.imageType != VK_IMAGE_TYPE_2D);
			assert(CheckCapability(GraphicsDeviceCapability_SPARSE_TEXTURE3D) || imageInfo.imageType != VK_IMAGE_TYPE_3D);

			vulkan_check(vkCreateImage(device, &imageInfo, nullptr, &internal_state->resource));

			VkMemoryRequirements memory_requirements = {};
			// no check needed, returns void
			vkGetImageMemoryRequirements(
				device,
				internal_state->resource,
				&memory_requirements
			);
			texture->sparse_page_size = (uint32_t)memory_requirements.alignment;

			uint32_t sparse_requirement_count = 0;

			vkGetImageSparseMemoryRequirements(
				device,
				internal_state->resource,
				&sparse_requirement_count,
				nullptr
			);

			Protean::vector<VkSparseImageMemoryRequirements> sparse_requirements(sparse_requirement_count);
			texture->sparse_properties = &internal_state->sparse_texture_properties;

			vkGetImageSparseMemoryRequirements(
				device,
				internal_state->resource,
				&sparse_requirement_count,
				sparse_requirements.data()
			);

			SparseTextureProperties& out_sparse = internal_state->sparse_texture_properties;
			out_sparse.total_tile_count = uint32_t(memory_requirements.size / memory_requirements.alignment);

			for (size_t i = 0; i < sparse_requirements.size(); ++i)
			{
				const VkSparseImageMemoryRequirements& in_sparse = sparse_requirements[i];
				if (i == 0)
				{
					// These should be common for all subresources right? Like in DX12?
					out_sparse.tile_width = in_sparse.formatProperties.imageGranularity.width;
					out_sparse.tile_height = in_sparse.formatProperties.imageGranularity.height;
					out_sparse.tile_depth = in_sparse.formatProperties.imageGranularity.depth;
					out_sparse.packed_mip_start = in_sparse.imageMipTailFirstLod;
					out_sparse.packed_mip_count = texture->desc.mip_levels - in_sparse.imageMipTailFirstLod;
					out_sparse.packed_mip_tile_offset = uint32_t(in_sparse.imageMipTailOffset / memory_requirements.alignment);
					out_sparse.packed_mip_tile_count = uint32_t(in_sparse.imageMipTailSize / memory_requirements.alignment);
				}
			}

		}
		else
		{
			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

			if (has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_BUFFER) ||
				has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS) ||
				has_flag(desc->misc_flags, ResourceMiscFlag_ALIASING_TEXTURE_RT_DS))
			{
				allocInfo.flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // workaround for potential image offset issue (varying formats, varying offset requirements)
				allocInfo.flags |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
			}

			if (texture->desc.usage == Usage_READBACK || texture->desc.usage == Usage_UPLOAD)
			{
				// Note: we are creating a buffer instead of linear image because linear image cannot have mips
				//	With a buffer, we can tightly pack mips linearly into a buffer so it won't have that limitation
				VkBufferCreateInfo bufferInfo = {};
				bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
				bufferInfo.size = ComputeTextureMemorySizeInBytes(*desc);

				allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
				if (texture->desc.usage == Usage_READBACK)
				{
					allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
					bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				}
				else if (texture->desc.usage == Usage_UPLOAD)
				{
					allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
					bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				}

				res = vulkan_check(vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &internal_state->staging_resource, &internal_state->allocation, nullptr));

				if (texture->desc.usage == Usage_READBACK || texture->desc.usage == Usage_UPLOAD)
				{
					texture->mapped_data = GetAllocationMappedData(internal_state->allocation);
					texture->mapped_size = GetAllocationSize(internal_state->allocation);

					CreateTextureSubresourceDatas(*desc, texture->mapped_data, internal_state->mapped_subresources, (uint32_t)properties2.properties.limits.optimalBufferCopyRowPitchAlignment);
					texture->mapped_subresources = internal_state->mapped_subresources.data();
					texture->mapped_subresource_count = internal_state->mapped_subresources.size();
				}
			}
			else
			{
				VmaAllocator allocator = allocationhandler->allocator;
				VkExternalMemoryImageCreateInfo externalMemImageCreateInfo = {};

				if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_SHARED))
				{
					externalMemImageCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
					// TODO: Expose this? should we use VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT?
					externalMemImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#else
					externalMemImageCreateInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
#endif
					imageInfo.pNext = &externalMemImageCreateInfo;

					// We have to use a dedicated allocator for external handles that has been created with VkExportMemoryAllocateInfo
					allocator = allocationhandler->externalAllocator;

					allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
				}

				if (alias == nullptr)
				{
					res = vulkan_check(vmaCreateImage(
						allocator,
						&imageInfo,
						&allocInfo,
						&internal_state->resource,
						&internal_state->allocation,
						nullptr
					));
				}
				else
				{
					// Aliasing: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/resource_aliasing.html
					if (alias->IsTexture())
					{
						auto alias_internal = to_internal<Texture>(alias);
						res = vulkan_check(vmaCreateAliasingImage2(
							allocator,
							alias_internal->allocation,
							alias_offset,
							&imageInfo,
							&internal_state->resource
						));
					}
					else
					{
						auto alias_internal = to_internal<GPUBuffer>(alias);
						res = vulkan_check(vmaCreateAliasingImage2(
							allocator,
							alias_internal->allocation,
							alias_offset,
							&imageInfo,
							&internal_state->resource
						));
					}
					if (res != VK_SUCCESS)
					{
						// safety fallback if dedicated memory required
						internal_state->allocation = VK_NULL_HANDLE;
						internal_state->resource = VK_NULL_HANDLE;
						res = vulkan_check(vmaCreateImage(
							allocator,
							&imageInfo,
							&allocInfo,
							&internal_state->resource,
							&internal_state->allocation,
							nullptr
						));
					}
				}

				if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_SHARED))
				{
					VmaAllocationInfo allocationInfo;
					vmaGetAllocationInfo(allocator, internal_state->allocation, &allocationInfo);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
					VkMemoryGetWin32HandleInfoKHR getWin32HandleInfoKHR = {};
					getWin32HandleInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR;
					getWin32HandleInfoKHR.pNext = nullptr;
					getWin32HandleInfoKHR.memory = allocationInfo.deviceMemory;
					getWin32HandleInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
					res = vulkan_check(vkGetMemoryWin32HandleKHR(allocationhandler->device, &getWin32HandleInfoKHR, &texture->shared_handle));
#elif defined(__linux__)
					VkMemoryGetFdInfoKHR memoryGetFdInfoKHR = {};
					memoryGetFdInfoKHR.sType = VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR;
					memoryGetFdInfoKHR.pNext = nullptr;
					memoryGetFdInfoKHR.memory = allocationInfo.deviceMemory;
					memoryGetFdInfoKHR.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
					res = vulkan_check(vkGetMemoryFdKHR(allocationhandler->device, &memoryGetFdInfoKHR, &texture->shared_handle));
#endif
				}
			}
		}

		// Issue data copy on request:
		if (initial_data != nullptr)
		{
			CopyAllocator::CopyCMD cmd;
			void* mapped_data = nullptr;
			if (desc->usage == Usage_UPLOAD)
			{
				mapped_data = texture->mapped_data;
			}
			else
			{
				cmd = copyAllocator.allocate(GetAllocationSize(internal_state->allocation));
				mapped_data = cmd.uploadbuffer.mapped_data;
			}

			Protean::vector<VkBufferImageCopy> copyRegions;

			const uint32_t data_stride = GetFormatStride(desc->format);
			const uint32_t block_size = GetFormatBlockSize(desc->format);

			VkDeviceSize copyOffset = 0;
			uint32_t initDataIdx = 0;
			for (uint32_t layer = 0; layer < desc->array_size; ++layer)
			{
				uint32_t width = imageInfo.extent.width;
				uint32_t height = imageInfo.extent.height;
				uint32_t depth = imageInfo.extent.depth;
				for (uint32_t mip = 0; mip < texture->desc.mip_levels; ++mip)
				{
					const SubresourceData& subresourceData = initial_data[initDataIdx++];
					const uint32_t num_blocks_x = std::max(1u, width / block_size);
					const uint32_t num_blocks_y = std::max(1u, height / block_size);
					const uint32_t dst_rowpitch = num_blocks_x * data_stride;
					const uint32_t dst_slicepitch = dst_rowpitch * num_blocks_y;
					const uint32_t src_rowpitch = subresourceData.row_pitch;
					const uint32_t src_slicepitch = subresourceData.slice_pitch;
					for (uint32_t z = 0; z < depth; ++z)
					{
						uint8_t* dst_slice = (uint8_t*)mapped_data + copyOffset + dst_slicepitch * z;
						uint8_t* src_slice = (uint8_t*)subresourceData.data_ptr + src_slicepitch * z;
						for (uint32_t y = 0; y < num_blocks_y; ++y)
						{
							std::memcpy(
								dst_slice + dst_rowpitch * y,
								src_slice + src_rowpitch * y,
								dst_rowpitch
							);
						}
					}

					if (cmd.IsValid())
					{
						VkBufferImageCopy copyRegion = {};
						copyRegion.bufferOffset = copyOffset;
						copyRegion.bufferRowLength = src_rowpitch / data_stride * block_size;
						copyRegion.bufferImageHeight = 0;

						copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						copyRegion.imageSubresource.mipLevel = mip;
						copyRegion.imageSubresource.baseArrayLayer = layer;
						copyRegion.imageSubresource.layerCount = 1;

						copyRegion.imageOffset = { 0, 0, 0 };
						copyRegion.imageExtent = {
							width,
							height,
							depth
						};

						copyRegions.push_back(copyRegion);
					}

					copyOffset += dst_slicepitch * depth;
					copyOffset = align(copyOffset, VkDeviceSize(4)); // fix for validation: on transfer queue the srcOffset must be 4-byte aligned

					width = std::max(1u, width / 2);
					height = std::max(1u, height / 2);
					depth = std::max(1u, depth / 2);
				}
			}

			if(cmd.IsValid())
			{
				VkImageMemoryBarrier2 barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.image = internal_state->resource;
				barrier.oldLayout = imageInfo.initialLayout;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrier.srcAccessMask = 0;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				VkDependencyInfo dependencyInfo = {};
				dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
				dependencyInfo.imageMemoryBarrierCount = 1;
				dependencyInfo.pImageMemoryBarriers = &barrier;

				vkCmdPipelineBarrier2(cmd.transferCommandBuffer, &dependencyInfo);

				vkCmdCopyBufferToImage(
					cmd.transferCommandBuffer,
					to_internal(&cmd.uploadbuffer)->resource,
					internal_state->resource,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					(uint32_t)copyRegions.size(),
					copyRegions.data()
				);

				copyAllocator.submit(cmd);

				// Note: the copy allocator is done at this point on CPU and GPU, so transition will be safe:
				std::swap(barrier.srcStageMask, barrier.dstStageMask);
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = _ConvertImageLayout(texture->desc.layout);
				barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = _ParseResourceState(texture->desc.layout);
				std::scoped_lock lck(transitionLocker);
				init_transitions.push_back(barrier);
			}
		}
		else if(texture->desc.layout != ResourceState_UNDEFINED && internal_state->resource != VK_NULL_HANDLE)
		{
			VkImageMemoryBarrier2 barrier = {};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			barrier.image = internal_state->resource;
			barrier.oldLayout = imageInfo.initialLayout;
			barrier.newLayout = _ConvertImageLayout(texture->desc.layout);
			barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			barrier.srcAccessMask = 0;
			barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			barrier.dstAccessMask = _ParseResourceState(texture->desc.layout);
			if (IsFormatDepthSupport(texture->desc.format))
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				if (IsFormatStencilSupport(texture->desc.format))
				{
					barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
				}
			}
			else
			{
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			}
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = imageInfo.arrayLayers;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = imageInfo.mipLevels;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			std::scoped_lock lck(transitionLocker);
			init_transitions.push_back(barrier);
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

		if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE) || has_flag(texture->desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE_OUTPUT_ONLY) || has_flag(texture->desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE_DPB_ONLY))
		{
			VkImageViewCreateInfo view_desc = {};
			view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			view_desc.flags = 0;
			view_desc.image = internal_state->resource;
			view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			view_desc.subresourceRange.baseArrayLayer = 0;
			view_desc.subresourceRange.layerCount = imageInfo.arrayLayers;
			view_desc.subresourceRange.baseMipLevel = 0;
			view_desc.subresourceRange.levelCount = imageInfo.mipLevels;
			view_desc.format = imageInfo.format;
			view_desc.viewType = imageInfo.arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

			VkImageViewUsageCreateInfo viewUsageInfo = {};
			viewUsageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
			if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE_DPB_ONLY))
			{
				viewUsageInfo.usage = VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
			}
			else if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE_OUTPUT_ONLY))
			{
				viewUsageInfo.usage = VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
			}
			else
			{
				viewUsageInfo.usage = VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
			}
			view_desc.pNext = &viewUsageInfo;

			res = vulkan_check(vkCreateImageView(device, &view_desc, nullptr, &internal_state->video_decode_view));
		}

		return res >= VK_SUCCESS;
	}

	bool VulkanDevice::CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader, const char* entrypoint) const
	{
		auto internal_state = Protean::Allocator::make_shared<Shader_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->entrypoint = entrypoint;
		shader->internal_state = internal_state;
		shader->stage = stage;

		VkResult res = VK_SUCCESS;

		VkShaderModuleCreateInfo moduleInfo = {};
		moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleInfo.codeSize = shadercode_size;
		moduleInfo.pCode = (const uint32_t*)shadercode;
		res = vulkan_check(vkCreateShaderModule(device, &moduleInfo, nullptr, &internal_state->shaderModule));

		internal_state->stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		internal_state->stageInfo.module = internal_state->shaderModule;
		internal_state->stageInfo.pName = internal_state->entrypoint.c_str();
		switch (stage)
		{
		case ShaderStage_MS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
			break;
		case ShaderStage_AS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
			break;
		case ShaderStage_VS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case ShaderStage_HS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			break;
		case ShaderStage_DS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			break;
		case ShaderStage_GS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case ShaderStage_PS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case ShaderStage_CS:
			internal_state->stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		default:
			// also means library shader (ray tracing)
			internal_state->stageInfo.stage = VK_SHADER_STAGE_ALL;
			break;
		}

		internal_state->layout = layout_template;

		// Right now reflection must be used to find exact binding types for SRV and UAV:
		{
			SpvReflectShaderModule module;
			SpvReflectResult result = spvReflectCreateShaderModule(moduleInfo.codeSize, moduleInfo.pCode, &module);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			uint32_t binding_count = 0;
			result = spvReflectEnumerateDescriptorBindings(&module, &binding_count, nullptr);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			SpvReflectDescriptorBinding* bindings[DESCRIPTORBINDER_ALL_COUNT] = {};
			assert(binding_count < arraysize(bindings));
			result = spvReflectEnumerateDescriptorBindings(&module, &binding_count, bindings);
			assert(result == SPV_REFLECT_RESULT_SUCCESS);

			for (uint32_t i = 0; i < binding_count; ++i)
			{
				if (bindings[i] == nullptr)
					continue;
				const auto& refl = *bindings[i];
				if (refl.accessed == false)
					continue;
				if (refl.set > 0)
					continue; // don't care about bindless here
				if (refl.resource_type != SPV_REFLECT_RESOURCE_FLAG_SRV && refl.resource_type != SPV_REFLECT_RESOURCE_FLAG_UAV)
					continue; // don't care about anything other than SRV and UAV here, we reduce descriptor sets by hashing only based on SRV and UAV types

				VkDescriptorSetLayoutBinding binding = {};
				binding.binding = refl.binding;
				binding.descriptorCount = refl.count;
				binding.descriptorType = (VkDescriptorType)refl.descriptor_type;
				binding.stageFlags = VK_SHADER_STAGE_ALL;

				VkImageViewType view_type = {};
				switch (refl.descriptor_type)
				{
				default:
					break;
				case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					switch (refl.image.dim)
					{
					default:
					case SpvDim1D:
						if (refl.image.arrayed == 0)
						{
							view_type = VK_IMAGE_VIEW_TYPE_1D;
						}
						else
						{
							view_type = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
						}
						break;
					case SpvDim2D:
						if (refl.image.arrayed == 0)
						{
							view_type = VK_IMAGE_VIEW_TYPE_2D;
						}
						else
						{
							view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
						}
						break;
					case SpvDim3D:
						view_type = VK_IMAGE_VIEW_TYPE_3D;
						break;
					case SpvDimCube:
						if (refl.image.arrayed == 0)
						{
							view_type = VK_IMAGE_VIEW_TYPE_CUBE;
						}
						else
						{
							view_type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
						}
						break;
					}
					break;
				}

				switch (refl.resource_type)
				{
				case SPV_REFLECT_RESOURCE_FLAG_SRV:
				{
					const uint32_t srv_slot = binding.binding - VULKAN_BINDING_SHIFT_T;
					internal_state->layout.table.SRV[srv_slot] = binding;
					internal_state->layout.SRV_image_types[srv_slot] = view_type;
				}
				break;
				case SPV_REFLECT_RESOURCE_FLAG_UAV:
				{
					const uint32_t uav_slot = binding.binding - VULKAN_BINDING_SHIFT_U;
					internal_state->layout.table.UAV[uav_slot] = binding;
					internal_state->layout.UAV_image_types[uav_slot] = view_type;
				}
				break;
				default:
					assert(0);
					break;
				}
			}

			spvReflectDestroyShaderModule(&module);
		}

		if (stage == ShaderStage_CS)
		{
			VkComputePipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
			pipelineInfo.layout = cache_pso_layout(internal_state->layout);
			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

			// Create compute pipeline state in place:
			pipelineInfo.stage = internal_state->stageInfo;

			res = vulkan_check(vkCreateComputePipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &internal_state->pipeline_cs));
		}
		else if (stage == ShaderStage_LIB)
		{
			cache_pso_layout(internal_state->layout);
		}

		return res == VK_SUCCESS;
	}

	bool VulkanDevice::CreateSampler(const SamplerDesc* desc, Sampler* sampler) const
	{
		auto internal_state = Protean::Allocator::make_shared<Sampler_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		sampler->internal_state = internal_state;
		sampler->desc = *desc;

		VkSamplerCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		createInfo.flags = 0;
		createInfo.pNext = nullptr;

		switch (desc->filter)
		{
		case Filter_MIN_MAG_MIP_POINT:
		case Filter_MINIMUM_MIN_MAG_MIP_POINT:
		case Filter_MAXIMUM_MIN_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter_MIN_MAG_POINT_MIP_LINEAR:
		case Filter_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
		case Filter_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter_MIN_POINT_MAG_LINEAR_MIP_POINT:
		case Filter_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
		case Filter_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter_MIN_POINT_MAG_MIP_LINEAR:
		case Filter_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
		case Filter_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter_MIN_LINEAR_MAG_MIP_POINT:
		case Filter_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
		case Filter_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		case Filter_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
		case Filter_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter_MIN_MAG_LINEAR_MIP_POINT:
		case Filter_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
		case Filter_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter_MIN_MAG_MIP_LINEAR:
		case Filter_MINIMUM_MIN_MAG_MIP_LINEAR:
		case Filter_MAXIMUM_MIN_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		case Filter_ANISOTROPIC:
		case Filter_MINIMUM_ANISOTROPIC:
		case Filter_MAXIMUM_ANISOTROPIC:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = std::min(16.0f, std::max(1.0f, static_cast<float>(desc->max_anisotropy)));
			createInfo.compareEnable = false;
			break;
		case Filter_COMPARISON_MIN_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter_COMPARISON_MIN_MAG_MIP_LINEAR:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = true;
			break;
		case Filter_COMPARISON_ANISOTROPIC:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = std::min(16.0f, std::max(1.0f, static_cast<float>(desc->max_anisotropy)));
			createInfo.compareEnable = true;
			break;
		default:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.anisotropyEnable = false;
			createInfo.compareEnable = false;
			break;
		}

		VkSamplerReductionModeCreateInfo reductionmode = {};
		reductionmode.sType = VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO;
		if (CheckCapability(GraphicsDeviceCapability_SAMPLER_MINMAX))
		{
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
				reductionmode.reductionMode = VK_SAMPLER_REDUCTION_MODE_MIN;
				createInfo.pNext = &reductionmode;
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
				reductionmode.reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
				createInfo.pNext = &reductionmode;
				break;
			default:
				break;
			}
		}

		createInfo.addressModeU = _ConvertTextureAddressMode(desc->address_u, features_1_2);
		createInfo.addressModeV = _ConvertTextureAddressMode(desc->address_v, features_1_2);
		createInfo.addressModeW = _ConvertTextureAddressMode(desc->address_w, features_1_2);
		createInfo.compareOp = _ConvertComparisonFunc(desc->comparison_func);
		createInfo.minLod = desc->min_lod;
		createInfo.maxLod = desc->max_lod;
		createInfo.mipLodBias = desc->mip_lod_bias;
		createInfo.borderColor = _ConvertSamplerBorderColor(desc->border_color);
		createInfo.unnormalizedCoordinates = VK_FALSE;

		VkResult res = vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &internal_state->resource));

		internal_state->index = allocationhandler->bindlessSamplers.allocate();
		if (internal_state->index >= 0)
		{
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.sampler = internal_state->resource;
			VkWriteDescriptorSet write = {};
			write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			write.dstBinding = 0;
			write.dstArrayElement = internal_state->index;
			write.descriptorCount = 1;
			write.dstSet = allocationhandler->bindlessSamplers.descriptorSet;
			write.pImageInfo = &imageInfo;
			vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
		}
		else
		{
			assert(0);
		}

		return res == VK_SUCCESS;
	}

	bool VulkanDevice::CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const
	{
		auto internal_state = Protean::Allocator::make_shared<QueryHeap_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		queryheap->internal_state = internal_state;
		queryheap->desc = *desc;

		VkQueryPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		poolInfo.queryCount = desc->query_count;

		switch (desc->type)
		{
		case GpuQueryType_TIMESTAMP:
			poolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
			break;
		case GpuQueryType_OCCLUSION:
		case GpuQueryType_OCCLUSION_BINARY:
			poolInfo.queryType = VK_QUERY_TYPE_OCCLUSION;
			break;
		}

		VkResult res = vulkan_check(vkCreateQueryPool(device, &poolInfo, nullptr, &internal_state->pool));

		return res == VK_SUCCESS;
	}

	bool VulkanDevice::CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info) const
	{
		auto internal_state = Protean::Allocator::make_shared<PipelineState_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		pso->internal_state = internal_state;
		pso->desc = *desc;

		const RasterizerState& rs_desc = pso->desc.rs != nullptr ? *pso->desc.rs : default_rasterizerstate;
		const DepthStencilState& dss_desc = pso->desc.dss != nullptr ? *pso->desc.dss : default_depthstencilstate;
		const BlendState& bs_desc = pso->desc.bs != nullptr ? *pso->desc.bs : default_blendstate;

		internal_state->layout = layout_template;

		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		VkPipelineShaderStageCreateInfo shaderStages[size_t(ShaderStage_Count)] = {};
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		VkPipelineRasterizationDepthClipStateCreateInfoEXT depthClipStateInfo = {};
		VkPipelineRasterizationConservativeStateCreateInfoEXT rasterizationConservativeState = {};
		VkPipelineViewportStateCreateInfo viewportState = {};
		VkPipelineDepthStencilStateCreateInfo depthstencil = {};
		VkSampleMask samplemask = {};
		VkPipelineTessellationStateCreateInfo tessellationInfo = {};

		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		if (CheckCapability(GraphicsDeviceCapability_VARIABLE_RATE_SHADING_TIER2))
		{
			pipelineInfo.flags |= VK_PIPELINE_CREATE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		}
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		auto insert_shader_layout = [&](Shader_Vulkan* shader_interal) {
			for (uint32_t i = 0; i < arraysize(shader_interal->layout.table.SRV); ++i)
			{
				if (internal_state->layout.table.SRV[i].descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
					continue; // it was already assigned to non-default, can't assign somethig else
				internal_state->layout.table.SRV[i] = shader_interal->layout.table.SRV[i];
				internal_state->layout.SRV_image_types[i] = shader_interal->layout.SRV_image_types[i];
			}
			for (uint32_t i = 0; i < arraysize(shader_interal->layout.table.UAV); ++i)
			{
				if (internal_state->layout.table.UAV[i].descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
					continue; // it was already assigned to non-default, can't assign somethig else
				internal_state->layout.table.UAV[i] = shader_interal->layout.table.UAV[i];
				internal_state->layout.UAV_image_types[i] = shader_interal->layout.UAV_image_types[i];
			}
		};

		uint32_t shaderStageCount = 0;
		if (pso->desc.ms != nullptr && pso->desc.ms->IsValid())
		{
			auto shader_internal = to_internal(pso->desc.ms);
			shaderStages[shaderStageCount++] = shader_internal->stageInfo;
			insert_shader_layout(shader_internal);
		}
		if (pso->desc.as != nullptr && pso->desc.as->IsValid())
		{
			auto shader_internal = to_internal(pso->desc.as);
			shaderStages[shaderStageCount++] = shader_internal->stageInfo;
			insert_shader_layout(shader_internal);
		}
		if (pso->desc.vs != nullptr && pso->desc.vs->IsValid())
		{
			auto shader_internal = to_internal(pso->desc.vs);
			shaderStages[shaderStageCount++] = shader_internal->stageInfo;
			insert_shader_layout(shader_internal);
		}
		if (pso->desc.hs != nullptr && pso->desc.hs->IsValid())
		{
			auto shader_internal = to_internal(pso->desc.hs);
			shaderStages[shaderStageCount++] = shader_internal->stageInfo;
			insert_shader_layout(shader_internal);
		}
		if (pso->desc.ds != nullptr && pso->desc.ds->IsValid())
		{
			auto shader_internal = to_internal(pso->desc.ds);
			shaderStages[shaderStageCount++] = shader_internal->stageInfo;
			insert_shader_layout(shader_internal);
		}
		if (pso->desc.gs != nullptr && pso->desc.gs->IsValid())
		{
			auto shader_internal = to_internal(pso->desc.gs);
			shaderStages[shaderStageCount++] = shader_internal->stageInfo;
			insert_shader_layout(shader_internal);
		}
		if (pso->desc.ps != nullptr && pso->desc.ps->IsValid())
		{
			auto shader_internal = to_internal(pso->desc.ps);
			shaderStages[shaderStageCount++] = shader_internal->stageInfo;
			insert_shader_layout(shader_internal);
		}
		pipelineInfo.stageCount = shaderStageCount;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.layout = cache_pso_layout(internal_state->layout);

		// Fixed function states:

		// Primitive type:
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		switch (pso->desc.pt)
		{
		case PrimitiveTopology_POINTLIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			break;
		case PrimitiveTopology_LINELIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
			break;
		case PrimitiveTopology_LINESTRIP:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
			break;
		case PrimitiveTopology_TRIANGLESTRIP:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
			break;
		case PrimitiveTopology_TRIANGLELIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			break;
		case PrimitiveTopology_PATCHLIST:
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			break;
		default:
			break;
		}
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		pipelineInfo.pInputAssemblyState = &inputAssembly;


		// Rasterizer:
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_TRUE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		const void** tail = &rasterizer.pNext;

		// depth clip will be enabled via Vulkan 1.1 extension VK_EXT_depth_clip_enable:
		depthClipStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
		depthClipStateInfo.depthClipEnable = VK_TRUE;
		if (depth_clip_enable_features.depthClipEnable == VK_TRUE)
		{
			*tail = &depthClipStateInfo;
			tail = &depthClipStateInfo.pNext;
		}

		switch (rs_desc.fill_mode)
		{
		case FillMode_WIREFRAME:
			rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
			break;
		case FillMode_SOLID:
		default:
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
			break;
		}

		switch (rs_desc.cull_mode)
		{
		case CullMode_BACK:
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			break;
		case CullMode_FRONT:
			rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
			break;
		case CullMode_NONE:
		default:
			rasterizer.cullMode = VK_CULL_MODE_NONE;
			break;
		}

		rasterizer.frontFace = rs_desc.front_counter_clockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = rs_desc.depth_bias != 0 || rs_desc.slope_scaled_depth_bias != 0;
		rasterizer.depthBiasConstantFactor = static_cast<float>(rs_desc.depth_bias);
		rasterizer.depthBiasClamp = rs_desc.depth_bias_clamp;
		rasterizer.depthBiasSlopeFactor = rs_desc.slope_scaled_depth_bias;

		// Depth clip will be enabled via Vulkan 1.1 extension VK_EXT_depth_clip_enable:
		depthClipStateInfo.depthClipEnable = rs_desc.depth_clip_enable ? VK_TRUE : VK_FALSE;

		if (CheckCapability(GraphicsDeviceCapability_CONSERVATIVE_RASTERIZATION) && rs_desc.conservative_rasterization_enable)
		{
			rasterizationConservativeState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
			rasterizationConservativeState.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
			rasterizationConservativeState.extraPrimitiveOverestimationSize = 0.0f;
			*tail = &rasterizationConservativeState;
			tail = &rasterizationConservativeState.pNext;
		}

		pipelineInfo.pRasterizationState = &rasterizer;

		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 0;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 0;
		viewportState.pScissors = nullptr;

		pipelineInfo.pViewportState = &viewportState;


		// Depth-Stencil:
		depthstencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthstencil.depthTestEnable = dss_desc.depth_enable ? VK_TRUE : VK_FALSE;
		depthstencil.depthWriteEnable = dss_desc.depth_write_mask == DepthWriteMask_ZERO ? VK_FALSE : VK_TRUE;
		depthstencil.depthCompareOp = _ConvertComparisonFunc(dss_desc.depth_func);

		if (dss_desc.stencil_enable)
		{
			depthstencil.stencilTestEnable = VK_TRUE;

			depthstencil.front.compareMask = dss_desc.stencil_read_mask;
			depthstencil.front.writeMask = dss_desc.stencil_write_mask;
			depthstencil.front.reference = 0; // runtime supplied
			depthstencil.front.compareOp = _ConvertComparisonFunc(dss_desc.front_face.stencil_func);
			depthstencil.front.passOp = _ConvertStencilOp(dss_desc.front_face.stencil_pass_op);
			depthstencil.front.failOp = _ConvertStencilOp(dss_desc.front_face.stencil_fail_op);
			depthstencil.front.depthFailOp = _ConvertStencilOp(dss_desc.front_face.stencil_depth_fail_op);

			depthstencil.back.compareMask = dss_desc.stencil_read_mask;
			depthstencil.back.writeMask = dss_desc.stencil_write_mask;
			depthstencil.back.reference = 0; // runtime supplied
			depthstencil.back.compareOp = _ConvertComparisonFunc(dss_desc.back_face.stencil_func);
			depthstencil.back.passOp = _ConvertStencilOp(dss_desc.back_face.stencil_pass_op);
			depthstencil.back.failOp = _ConvertStencilOp(dss_desc.back_face.stencil_fail_op);
			depthstencil.back.depthFailOp = _ConvertStencilOp(dss_desc.back_face.stencil_depth_fail_op);
		}
		else
		{
			depthstencil.stencilTestEnable = VK_FALSE;

			depthstencil.front.compareMask = 0;
			depthstencil.front.writeMask = 0;
			depthstencil.front.reference = 0;
			depthstencil.front.compareOp = VK_COMPARE_OP_NEVER;
			depthstencil.front.passOp = VK_STENCIL_OP_KEEP;
			depthstencil.front.failOp = VK_STENCIL_OP_KEEP;
			depthstencil.front.depthFailOp = VK_STENCIL_OP_KEEP;

			depthstencil.back.compareMask = 0;
			depthstencil.back.writeMask = 0;
			depthstencil.back.reference = 0; // runtime supplied
			depthstencil.back.compareOp = VK_COMPARE_OP_NEVER;
			depthstencil.back.passOp = VK_STENCIL_OP_KEEP;
			depthstencil.back.failOp = VK_STENCIL_OP_KEEP;
			depthstencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
		}

		if (CheckCapability(GraphicsDeviceCapability_DEPTH_BOUNDS_TEST))
		{
			depthstencil.depthBoundsTestEnable = dss_desc.depth_bounds_test_enable ? VK_TRUE : VK_FALSE;
		}
		else
		{
			depthstencil.depthBoundsTestEnable = VK_FALSE;
		}

		pipelineInfo.pDepthStencilState = &depthstencil;


		// Tessellation:
		tessellationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
		tessellationInfo.patchControlPoints = desc->patch_control_points;

		pipelineInfo.pTessellationState = &tessellationInfo;

		if (pso->desc.ms == nullptr)
		{
			pipelineInfo.pDynamicState = &dynamicStateInfo;
		}
		else
		{
			pipelineInfo.pDynamicState = &dynamicStateInfo_MeshShader;
		}

		VkResult res = VK_SUCCESS;

		if (renderpass_info != nullptr)
		{
			// MSAA:
			VkPipelineMultisampleStateCreateInfo multisampling = {};
			multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampling.sampleShadingEnable = VK_FALSE;
			multisampling.rasterizationSamples = (VkSampleCountFlagBits)renderpass_info->sample_count;
			if (rs_desc.forced_sample_count > 1)
			{
				multisampling.rasterizationSamples = (VkSampleCountFlagBits)rs_desc.forced_sample_count;
			}
			multisampling.minSampleShading = 1.0f;
			samplemask = pso->desc.sample_mask;
			multisampling.pSampleMask = &samplemask;
			multisampling.alphaToCoverageEnable = bs_desc.alpha_to_coverage_enable ? VK_TRUE : VK_FALSE;
			multisampling.alphaToOneEnable = VK_FALSE;

			pipelineInfo.pMultisampleState = &multisampling;


			// Blending:
			uint32_t numBlendAttachments = 0;
			VkPipelineColorBlendAttachmentState colorBlendAttachments[8] = {};
			for (size_t i = 0; i < renderpass_info->rt_count; ++i)
			{
				size_t attachmentIndex = 0;
				if (bs_desc.independent_blend_enable)
					attachmentIndex = i;

				const auto& desc = bs_desc.render_target[attachmentIndex];
				VkPipelineColorBlendAttachmentState& attachment = colorBlendAttachments[numBlendAttachments];
				numBlendAttachments++;

				attachment.blendEnable = desc.blend_enable ? VK_TRUE : VK_FALSE;

				attachment.colorWriteMask = 0;
				if (has_flag(desc.render_target_write_mask, ColorWrite_ENABLE_RED))
				{
					attachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
				}
				if (has_flag(desc.render_target_write_mask, ColorWrite_ENABLE_GREEN))
				{
					attachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
				}
				if (has_flag(desc.render_target_write_mask, ColorWrite_ENABLE_BLUE))
				{
					attachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
				}
				if (has_flag(desc.render_target_write_mask, ColorWrite_ENABLE_ALPHA))
				{
					attachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
				}

				attachment.srcColorBlendFactor = _ConvertBlend(desc.src_blend);
				attachment.dstColorBlendFactor = _ConvertBlend(desc.dest_blend);
				attachment.colorBlendOp = _ConvertBlendOp(desc.blend_op);
				attachment.srcAlphaBlendFactor = _ConvertBlend(desc.src_blend_alpha);
				attachment.dstAlphaBlendFactor = _ConvertBlend(desc.dest_blend_alpha);
				attachment.alphaBlendOp = _ConvertBlendOp(desc.blend_op_alpha);
			}

			VkPipelineColorBlendStateCreateInfo colorBlending = {};
			colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlending.logicOpEnable = VK_FALSE;
			colorBlending.logicOp = VK_LOGIC_OP_COPY;
			colorBlending.attachmentCount = numBlendAttachments;
			colorBlending.pAttachments = colorBlendAttachments;
			colorBlending.blendConstants[0] = 1.0f;
			colorBlending.blendConstants[1] = 1.0f;
			colorBlending.blendConstants[2] = 1.0f;
			colorBlending.blendConstants[3] = 1.0f;

			pipelineInfo.pColorBlendState = &colorBlending;

			// Input layout:
			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			static constexpr uint32_t max_il_elemet_count = 32;
			StackVector<VkVertexInputBindingDescription, max_il_elemet_count> bindings;
			StackVector<VkVertexInputAttributeDescription, max_il_elemet_count> attributes;
			if (pso->desc.il != nullptr)
			{
				uint32_t lastBinding = 0xFFFFFFFF;
				const uint32_t element_count = std::min((uint32_t)pso->desc.il->elements.size(), max_il_elemet_count);
				for (uint32_t i = 0; i < element_count; ++i)
				{
					auto& element = pso->desc.il->elements[i];
					if (element.input_slot == lastBinding)
						continue;
					lastBinding = element.input_slot;
					VkVertexInputBindingDescription& bind = bindings.emplace_back();
					bind.binding = element.input_slot;
					bind.inputRate = element.input_slot_class == InputClassification_PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
					bind.stride = GetFormatStride(element.format);
				}

				uint32_t offset = 0;
				lastBinding = 0xFFFFFFFF;
				for (uint32_t i = 0; i < element_count; ++i)
				{
					auto& element = pso->desc.il->elements[i];
					VkVertexInputAttributeDescription attr = {};
					attr.binding = element.input_slot;
					if (attr.binding != lastBinding)
					{
						lastBinding = attr.binding;
						offset = 0;
					}
					attr.format = _ConvertFormat(element.format);
					attr.location = i;
					attr.offset = element.aligned_byte_offset;
					if (attr.offset == InputLayout::APPEND_ALIGNED_ELEMENT)
					{
						// need to manually resolve this from the format spec.
						attr.offset = offset;
						offset += GetFormatStride(element.format);
					}

					attributes.push_back(attr);
				}

				vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
				vertexInputInfo.pVertexBindingDescriptions = bindings.data();
				vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
				vertexInputInfo.pVertexAttributeDescriptions = attributes.data();
			}
			pipelineInfo.pVertexInputState = &vertexInputInfo;

			pipelineInfo.renderPass = VK_NULL_HANDLE; // instead we use VkPipelineRenderingCreateInfo

			VkPipelineRenderingCreateInfo renderingInfo = {};
			renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
			renderingInfo.viewMask = 0;
			renderingInfo.colorAttachmentCount = renderpass_info->rt_count;
			VkFormat formats[8] = {};
			for (uint32_t i = 0; i < renderpass_info->rt_count; ++i)
			{
				formats[i] = _ConvertFormat(renderpass_info->rt_formats[i]);
			}
			renderingInfo.pColorAttachmentFormats = formats;
			renderingInfo.depthAttachmentFormat = _ConvertFormat(renderpass_info->ds_format);
			if (IsFormatStencilSupport(renderpass_info->ds_format))
			{
				renderingInfo.stencilAttachmentFormat = renderingInfo.depthAttachmentFormat;
			}
			pipelineInfo.pNext = &renderingInfo;

			res = vulkan_check(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &internal_state->pipeline));
		}

		return res == VK_SUCCESS;
	}

	bool VulkanDevice::CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const
	{
		auto internal_state = Protean::Allocator::make_shared<BVH_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		bvh->internal_state = internal_state;
		bvh->type = GPUResource::Type_RAYTRACING_ACCELERATION_STRUCTURE;
		bvh->desc = *desc;
		bvh->size = 0;

		internal_state->buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		internal_state->buildInfo.flags = 0;
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
		}
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_COMPACTION)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
		}
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_TRACE)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		}
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_PREFER_FAST_BUILD)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
		}
		if (bvh->desc.flags & RaytracingAccelerationStructureDesc::FLAG_MINIMIZE_MEMORY)
		{
			internal_state->buildInfo.flags |= VK_BUILD_ACCELERATION_STRUCTURE_LOW_MEMORY_BIT_KHR;
		}

		switch (desc->type)
		{
		case RaytracingAccelerationStructureDesc::Type_BOTTOMLEVEL:
		{
			internal_state->buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

			for (auto& x : desc->bottom_level.geometries)
			{
				internal_state->geometries.emplace_back();
				auto& geometry = internal_state->geometries.back();
				geometry = {};
				geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;

				internal_state->primitiveCounts.emplace_back();
				uint32_t& primitiveCount = internal_state->primitiveCounts.back();

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_TRIANGLES)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
					geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
					geometry.geometry.triangles.indexType = x.triangles.index_format == IndexBufferFormat_UINT16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
					geometry.geometry.triangles.maxVertex = x.triangles.vertex_count;
					geometry.geometry.triangles.vertexStride = x.triangles.vertex_stride;
					geometry.geometry.triangles.vertexFormat = _ConvertFormat(x.triangles.vertex_format);

					primitiveCount = x.triangles.index_count / 3;
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_PROCEDURAL_AABBS)
				{
					geometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
					geometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
					geometry.geometry.aabbs.stride = sizeof(float) * 6; // min - max corners

					primitiveCount = x.aabbs.count;
				}
			}


		}
		break;
		case RaytracingAccelerationStructureDesc::Type_TOPLEVEL:
		{
			internal_state->buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

			internal_state->geometries.emplace_back();
			auto& geometry = internal_state->geometries.back();
			geometry = {};
			geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
			geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
			geometry.geometry.instances.arrayOfPointers = VK_FALSE;

			internal_state->primitiveCounts.emplace_back();
			uint32_t& primitiveCount = internal_state->primitiveCounts.back();
			primitiveCount = desc->top_level.count;
		}
		break;
		}

		internal_state->buildInfo.geometryCount = (uint32_t)internal_state->geometries.size();
		internal_state->buildInfo.pGeometries = internal_state->geometries.data();

		internal_state->sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

		// Compute memory requirements:
		vkGetAccelerationStructureBuildSizesKHR(
			device,
			VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
			&internal_state->buildInfo,
			internal_state->primitiveCounts.data(),
			&internal_state->sizeInfo
		);

		// Backing memory as buffer:
		VkBufferCreateInfo bufferInfo = {};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = align(internal_state->sizeInfo.accelerationStructureSize, (VkDeviceSize)acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment) + std::max(internal_state->sizeInfo.buildScratchSize, internal_state->sizeInfo.updateScratchSize);
		bufferInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
		bufferInfo.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; // scratch
		bufferInfo.usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
		bufferInfo.flags = 0;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

		vulkan_check(vmaCreateBuffer(
			allocationhandler->allocator,
			&bufferInfo,
			&allocInfo,
			&internal_state->buffer,
			&internal_state->allocation,
			nullptr
		));

		// Create the acceleration structure:
		internal_state->createInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		internal_state->createInfo.type = internal_state->buildInfo.type;
		internal_state->createInfo.buffer = internal_state->buffer;
		internal_state->createInfo.size = internal_state->sizeInfo.accelerationStructureSize;

		VkResult res = vulkan_check(vkCreateAccelerationStructureKHR(
			device,
			&internal_state->createInfo,
			nullptr,
			&internal_state->resource
		));

		// Get the device address for the acceleration structure:
		VkAccelerationStructureDeviceAddressInfoKHR addrinfo = {};
		addrinfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		addrinfo.accelerationStructure = internal_state->resource;
		internal_state->as_address = vkGetAccelerationStructureDeviceAddressKHR(device, &addrinfo);

		// Get scratch address:
		VkBufferDeviceAddressInfo addressinfo = {};
		addressinfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		addressinfo.buffer = internal_state->buffer;
		internal_state->scratch_address = align(vkGetBufferDeviceAddress(device, &addressinfo) + internal_state->sizeInfo.accelerationStructureSize, (VkDeviceAddress)acceleration_structure_properties.minAccelerationStructureScratchOffsetAlignment);

		if (desc->type == RaytracingAccelerationStructureDesc::Type_TOPLEVEL)
		{
			internal_state->index = allocationhandler->bindlessAccelerationStructures.allocate();
			if (internal_state->index >= 0)
			{
				VkWriteDescriptorSetAccelerationStructureKHR acc = {};
				acc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				acc.accelerationStructureCount = 1;
				acc.pAccelerationStructures = &internal_state->resource;

				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				write.dstBinding = 0;
				write.dstArrayElement = internal_state->index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessAccelerationStructures.descriptorSet;
				write.pNext = &acc;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			}
		}

		bvh->size = bufferInfo.size;

		return res == VK_SUCCESS;
	}

	bool VulkanDevice::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const
	{
		auto internal_state = Protean::Allocator::make_shared<RTPipelineState_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		rtpso->internal_state = internal_state;
		rtpso->desc = *desc;

		VkRayTracingPipelineCreateInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
		info.flags = 0;

		Protean::vector<VkPipelineShaderStageCreateInfo> stages;
		for (auto& x : desc->shader_libraries)
		{
			auto shader_internal = to_internal(x.shader);
			info.layout = shader_internal->layout.pipeline_layout;
			stages.emplace_back();
			auto& stage = stages.back();
			stage = {};
			stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			stage.module = shader_internal->shaderModule;
			switch (x.type)
			{
			default:
			case ShaderLibrary::Type_RAYGENERATION:
				stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
				break;
			case ShaderLibrary::Type_MISS:
				stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
				break;
			case ShaderLibrary::Type_CLOSESTHIT:
				stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
				break;
			case ShaderLibrary::Type_ANYHIT:
				stage.stage = VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
				break;
			case ShaderLibrary::Type_INTERSECTION:
				stage.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
				break;
			}
			stage.pName = x.function_name.c_str();
		}
		info.stageCount = (uint32_t)stages.size();
		info.pStages = stages.data();

		Protean::vector<VkRayTracingShaderGroupCreateInfoKHR> groups;
		groups.reserve(desc->hit_groups.size());
		for (auto& x : desc->hit_groups)
		{
			groups.emplace_back();
			auto& group = groups.back();
			group = {};
			group.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			switch (x.type)
			{
			default:
			case ShaderHitGroup::Type_GENERAL:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
				break;
			case ShaderHitGroup::Type_TRIANGLES:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
				break;
			case ShaderHitGroup::Type_PROCEDURAL:
				group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
				break;
			}
			group.generalShader = x.general_shader;
			group.closestHitShader = x.closest_hit_shader;
			group.anyHitShader = x.any_hit_shader;
			group.intersectionShader = x.intersection_shader;
		}
		info.groupCount = (uint32_t)groups.size();
		info.pGroups = groups.data();

		info.maxPipelineRayRecursionDepth = desc->max_trace_recursion_depth;

		//VkRayTracingPipelineInterfaceCreateInfoKHR library_interface = {};
		//library_interface.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_INTERFACE_CREATE_INFO_KHR;
		//library_interface.maxPipelineRayPayloadSize = pDesc->max_payload_size_in_bytes;
		//library_interface.maxPipelineRayHitAttributeSize = pDesc->max_attribute_size_in_bytes;
		//info.pLibraryInterface = &library_interface;

		info.basePipelineHandle = VK_NULL_HANDLE;
		info.basePipelineIndex = 0;

		VkResult res = vulkan_check(vkCreateRayTracingPipelinesKHR(
			device,
			VK_NULL_HANDLE,
			pipelineCache,
			1,
			&info,
			nullptr,
			&internal_state->pipeline
		));

		return res == VK_SUCCESS;
	}

	bool VulkanDevice::CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const
	{
		auto internal_state = Protean::Allocator::make_shared<VideoDecoder_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		video_decoder->internal_state = internal_state;
		video_decoder->desc = *desc;

		if (video_decoder->desc.profile == VideoProfile_H264)
		{
			Protean::vector<StdVideoH264PictureParameterSet> pps_array_h264(desc->pps_count);
			Protean::vector<StdVideoH264ScalingLists> scalinglist_array_h264(desc->pps_count);
			for (uint32_t i = 0; i < desc->pps_count; ++i)
			{
				const h264::PPS* pps = (const h264::PPS*)desc->pps_datas + i;
				StdVideoH264PictureParameterSet& vk_pps = pps_array_h264[i];
				StdVideoH264ScalingLists& vk_scalinglist = scalinglist_array_h264[i];

				vk_pps.flags.transform_8x8_mode_flag = pps->transform_8x8_mode_flag;
				vk_pps.flags.redundant_pic_cnt_present_flag = pps->redundant_pic_cnt_present_flag;
				vk_pps.flags.constrained_intra_pred_flag = pps->constrained_intra_pred_flag;
				vk_pps.flags.deblocking_filter_control_present_flag = pps->deblocking_filter_control_present_flag;
				vk_pps.flags.weighted_pred_flag = pps->weighted_pred_flag;
				vk_pps.flags.bottom_field_pic_order_in_frame_present_flag = pps->pic_order_present_flag;
				vk_pps.flags.entropy_coding_mode_flag = pps->entropy_coding_mode_flag;
				vk_pps.flags.pic_scaling_matrix_present_flag = pps->pic_scaling_matrix_present_flag;

				vk_pps.seq_parameter_set_id = pps->seq_parameter_set_id;
				vk_pps.pic_parameter_set_id = pps->pic_parameter_set_id;
				vk_pps.num_ref_idx_l0_default_active_minus1 = pps->num_ref_idx_l0_active_minus1;
				vk_pps.num_ref_idx_l1_default_active_minus1 = pps->num_ref_idx_l1_active_minus1;
				vk_pps.weighted_bipred_idc = (StdVideoH264WeightedBipredIdc)pps->weighted_bipred_idc;
				vk_pps.pic_init_qp_minus26 = pps->pic_init_qp_minus26;
				vk_pps.pic_init_qs_minus26 = pps->pic_init_qs_minus26;
				vk_pps.chroma_qp_index_offset = pps->chroma_qp_index_offset;
				vk_pps.second_chroma_qp_index_offset = pps->second_chroma_qp_index_offset;

				vk_pps.pScalingLists = &vk_scalinglist;
				for (int j = 0; j < arraysize(pps->pic_scaling_list_present_flag); ++j)
				{
					vk_scalinglist.scaling_list_present_mask |= pps->pic_scaling_list_present_flag[j] << j;
				}
				for (int j = 0; j < arraysize(pps->UseDefaultScalingMatrix4x4Flag); ++j)
				{
					vk_scalinglist.use_default_scaling_matrix_mask |= pps->UseDefaultScalingMatrix4x4Flag[j] << j;
				}
				for (int j = 0; j < arraysize(pps->ScalingList4x4); ++j)
				{
					for (int k = 0; k < arraysize(pps->ScalingList4x4[j]); ++k)
					{
						vk_scalinglist.ScalingList4x4[j][k] = (uint8_t)pps->ScalingList4x4[j][k];
					}
				}
				for (int j = 0; j < arraysize(pps->ScalingList8x8); ++j)
				{
					for (int k = 0; k < arraysize(pps->ScalingList8x8[j]); ++k)
					{
						vk_scalinglist.ScalingList8x8[j][k] = (uint8_t)pps->ScalingList8x8[j][k];
					}
				}
			}

			uint32_t num_reference_frames = 0;
			Protean::vector<StdVideoH264SequenceParameterSet> sps_array_h264(desc->sps_count);
			Protean::vector<StdVideoH264SequenceParameterSetVui> vui_array_h264(desc->sps_count);
			Protean::vector<StdVideoH264HrdParameters> hrd_array_h264(desc->sps_count);
			for (uint32_t i = 0; i < desc->sps_count; ++i)
			{
				const h264::SPS* sps = (const h264::SPS*)desc->sps_datas + i;
				StdVideoH264SequenceParameterSet& vk_sps = sps_array_h264[i];

				vk_sps.flags.constraint_set0_flag = sps->constraint_set0_flag;
				vk_sps.flags.constraint_set1_flag = sps->constraint_set1_flag;
				vk_sps.flags.constraint_set2_flag = sps->constraint_set2_flag;
				vk_sps.flags.constraint_set3_flag = sps->constraint_set3_flag;
				vk_sps.flags.constraint_set4_flag = sps->constraint_set4_flag;
				vk_sps.flags.constraint_set5_flag = sps->constraint_set5_flag;
				vk_sps.flags.direct_8x8_inference_flag = sps->direct_8x8_inference_flag;
				vk_sps.flags.mb_adaptive_frame_field_flag = sps->mb_adaptive_frame_field_flag;
				vk_sps.flags.frame_mbs_only_flag = sps->frame_mbs_only_flag;
				vk_sps.flags.delta_pic_order_always_zero_flag = sps->delta_pic_order_always_zero_flag;
				vk_sps.flags.separate_colour_plane_flag = sps->separate_colour_plane_flag;
				vk_sps.flags.gaps_in_frame_num_value_allowed_flag = sps->gaps_in_frame_num_value_allowed_flag;
				vk_sps.flags.qpprime_y_zero_transform_bypass_flag = sps->qpprime_y_zero_transform_bypass_flag;
				vk_sps.flags.frame_cropping_flag = sps->frame_cropping_flag;
				vk_sps.flags.seq_scaling_matrix_present_flag = sps->seq_scaling_matrix_present_flag;
				vk_sps.flags.vui_parameters_present_flag = sps->vui_parameters_present_flag;

				if (vk_sps.flags.vui_parameters_present_flag)
				{
					StdVideoH264SequenceParameterSetVui& vk_vui = vui_array_h264[i];
					vk_sps.pSequenceParameterSetVui = &vk_vui;
					vk_vui.flags.aspect_ratio_info_present_flag = sps->vui.aspect_ratio_info_present_flag;
					vk_vui.flags.overscan_info_present_flag = sps->vui.overscan_info_present_flag;
					vk_vui.flags.overscan_appropriate_flag = sps->vui.overscan_appropriate_flag;
					vk_vui.flags.video_signal_type_present_flag = sps->vui.video_signal_type_present_flag;
					vk_vui.flags.video_full_range_flag = sps->vui.video_full_range_flag;
					vk_vui.flags.color_description_present_flag = sps->vui.colour_description_present_flag;
					vk_vui.flags.chroma_loc_info_present_flag = sps->vui.chroma_loc_info_present_flag;
					vk_vui.flags.timing_info_present_flag = sps->vui.timing_info_present_flag;
					vk_vui.flags.fixed_frame_rate_flag = sps->vui.fixed_frame_rate_flag;
					vk_vui.flags.bitstream_restriction_flag = sps->vui.bitstream_restriction_flag;
					vk_vui.flags.nal_hrd_parameters_present_flag = sps->vui.nal_hrd_parameters_present_flag;
					vk_vui.flags.vcl_hrd_parameters_present_flag = sps->vui.vcl_hrd_parameters_present_flag;

					vk_vui.aspect_ratio_idc = (StdVideoH264AspectRatioIdc)sps->vui.aspect_ratio_idc;
					vk_vui.sar_width = sps->vui.sar_width;
					vk_vui.sar_height = sps->vui.sar_height;
					vk_vui.video_format = sps->vui.video_format;
					vk_vui.colour_primaries = sps->vui.colour_primaries;
					vk_vui.transfer_characteristics = sps->vui.transfer_characteristics;
					vk_vui.matrix_coefficients = sps->vui.matrix_coefficients;
					vk_vui.num_units_in_tick = sps->vui.num_units_in_tick;
					vk_vui.time_scale = sps->vui.time_scale;
					vk_vui.max_num_reorder_frames = sps->vui.num_reorder_frames;
					vk_vui.max_dec_frame_buffering = sps->vui.max_dec_frame_buffering;
					vk_vui.chroma_sample_loc_type_top_field = sps->vui.chroma_sample_loc_type_top_field;
					vk_vui.chroma_sample_loc_type_bottom_field = sps->vui.chroma_sample_loc_type_bottom_field;

					StdVideoH264HrdParameters& vk_hrd = hrd_array_h264[i];
					vk_vui.pHrdParameters = &vk_hrd;
					vk_hrd.cpb_cnt_minus1 = sps->hrd.cpb_cnt_minus1;
					vk_hrd.bit_rate_scale = sps->hrd.bit_rate_scale;
					vk_hrd.cpb_size_scale = sps->hrd.cpb_size_scale;
					for (int j = 0; j < arraysize(sps->hrd.bit_rate_value_minus1); ++j)
					{
						vk_hrd.bit_rate_value_minus1[j] = sps->hrd.bit_rate_value_minus1[j];
						vk_hrd.cpb_size_value_minus1[j] = sps->hrd.cpb_size_value_minus1[j];
						vk_hrd.cbr_flag[j] = sps->hrd.cbr_flag[j];
					}
					vk_hrd.initial_cpb_removal_delay_length_minus1 = sps->hrd.initial_cpb_removal_delay_length_minus1;
					vk_hrd.cpb_removal_delay_length_minus1 = sps->hrd.cpb_removal_delay_length_minus1;
					vk_hrd.dpb_output_delay_length_minus1 = sps->hrd.dpb_output_delay_length_minus1;
					vk_hrd.time_offset_length = sps->hrd.time_offset_length;
				}

				vk_sps.profile_idc = (StdVideoH264ProfileIdc)sps->profile_idc;
				switch (sps->level_idc)
				{
				case 0:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_1_0;
					break;
				case 11:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_1_1;
					break;
				case 12:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_1_2;
					break;
				case 13:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_1_3;
					break;
				case 20:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_2_0;
					break;
				case 21:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_2_1;
					break;
				case 22:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_2_2;
					break;
				case 30:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_3_0;
					break;
				case 31:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_3_1;
					break;
				case 32:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_3_2;
					break;
				case 40:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_4_0;
					break;
				case 41:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_4_1;
					break;
				case 42:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_4_2;
					break;
				case 50:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_5_0;
					break;
				case 51:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_5_1;
					break;
				case 52:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_5_2;
					break;
				case 60:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_6_0;
					break;
				case 61:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_6_1;
					break;
				case 62:
					vk_sps.level_idc = STD_VIDEO_H264_LEVEL_IDC_6_2;
					break;
				default:
					assert(0);
					break;
				}
				assert(vk_sps.level_idc <= decode_h264_capabilities.maxLevelIdc);
				//vk_sps.chroma_format_idc = (StdVideoH264ChromaFormatIdc)sps->chroma_format_idc;
				vk_sps.chroma_format_idc = STD_VIDEO_H264_CHROMA_FORMAT_IDC_420; // only one we support currently
				vk_sps.seq_parameter_set_id = sps->seq_parameter_set_id;
				vk_sps.bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
				vk_sps.bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
				vk_sps.log2_max_frame_num_minus4 = sps->log2_max_frame_num_minus4;
				vk_sps.pic_order_cnt_type = (StdVideoH264PocType)sps->pic_order_cnt_type;
				vk_sps.offset_for_non_ref_pic = sps->offset_for_non_ref_pic;
				vk_sps.offset_for_top_to_bottom_field = sps->offset_for_top_to_bottom_field;
				vk_sps.log2_max_pic_order_cnt_lsb_minus4 = sps->log2_max_pic_order_cnt_lsb_minus4;
				vk_sps.num_ref_frames_in_pic_order_cnt_cycle = sps->num_ref_frames_in_pic_order_cnt_cycle;
				vk_sps.max_num_ref_frames = sps->num_ref_frames;
				vk_sps.pic_width_in_mbs_minus1 = sps->pic_width_in_mbs_minus1;
				vk_sps.pic_height_in_map_units_minus1 = sps->pic_height_in_map_units_minus1;
				vk_sps.frame_crop_left_offset = sps->frame_crop_left_offset;
				vk_sps.frame_crop_right_offset = sps->frame_crop_right_offset;
				vk_sps.frame_crop_top_offset = sps->frame_crop_top_offset;
				vk_sps.frame_crop_bottom_offset = sps->frame_crop_bottom_offset;
				vk_sps.pOffsetForRefFrame = sps->offset_for_ref_frame;

				num_reference_frames = std::max(num_reference_frames, (uint32_t)sps->num_ref_frames);
			}

			num_reference_frames = std::min(num_reference_frames, video_capability_h264.video_capabilities.maxActiveReferencePictures);

			VkVideoDecodeH264SessionParametersAddInfoKHR session_parameters_add_info_h264 = {};
			session_parameters_add_info_h264.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_ADD_INFO_KHR;
			session_parameters_add_info_h264.stdPPSCount = (uint32_t)pps_array_h264.size();
			session_parameters_add_info_h264.pStdPPSs = pps_array_h264.data();
			session_parameters_add_info_h264.stdSPSCount = (uint32_t)sps_array_h264.size();
			session_parameters_add_info_h264.pStdSPSs = sps_array_h264.data();

			VkVideoSessionCreateInfoKHR info = {};
			info.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_CREATE_INFO_KHR;
			info.queueFamilyIndex = videoFamily;
			info.maxActiveReferencePictures = num_reference_frames * 2; // *2: top and bottom field counts as two I think: https://vulkan.lunarg.com/doc/view/1.3.239.0/windows/1.3-extensions/vkspec.html#_video_decode_commands
			info.maxDpbSlots = std::min(desc->num_dpb_slots, video_capability_h264.video_capabilities.maxDpbSlots);
			info.maxCodedExtent.width = std::min(desc->width, video_capability_h264.video_capabilities.maxCodedExtent.width);
			info.maxCodedExtent.height = std::min(desc->height, video_capability_h264.video_capabilities.maxCodedExtent.height);
			info.pictureFormat = _ConvertFormat(desc->format);
			info.referencePictureFormat = info.pictureFormat;
			info.pVideoProfile = &video_capability_h264.profile;
			info.pStdHeaderVersion = &video_capability_h264.video_capabilities.stdHeaderVersion;

			vulkan_check(vkCreateVideoSessionKHR(device, &info, nullptr, &internal_state->video_session));

			uint32_t requirement_count = 0;
			vulkan_check(vkGetVideoSessionMemoryRequirementsKHR(device, internal_state->video_session, &requirement_count, nullptr));

			Protean::vector<VkVideoSessionMemoryRequirementsKHR> requirements(requirement_count);
			for (auto& x : requirements)
			{
				x.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_MEMORY_REQUIREMENTS_KHR;
			}
			vulkan_check(vkGetVideoSessionMemoryRequirementsKHR(device, internal_state->video_session, &requirement_count, requirements.data()));

			internal_state->allocations.resize(requirement_count);
			Protean::vector<VkBindVideoSessionMemoryInfoKHR> bind_session_memory_infos(requirement_count);
			for (uint32_t i = 0; i < requirement_count; ++i)
			{
				const VkVideoSessionMemoryRequirementsKHR& video_req = requirements[i];
				VmaAllocationCreateInfo alloc_create_info = {};
				alloc_create_info.memoryTypeBits = video_req.memoryRequirements.memoryTypeBits;
				VmaAllocationInfo alloc_info = {};

				vulkan_check(vmaAllocateMemory(
					allocationhandler->allocator,
					&video_req.memoryRequirements,
					&alloc_create_info,
					&internal_state->allocations[i],
					&alloc_info
				));

				VkBindVideoSessionMemoryInfoKHR& bind_info = bind_session_memory_infos[i];
				bind_info.sType = VK_STRUCTURE_TYPE_BIND_VIDEO_SESSION_MEMORY_INFO_KHR;
				bind_info.memory = alloc_info.deviceMemory;
				bind_info.memoryBindIndex = video_req.memoryBindIndex;
				bind_info.memoryOffset = alloc_info.offset;
				bind_info.memorySize = alloc_info.size;
			}
			vulkan_check(vkBindVideoSessionMemoryKHR(device, internal_state->video_session, requirement_count, bind_session_memory_infos.data()));

			VkVideoDecodeH264SessionParametersCreateInfoKHR session_parameters_info_h264 = {};
			session_parameters_info_h264.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_SESSION_PARAMETERS_CREATE_INFO_KHR;
			session_parameters_info_h264.maxStdPPSCount = (uint32_t)pps_array_h264.size();
			session_parameters_info_h264.maxStdSPSCount = (uint32_t)sps_array_h264.size();
			session_parameters_info_h264.pParametersAddInfo = &session_parameters_add_info_h264;

			VkVideoSessionParametersCreateInfoKHR session_parameters_info = {};
			session_parameters_info.sType = VK_STRUCTURE_TYPE_VIDEO_SESSION_PARAMETERS_CREATE_INFO_KHR;
			session_parameters_info.videoSession = internal_state->video_session;
			session_parameters_info.videoSessionParametersTemplate = VK_NULL_HANDLE;
			session_parameters_info.pNext = &session_parameters_info_h264;
			vulkan_check(vkCreateVideoSessionParametersKHR(device, &session_parameters_info, nullptr, &internal_state->session_parameters));

			video_decoder->support = {};
			if (video_capability_h264.decode_capabilities.flags & VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_COINCIDE_BIT_KHR)
			{
				video_decoder->support |= VideoDecoderSupportFlags_DPB_AND_OUTPUT_COINCIDE;
			}
			if (video_capability_h264.decode_capabilities.flags & VK_VIDEO_DECODE_CAPABILITY_DPB_AND_OUTPUT_DISTINCT_BIT_KHR)
			{
				video_decoder->support |= VideoDecoderSupportFlags_DPB_AND_OUTPUT_DISTINCT;
			}
			video_decoder->support |= VideoDecoderSupportFlags_DPB_INDIVIDUAL_TEXTURES_SUPPORTED;
		}
		else if (video_decoder->desc.profile == VideoProfile_H265)
		{
			assert(0); // TODO
			return false;
		}

		return true;
	}

	int VulkanDevice::CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change, const ImageAspect* aspect, const Swizzle* swizzle, float min_lod_clamp) const
	{
		auto internal_state = to_internal(texture);

		Format format = texture->GetDesc().format;
		if (format_change != nullptr)
		{
			format = *format_change;
		}

		Texture_Vulkan::TextureSubresource subresource;
		subresource.firstMip = firstMip;
		subresource.mipCount = mipCount;
		subresource.firstSlice = firstSlice;
		subresource.sliceCount = sliceCount;

		VkImageViewCreateInfo view_desc = {};
		view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_desc.flags = 0;
		view_desc.image = internal_state->resource;
		view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		if (aspect != nullptr)
		{
			view_desc.subresourceRange.aspectMask = _ConvertImageAspect(*aspect);
		}
		view_desc.subresourceRange.baseArrayLayer = firstSlice;
		view_desc.subresourceRange.layerCount = sliceCount;
		view_desc.subresourceRange.baseMipLevel = firstMip;
		view_desc.subresourceRange.levelCount = mipCount;
		if (type == SubresourceType_SRV)
		{
			view_desc.components = _ConvertSwizzle(swizzle == nullptr ? texture->desc.swizzle : *swizzle);
		}
		switch (format)
		{
		case Format_NV12:
			if (view_desc.subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT)
			{
				view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT;
				view_desc.format = VK_FORMAT_R8_UNORM;
			}
			else if (view_desc.subresourceRange.aspectMask == VK_IMAGE_ASPECT_PLANE_0_BIT)
			{
				view_desc.format = VK_FORMAT_R8_UNORM;
			}
			else if (view_desc.subresourceRange.aspectMask == VK_IMAGE_ASPECT_PLANE_1_BIT)
			{
				view_desc.format = VK_FORMAT_R8G8_UNORM;
			}
			break;
		default:
			view_desc.format = _ConvertFormat(format);
			break;
		}

		if (texture->desc.type == TextureDesc::Type_TEXTURE_1D)
		{
			if (texture->desc.array_size > 1)
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			}
			else
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_1D;
			}
		}
		else if (texture->desc.type == TextureDesc::Type_TEXTURE_2D)
		{
			if (texture->desc.array_size > 1)
			{
				if (has_flag(texture->desc.misc_flags, ResourceMiscFlag_TEXTURECUBE))
				{
					if (texture->desc.array_size > 6 && sliceCount > 6)
					{
						view_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
					}
					else
					{
						view_desc.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
					}
				}
				else
				{
					view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				}
			}
			else
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
			}
		}
		else if (texture->desc.type == TextureDesc::Type_TEXTURE_3D)
		{
			view_desc.viewType = VK_IMAGE_VIEW_TYPE_3D;
		}

		switch (type)
		{
		case SubresourceType_SRV:
		{
			if (IsFormatDepthSupport(format))
			{
				view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			}

			// This is required in cases where image was created with eg. USAGE_STORAGE, but
			//	the view format that we are creating doesn't support USAGE_STORAGE (for examplle: SRGB formats)
			VkImageViewUsageCreateInfo viewUsageInfo = {};
			viewUsageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
			viewUsageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			view_desc.pNext = &viewUsageInfo;

			VkImageViewMinLodCreateInfoEXT min_lod_info = {};
			min_lod_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_MIN_LOD_CREATE_INFO_EXT;
			min_lod_info.minLod = min_lod_clamp;
			if (min_lod_clamp > 0 && image_view_min_lod_features.minLod == VK_TRUE)
			{
				viewUsageInfo.pNext = &min_lod_info;
			}

			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &subresource.image_view);

			subresource.index = allocationhandler->bindlessSampledImages.allocate();
			if (subresource.index >= 0)
			{
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageView = subresource.image_view;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
				write.dstBinding = 0;
				write.dstArrayElement = subresource.index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessSampledImages.descriptorSet;
				write.pImageInfo = &imageInfo;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			}

			if (res == VK_SUCCESS)
			{
				if (!internal_state->srv.IsValid())
				{
					internal_state->srv = subresource;
					return -1;
				}
				internal_state->subresources_srv.push_back(subresource);
				return int(internal_state->subresources_srv.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case SubresourceType_UAV:
		{
			if (view_desc.viewType == VK_IMAGE_VIEW_TYPE_CUBE || view_desc.viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
			{
				view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			}

			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &subresource.image_view);

			subresource.index = allocationhandler->bindlessStorageImages.allocate();
			if (subresource.index >= 0)
			{
				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageView = subresource.image_view;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				VkWriteDescriptorSet write = {};
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				write.dstBinding = 0;
				write.dstArrayElement = subresource.index;
				write.descriptorCount = 1;
				write.dstSet = allocationhandler->bindlessStorageImages.descriptorSet;
				write.pImageInfo = &imageInfo;
				vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
			}

			if (res == VK_SUCCESS)
			{
				if (!internal_state->uav.IsValid())
				{
					internal_state->uav = subresource;
					return -1;
				}
				internal_state->subresources_uav.push_back(subresource);
				return int(internal_state->subresources_uav.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case SubresourceType_RTV:
		{
			view_desc.subresourceRange.levelCount = 1;
			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &subresource.image_view);

			if (res == VK_SUCCESS)
			{
				if (!internal_state->rtv.IsValid())
				{
					internal_state->rtv = subresource;
					internal_state->framebuffer_layercount = view_desc.subresourceRange.layerCount;
					return -1;
				}
				internal_state->subresources_rtv.push_back(subresource);
				return int(internal_state->subresources_rtv.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		case SubresourceType_DSV:
		{
			view_desc.subresourceRange.levelCount = 1;
			view_desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

			VkResult res = vkCreateImageView(device, &view_desc, nullptr, &subresource.image_view);

			if (res == VK_SUCCESS)
			{
				if (!internal_state->dsv.IsValid())
				{
					internal_state->dsv = subresource;
					internal_state->framebuffer_layercount = view_desc.subresourceRange.layerCount;
					return -1;
				}
				internal_state->subresources_dsv.push_back(subresource);
				return int(internal_state->subresources_dsv.size() - 1);
			}
			else
			{
				assert(0);
			}
		}
		break;
		default:
			break;
		}
		return -1;
	}

	int VulkanDevice::CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size, const Format* format_change, const uint32_t* structuredbuffer_stride_change) const
	{
		auto internal_state = to_internal(buffer);
		const GPUBufferDesc& desc = buffer->GetDesc();
		VkResult res;

		Buffer_Vulkan::BufferSubresource subresource;

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
		case SubresourceType_UAV:
		{
			if (format == Format_UNKNOWN)
			{
				// Raw buffer
				subresource.index = allocationhandler->bindlessStorageBuffers.allocate();
				if (subresource.IsValid())
				{
					subresource.buffer_info.buffer = internal_state->resource;
					subresource.buffer_info.offset = offset;
					subresource.buffer_info.range = size;

					VkWriteDescriptorSet write = {};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					write.dstBinding = 0;
					write.dstArrayElement = subresource.index;
					write.descriptorCount = 1;
					write.dstSet = allocationhandler->bindlessStorageBuffers.descriptorSet;
					write.pBufferInfo = &subresource.buffer_info;
					vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
				}

				if (type == SubresourceType_SRV)
				{
					if (!internal_state->srv.IsValid())
					{
						internal_state->srv = subresource;
						return -1;
					}
					internal_state->subresources_srv.push_back(subresource);
					return int(internal_state->subresources_srv.size() - 1);
				}
				else
				{
					if (!internal_state->uav.IsValid())
					{
						internal_state->uav = subresource;
						return -1;
					}
					internal_state->subresources_uav.push_back(subresource);
					return int(internal_state->subresources_uav.size() - 1);
				}
			}
			else
			{
				// Typed buffer
				VkBufferViewCreateInfo srv_desc = {};
				srv_desc.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
				srv_desc.buffer = internal_state->resource;
				srv_desc.flags = 0;
				srv_desc.format = _ConvertFormat(format);
				srv_desc.offset = offset;
				srv_desc.range = std::min(size, (uint64_t)desc.size - srv_desc.offset);

				res = vkCreateBufferView(device, &srv_desc, nullptr, &subresource.buffer_view);

				if (res == VK_SUCCESS)
				{
					if (type == SubresourceType_SRV)
					{
						subresource.index = allocationhandler->bindlessUniformTexelBuffers.allocate();
						if (subresource.IsValid())
						{
							VkWriteDescriptorSet write = {};
							write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
							write.dstBinding = 0;
							write.dstArrayElement = subresource.index;
							write.descriptorCount = 1;
							write.dstSet = allocationhandler->bindlessUniformTexelBuffers.descriptorSet;
							write.pTexelBufferView = &subresource.buffer_view;
							vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
						}

						if (!internal_state->srv.IsValid())
						{
							internal_state->srv = subresource;
							return -1;
						}
						internal_state->subresources_srv.push_back(subresource);
						return int(internal_state->subresources_srv.size() - 1);
					}
					else
					{
						subresource.index = allocationhandler->bindlessStorageTexelBuffers.allocate();
						if (subresource.IsValid())
						{
							VkWriteDescriptorSet write = {};
							write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
							write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
							write.dstBinding = 0;
							write.dstArrayElement = subresource.index;
							write.descriptorCount = 1;
							write.dstSet = allocationhandler->bindlessStorageTexelBuffers.descriptorSet;
							write.pTexelBufferView = &subresource.buffer_view;
							vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
						}

						if (!internal_state->uav.IsValid())
						{
							internal_state->uav = subresource;
							return -1;
						}
						internal_state->subresources_uav.push_back(subresource);
						return int(internal_state->subresources_uav.size() - 1);
					}
				}
				else
				{
					assert(0);
				}
			}
		}
		break;
		default:
			assert(0);
			break;
		}
		return -1;
	}

	void VulkanDevice::DeleteSubresources(GPUResource* resource)
	{
		if (resource->IsTexture())
		{
			auto internal_state = to_internal((Texture*)resource);
			internal_state->allocationhandler->destroylocker.lock();
			internal_state->destroy_subresources();
			internal_state->allocationhandler->destroylocker.unlock();
		}
		else if (resource->IsBuffer())
		{
			auto internal_state = to_internal((GPUBuffer*)resource);
			internal_state->allocationhandler->destroylocker.lock();
			internal_state->destroy_subresources();
			internal_state->allocationhandler->destroylocker.unlock();
		}
	}

	int VulkanDevice::GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource) const
	{
		if (resource == nullptr || !resource->IsValid())
			return -1;

		switch (type)
		{
		default:
		case SubresourceType_SRV:
			if (resource->IsBuffer())
			{
				const auto internal_state = to_internal<GPUBuffer>(resource);
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
			}
			else if (resource->IsTexture())
			{
				const auto internal_state = to_internal<Texture>(resource);
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
			}
			else if (resource->IsAccelerationStructure())
			{
				const auto internal_state = to_internal<RaytracingAccelerationStructure>(resource);
				return internal_state->index;
			}
			break;
		case SubresourceType_UAV:
			if (resource->IsBuffer())
			{
				const auto internal_state = to_internal<GPUBuffer>(resource);
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
			}
			else if (resource->IsTexture())
			{
				const auto internal_state = to_internal<Texture>(resource);
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
			}
			break;
		}

		return -1;
	}

	int VulkanDevice::GetDescriptorIndex(const Sampler* sampler) const
	{
		if (sampler == nullptr || !sampler->IsValid())
			return -1;

		auto internal_state = to_internal(sampler);
		return internal_state->index;
	}

	void VulkanDevice::WriteShadingRateValue(ShadingRate rate, void* dest) const
	{
		// How to compute shading rate value texel data:
		// https://www.khronos.org/registry/vulkan/specs/1.2-extensions/html/vkspec.html#primsrast-fragment-shading-rate-attachment

		switch (rate)
		{
		default:
		case ShadingRate_RATE_1X1:
			*(uint8_t*)dest = 0;
			break;
		case ShadingRate_RATE_1X2:
			*(uint8_t*)dest = 0x1;
			break;
		case ShadingRate_RATE_2X1:
			*(uint8_t*)dest = 0x4;
			break;
		case ShadingRate_RATE_2X2:
			*(uint8_t*)dest = 0x5;
			break;
		case ShadingRate_RATE_2X4:
			*(uint8_t*)dest = 0x6;
			break;
		case ShadingRate_RATE_4X2:
			*(uint8_t*)dest = 0x9;
			break;
		case ShadingRate_RATE_4X4:
			*(uint8_t*)dest = 0xa;
			break;
		}

	}

	void VulkanDevice::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const
	{
		VkAccelerationStructureInstanceKHR tmp = {};
		if (instance != nullptr)
		{
			tmp.transform = *(VkTransformMatrixKHR*)&instance->transform;
			tmp.instanceCustomIndex = instance->instance_id;
			tmp.mask = instance->instance_mask;
			tmp.instanceShaderBindingTableRecordOffset = instance->instance_contribution_to_hit_group_index;
			tmp.flags = instance->flags;

			assert(instance->bottom_level->IsAccelerationStructure());
			auto internal_state = to_internal((RaytracingAccelerationStructure*)instance->bottom_level);
			tmp.accelerationStructureReference = internal_state->as_address;
		}
		std::memcpy(dest, &tmp, sizeof(tmp)); // memcpy whole structure into mapped pointer to avoid read from uncached memory
	}

	void VulkanDevice::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
		vulkan_check(vkGetRayTracingShaderGroupHandlesKHR(device, to_internal(rtpso)->pipeline, group_index, 1, SHADER_IDENTIFIER_SIZE, dest));
	}

	void VulkanDevice::SetName(GPUResource* pResource, const char* name) const
	{
		if (!debugUtils || pResource == nullptr || !pResource->IsValid())
			return;

		VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.pObjectName = name;
		if (pResource->IsTexture())
		{
			info.objectType = VK_OBJECT_TYPE_IMAGE;
			info.objectHandle = (uint64_t)to_internal<Texture>(pResource)->resource;
		}
		else if (pResource->IsBuffer())
		{
			info.objectType = VK_OBJECT_TYPE_BUFFER;
			info.objectHandle = (uint64_t)to_internal<GPUBuffer>(pResource)->resource;
		}
		else if (pResource->IsAccelerationStructure())
		{
			info.objectType = VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR;
			info.objectHandle = (uint64_t)to_internal<RaytracingAccelerationStructure>(pResource)->resource;
		}

		if (info.objectHandle == (uint64_t)VK_NULL_HANDLE)
			return;

		vulkan_check(vkSetDebugUtilsObjectNameEXT(device, &info));
	}

	void VulkanDevice::SetName(Shader* shader, const char* name) const
	{
		if (!debugUtils || shader == nullptr || !shader->IsValid())
			return;

		VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.pObjectName = name;
		info.objectType = VK_OBJECT_TYPE_SHADER_MODULE;
		info.objectHandle = (uint64_t)to_internal(shader)->shaderModule;

		if (info.objectHandle == (uint64_t)VK_NULL_HANDLE)
			return;

		vulkan_check(vkSetDebugUtilsObjectNameEXT(device, &info));
	}

	SizeAlignment VulkanDevice::GetDeviceTextureMemoryRequirements(const TextureDesc* desc) const
	{
		VkVideoProfileListInfoKHR profile_list_info = {};
		profile_list_info.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_LIST_INFO_KHR;

		VkImageCreateInfo imageInfo = _ConvertTextureDesc(*desc, families.data(), (uint32_t)families.size(), profile_list_info, video_capability_h264.profile, video_capability_h265.profile);

		VkDeviceImageMemoryRequirements device_image_memory_requirements = {};
		device_image_memory_requirements.sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS;
		device_image_memory_requirements.pCreateInfo = &imageInfo;
		VkMemoryRequirements2 memory_requirements2 = {};
		memory_requirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		vkGetDeviceImageMemoryRequirements(device, &device_image_memory_requirements, &memory_requirements2);

		SizeAlignment ret;
		ret.size = memory_requirements2.memoryRequirements.size;
		ret.alignment = memory_requirements2.memoryRequirements.alignment;
		return ret;
	}

	void VulkanDevice::SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count)
	{
		thread_local Protean::vector<VkBindSparseInfo> sparse_infos;
		struct DataPerBind
		{
			VkSparseBufferMemoryBindInfo buffer_bind_info;
			VkSparseImageOpaqueMemoryBindInfo image_opaque_bind_info;
			VkSparseImageMemoryBindInfo image_bind_info;
			Protean::vector<VkSparseMemoryBind> memory_binds;
			Protean::vector<VkSparseImageMemoryBind> image_memory_binds;
		};
		thread_local Protean::vector<DataPerBind> sparse_binds;

		sparse_infos.resize(command_count);
		sparse_binds.resize(command_count);

		for (uint32_t i = 0; i < command_count; ++i)
		{
			const SparseUpdateCommand& in_command = commands[i];
			VkBindSparseInfo& out_info = sparse_infos[i];
			out_info = {};
			out_info.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;

			DataPerBind& out_bind = sparse_binds[i];

			VkDeviceMemory tile_pool_memory = VK_NULL_HANDLE;
			VkDeviceSize tile_pool_offset = 0;
			if (in_command.tile_pool != nullptr)
			{
				auto internal_tile_pool = to_internal(in_command.tile_pool);
				tile_pool_memory = GetAllocationMemory(internal_tile_pool->allocation);
				tile_pool_offset = GetAllocationOffset(internal_tile_pool->allocation);
			}

			out_bind.memory_binds.clear();
			out_bind.image_memory_binds.clear();

			out_bind.memory_binds.reserve(in_command.num_resource_regions);
			out_bind.image_memory_binds.reserve(in_command.num_resource_regions);

			const VkSparseMemoryBind* const memory_bind_ptr = out_bind.memory_binds.data();
			const VkSparseImageMemoryBind* const image_memory_bind_ptr = out_bind.image_memory_binds.data();

			if (in_command.sparse_resource->IsBuffer())
			{
				auto internal_sparse = to_internal<GPUBuffer>(in_command.sparse_resource);

				VkSparseBufferMemoryBindInfo& info = out_bind.buffer_bind_info;
				info = {};
				info.buffer = internal_sparse->resource;
				info.pBinds = memory_bind_ptr;
				info.bindCount = in_command.num_resource_regions;

				for (uint32_t j = 0; j < in_command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& in_coordinate = in_command.coordinates[j];
					const SparseRegionSize& in_size = in_command.sizes[j];

					const TileRangeFlags& in_flags = in_command.range_flags[j];
					uint32_t in_offset = in_command.range_start_offsets[j];
					uint32_t in_tile_count = in_command.range_tile_counts[j];
					VkSparseMemoryBind& out_memory_bind = out_bind.memory_binds.emplace_back();
					out_memory_bind = {};
					out_memory_bind.resourceOffset = in_coordinate.x * in_command.sparse_resource->sparse_page_size;
					out_memory_bind.size = in_tile_count * in_command.sparse_resource->sparse_page_size;
					if (in_flags == TileRangeFlags_Null)
					{
						out_memory_bind.memory = VK_NULL_HANDLE;
					}
					else
					{
						out_memory_bind.memory = tile_pool_memory;
						out_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
					}
				}

				if (info.bindCount > 0)
				{
					out_info.pBufferBinds = &out_bind.buffer_bind_info;
					out_info.bufferBindCount = 1;
				}
			}
			else if (in_command.sparse_resource->IsTexture())
			{
				const Texture* sparse_texture = (const Texture*)in_command.sparse_resource;
				const TextureDesc& texture_desc = sparse_texture->GetDesc();
				auto internal_sparse = to_internal(sparse_texture);

				VkImageAspectFlags aspectMask = {};
				if (IsFormatDepthSupport(texture_desc.format))
				{
					aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(texture_desc.format))
					{
						aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				if (has_flag(texture_desc.bind_flags, BindFlag_RENDER_TARGET) ||
					has_flag(texture_desc.bind_flags, BindFlag_SHADER_RESOURCE) ||
					has_flag(texture_desc.bind_flags, BindFlag_UNORDERED_ACCESS))
				{
					aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
				}

				VkSparseImageOpaqueMemoryBindInfo& opaque_info = out_bind.image_opaque_bind_info;
				opaque_info = {};
				opaque_info.image = internal_sparse->resource;
				opaque_info.pBinds = memory_bind_ptr;
				opaque_info.bindCount = 0;

				VkSparseImageMemoryBindInfo& info = out_bind.image_bind_info;
				info = {};
				info.image = internal_sparse->resource;
				info.pBinds = image_memory_bind_ptr;
				info.bindCount = 0;

				for (uint32_t j = 0; j < in_command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& in_coordinate = in_command.coordinates[j];
					const SparseRegionSize& in_size = in_command.sizes[j];
					const bool is_miptail = in_coordinate.mip >= internal_sparse->sparse_texture_properties.packed_mip_start;

					if (is_miptail)
					{
						opaque_info.bindCount++;

						const TileRangeFlags& in_flags = in_command.range_flags[j];
						uint32_t in_offset = in_command.range_start_offsets[j];
						uint32_t in_tile_count = in_command.range_tile_counts[j];
						VkSparseMemoryBind& out_memory_bind = out_bind.memory_binds.emplace_back();
						out_memory_bind = {};
						out_memory_bind.resourceOffset = internal_sparse->sparse_texture_properties.packed_mip_tile_offset * sparse_texture->sparse_page_size;
						out_memory_bind.size = in_tile_count * in_command.sparse_resource->sparse_page_size;
						if (in_flags == TileRangeFlags_Null)
						{
							out_memory_bind.memory = VK_NULL_HANDLE;
						}
						else
						{
							out_memory_bind.memory = tile_pool_memory;
							out_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
						}
					}
					else
					{
						info.bindCount++;

						const TileRangeFlags& in_flags = in_command.range_flags[j];
						uint32_t in_offset = in_command.range_start_offsets[j];
						uint32_t in_tile_count = in_command.range_tile_counts[j];
						VkSparseImageMemoryBind& out_image_memory_bind = out_bind.image_memory_binds.emplace_back();
						out_image_memory_bind = {};
						if (in_flags == TileRangeFlags_Null)
						{
							out_image_memory_bind.memory = VK_NULL_HANDLE;
						}
						else
						{
							out_image_memory_bind.memory = tile_pool_memory;
							out_image_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
						}
						out_image_memory_bind.subresource.mipLevel = in_coordinate.mip;
						out_image_memory_bind.subresource.arrayLayer = in_coordinate.slice;
						out_image_memory_bind.subresource.aspectMask = aspectMask;
						out_image_memory_bind.offset.x = in_coordinate.x * internal_sparse->sparse_texture_properties.tile_width;
						out_image_memory_bind.offset.y = in_coordinate.y * internal_sparse->sparse_texture_properties.tile_height;
						out_image_memory_bind.offset.z = in_coordinate.z * internal_sparse->sparse_texture_properties.tile_depth;
						out_image_memory_bind.extent.width = std::min(texture_desc.width, in_size.width * internal_sparse->sparse_texture_properties.tile_width);
						out_image_memory_bind.extent.height = std::min(texture_desc.height, in_size.height * internal_sparse->sparse_texture_properties.tile_height);
						out_image_memory_bind.extent.depth = std::min(texture_desc.depth, in_size.depth * internal_sparse->sparse_texture_properties.tile_depth);
					}

				}

				if (opaque_info.bindCount > 0)
				{
					out_info.pImageOpaqueBinds = &out_bind.image_opaque_bind_info;
					out_info.imageOpaqueBindCount = 1;
				}
				if (info.bindCount > 0)
				{
					out_info.pImageBinds = &out_bind.image_bind_info;
					out_info.imageBindCount = 1;
				}

			}

		}

		// Queue command:
		{
			CommandQueue* q = &queues[queue];
			if (!q->sparse_binding_supported)
			{
				// 1.) fall back to any sparse supporting queue
				q = &queue_sparse;
			}
			std::scoped_lock lock(*q->locker);
			wilog_assert(q->sparse_binding_supported, "Vulkan sparse mapping was used while the feature is not available! This can result in broken rendering or crash. Try to update the graphics driver if this happens.");

			vulkan_check(vkQueueBindSparse(q->queue, (uint32_t)sparse_infos.size(), sparse_infos.data(), VK_NULL_HANDLE));
		}
	}

	void VulkanDevice::CopyBufferAsync(const GPUBufferCopyCommand* commands, uint32_t command_count, const char* name) const
	{
		CopyAllocator::CopyCMD cmd = copyAllocator.allocate(0);

		if (name != nullptr && debugUtils)
		{
			VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
			label.pLabelName = name;
			label.color[0] = 0.0f;
			label.color[1] = 0.0f;
			label.color[2] = 0.0f;
			label.color[3] = 1.0f;
			vkCmdBeginDebugUtilsLabelEXT(cmd.transferCommandBuffer, &label);
		}

		for (uint32_t i = 0; i < command_count; ++i)
		{
			const GPUBufferCopyCommand& command = commands[i];
			auto dst_internal = to_internal(command.dst);
			auto src_internal = to_internal(command.src);
			VkBufferCopy copyRegion = {};
			copyRegion.size = command.size;
			copyRegion.srcOffset = command.src_offset;
			copyRegion.dstOffset = command.dst_offset;
			vkCmdCopyBuffer(
				cmd.transferCommandBuffer,
				src_internal->resource,
				dst_internal->resource,
				1,
				&copyRegion
			);
		}

		if (name != nullptr && debugUtils)
		{
			vkCmdEndDebugUtilsLabelEXT(cmd.transferCommandBuffer);
		}

		copyAllocator.submit(cmd, false);
	}
}

#endif // WICKEDENGINE_BUILD_VULKAN
