#define VOLK_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "rhi/vulkan/vulkan_internal.h"
// Vulkan backend: Device creation, frame submission and queries

#ifdef WICKEDENGINE_BUILD_VULKAN

namespace Protean::Graphics
{
	using namespace VulkanInternal;

	VkDeviceMemory VulkanInternal::GetAllocationMemory(VmaAllocation allocation) { return allocation->GetMemory(); }
	VkDeviceSize VulkanInternal::GetAllocationOffset(VmaAllocation allocation) { return allocation->GetOffset(); }
	void* VulkanInternal::GetAllocationMappedData(VmaAllocation allocation) { return allocation->GetMappedData(); }
	VkDeviceSize VulkanInternal::GetAllocationSize(VmaAllocation allocation) { return allocation->GetSize(); }

	void VulkanDevice::set_fence_name(VkFence fence, const char* name)
	{
		if (!debugUtils)
			return;
		if (fence == VK_NULL_HANDLE)
			return;

		VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.pObjectName = name;
		info.objectType = VK_OBJECT_TYPE_FENCE;
		info.objectHandle = (uint64_t)fence;

		vulkan_check(vkSetDebugUtilsObjectNameEXT(device, &info));
	}

	void VulkanDevice::set_semaphore_name(VkSemaphore semaphore, const char* name)
	{
		if (!debugUtils)
			return;
		if (semaphore == VK_NULL_HANDLE)
			return;

		VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.pObjectName = name;
		info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
		info.objectHandle = (uint64_t)semaphore;

		vulkan_check(vkSetDebugUtilsObjectNameEXT(device, &info));
	}

	VkPipelineLayout VulkanDevice::cache_pso_layout(PSOLayout& layout) const
	{
		PSOLayoutHash layout_hash;
		for (int i = 0; i < arraysize(layout.table.SRV); ++i)
		{
			layout_hash.SRV[i] = layout.table.SRV[i].descriptorType;
		}
		for (int i = 0; i < arraysize(layout.table.UAV); ++i)
		{
			layout_hash.UAV[i] = layout.table.UAV[i].descriptorType;
		}
		layout_hash.embed_hash();

		std::scoped_lock lck(layout_locker);
		auto it = pso_layouts.find(layout_hash);
		if (it != pso_layouts.end())
		{
			layout = it->second;
			return layout.pipeline_layout;
		}

		VkDescriptorSetLayout descriptor_set_layouts[DESCRIPTOR_SET_COUNT] = {};

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = sizeof(layout.table) / sizeof(VkDescriptorSetLayoutBinding);
		layoutInfo.pBindings = (const VkDescriptorSetLayoutBinding*)&layout.table;

		VkDescriptorBindingFlags bindingFlags[sizeof(layout.table) / sizeof(VkDescriptorSetLayoutBinding)] = {};
		for (auto& x : bindingFlags)
		{
			x =
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | // shuts up warning about format mismatches
				VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT; // descriptor sets will be reused after they are no longer used by GPU
		}
		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {};
		bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlagsInfo.bindingCount = layoutInfo.bindingCount;
		bindingFlagsInfo.pBindingFlags = bindingFlags;
		layoutInfo.pNext = &bindingFlagsInfo;

		vulkan_check(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout.descriptor_set_layout));

		descriptor_set_layouts[DESCRIPTOR_SET_BINDINGS] = layout.descriptor_set_layout;
		descriptor_set_layouts[DESCRIPTOR_SET_BINDLESS_SAMPLER] = allocationhandler->bindlessSamplers.descriptorSetLayout;
		descriptor_set_layouts[DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER] = allocationhandler->bindlessStorageBuffers.descriptorSetLayout;
		descriptor_set_layouts[DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER] = allocationhandler->bindlessUniformTexelBuffers.descriptorSetLayout;
		descriptor_set_layouts[DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE] = allocationhandler->bindlessSampledImages.descriptorSetLayout;
		descriptor_set_layouts[DESCRIPTOR_SET_BINDLESS_STORAGE_IMAGE] = allocationhandler->bindlessStorageImages.descriptorSetLayout;
		descriptor_set_layouts[DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER] = allocationhandler->bindlessStorageTexelBuffers.descriptorSetLayout;
		if (CheckCapability(GraphicsDeviceCapability_RAYTRACING))
		{
			descriptor_set_layouts[DESCRIPTOR_SET_BINDLESS_ACCELERATION_STRUCTURE] = allocationhandler->bindlessAccelerationStructures.descriptorSetLayout;
		}
		else
		{
			// Unused, set dummy sampler set:
			descriptor_set_layouts[DESCRIPTOR_SET_BINDLESS_ACCELERATION_STRUCTURE] = allocationhandler->bindlessSamplers.descriptorSetLayout;
		}

		VkPushConstantRange range = {};
		range.size = sizeof(uint32_t) * PUSH_CONSTANT_COUNT;
		range.stageFlags = VK_SHADER_STAGE_ALL;

		VkPipelineLayoutCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		info.pPushConstantRanges = &range;
		info.pushConstantRangeCount = 1;
		info.pSetLayouts = descriptor_set_layouts;
		info.setLayoutCount = arraysize(descriptor_set_layouts);
		vulkan_check(vkCreatePipelineLayout(device, &info, nullptr, &layout.pipeline_layout));

		static uint32_t next_layout_id = 0;
		layout.id = next_layout_id++;

		pso_layouts[layout_hash] = layout;

		return layout.pipeline_layout;
	}

	void VulkanDevice::CommandQueue::clear()
	{
		swapchain_updates.clear();
		submit_waitSemaphoreInfos.clear();
		submit_signalSemaphoreInfos.clear();
		submit_cmds.clear();

		swapchainWaitSemaphores.clear();
		swapchains.clear();
		swapchainImageIndices.clear();
	}
	void VulkanDevice::CommandQueue::signal(VkSemaphore semaphore)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		VkSemaphoreSubmitInfo& signalSemaphore = submit_signalSemaphoreInfos.emplace_back();
		signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphore.semaphore = semaphore;
		signalSemaphore.value = 0; // not a timeline semaphore
		signalSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}
	void VulkanDevice::CommandQueue::wait(VkSemaphore semaphore)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		VkSemaphoreSubmitInfo& waitSemaphore = submit_waitSemaphoreInfos.emplace_back();
		waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphore.semaphore = semaphore;
		waitSemaphore.value = 0; // not a timeline semaphore
		waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}
	void VulkanDevice::CommandQueue::submit(VulkanDevice* device, VkFence fence)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		std::scoped_lock lock(*locker);

		// Main submit with command lists and semaphores:
		{
			if (fence != VK_NULL_HANDLE)
			{
				// end of frame mark:
				for (int q = 0; q < QUEUE_COUNT; ++q)
				{
					if (frame_semaphores[device->GetBufferIndex()][q] == VK_NULL_HANDLE)
						continue;
					signal(frame_semaphores[device->GetBufferIndex()][q]);
				}
			}

			VkSubmitInfo2 submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
			submitInfo.commandBufferInfoCount = (uint32_t)submit_cmds.size();
			submitInfo.pCommandBufferInfos = submit_cmds.data();

			submitInfo.waitSemaphoreInfoCount = (uint32_t)submit_waitSemaphoreInfos.size();
			submitInfo.pWaitSemaphoreInfos = submit_waitSemaphoreInfos.data();

			submitInfo.signalSemaphoreInfoCount = (uint32_t)submit_signalSemaphoreInfos.size();
			submitInfo.pSignalSemaphoreInfos = submit_signalSemaphoreInfos.data();

			vulkan_check(vkQueueSubmit2(queue, 1, &submitInfo, fence));

			submit_waitSemaphoreInfos.clear();
			submit_signalSemaphoreInfos.clear();
			submit_cmds.clear();
		}

		// Swapchain presents:
		if (!swapchains.empty())
		{
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = (uint32_t)swapchainWaitSemaphores.size();
			presentInfo.pWaitSemaphores = swapchainWaitSemaphores.data();
			presentInfo.swapchainCount = (uint32_t)swapchains.size();
			presentInfo.pSwapchains = swapchains.data();
			presentInfo.pImageIndices = swapchainImageIndices.data();
			VkResult res = vkQueuePresentKHR(queue, &presentInfo);
			if (res != VK_SUCCESS)
			{
				// Handle outdated error in present:
				if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
				{
					for (auto& swapchain : swapchain_updates)
					{
						auto internal_state = to_internal(&swapchain);
						bool success = CreateSwapChainInternal(internal_state, device->physicalDevice, device->device, device->allocationhandler);
						assert(success);
					}
				}
				else
				{
					vulkan_assert(false, "vkQueuePresentKHR");
				}
			}

			swapchain_updates.clear();
			swapchains.clear();
			swapchainImageIndices.clear();
			swapchainWaitSemaphores.clear();
		}
	}

	void VulkanDevice::CopyAllocator::init(VulkanDevice* device)
	{
		this->device = device;
	}
	void VulkanDevice::CopyAllocator::destroy()
	{
		vkQueueWaitIdle(device->queue_init.queue);
		for (auto& x : freelist)
		{
			vkDestroyCommandPool(device->device, x.transferCommandPool, nullptr);
			vkDestroyFence(device->device, x.fence, nullptr);
			for (auto& sema : x.semaphores)
			{
				vkDestroySemaphore(device->device, sema, nullptr);
			}
		}
		for (auto& x : async_worklist)
		{
			vkDestroyCommandPool(device->device, x.transferCommandPool, nullptr);
			vkDestroyFence(device->device, x.fence, nullptr);
			for (auto& sema : x.semaphores)
			{
				vkDestroySemaphore(device->device, sema, nullptr);
			}
		}
		for (auto& x : async_semaphore_recycle)
		{
			for (auto& sema : x)
			{
				vkDestroySemaphore(device->device, sema, nullptr);
			}
		}
	}
	VulkanDevice::CopyAllocator::CopyCMD VulkanDevice::CopyAllocator::allocate(uint64_t staging_size)
	{
		CopyCMD cmd;

		locker.lock();
		// Try to search for a staging buffer that can fit the request:
		for (size_t i = 0; i < freelist.size(); ++i)
		{
			if (freelist[i].uploadbuffer.desc.size >= staging_size)
			{
				cmd = std::move(freelist[i]);
				std::swap(freelist[i], freelist.back());
				freelist.pop_back();
				break;
			}
		}
		locker.unlock();

		// If no buffer was found that fits the data, create one:
		if (!cmd.IsValid())
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			poolInfo.queueFamilyIndex = device->initFamily;
			vulkan_check(vkCreateCommandPool(device->device, &poolInfo, nullptr, &cmd.transferCommandPool));

			VkCommandBufferAllocateInfo commandBufferInfo = {};
			commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferInfo.commandBufferCount = 1;
			commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferInfo.commandPool = cmd.transferCommandPool;
			vulkan_check(vkAllocateCommandBuffers(device->device, &commandBufferInfo, &cmd.transferCommandBuffer));

			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			vulkan_check(vkCreateFence(device->device, &fenceInfo, nullptr, &cmd.fence));
			device->set_fence_name(cmd.fence, "CopyAllocator::fence");

			if (staging_size > 0)
			{
				GPUBufferDesc uploaddesc;
				uploaddesc.size = Protean::Math::GetNextPowerOfTwo(staging_size);
				uploaddesc.size = std::max(uploaddesc.size, uint64_t(65536));
				uploaddesc.usage = Usage_UPLOAD;
				bool upload_success = device->CreateBuffer(&uploaddesc, nullptr, &cmd.uploadbuffer);
				assert(upload_success);
				device->SetName(&cmd.uploadbuffer, "CopyAllocator::uploadBuffer");
			}
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		vulkan_check(vkResetCommandPool(device->device, cmd.transferCommandPool, 0));
		vulkan_check(vkBeginCommandBuffer(cmd.transferCommandBuffer, &beginInfo));

		vulkan_check(vkResetFences(device->device, 1, &cmd.fence));

		return cmd;
	}
	void VulkanDevice::CopyAllocator::submit(CopyCMD cmd, bool wait_cpu)
	{
		VkSubmitInfo2 submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

		VkCommandBufferSubmitInfo cbSubmitInfo = {};
		cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

		StackVector<VkSemaphoreSubmitInfo, QUEUE_COUNT> signals;

		{
			vulkan_check(vkEndCommandBuffer(cmd.transferCommandBuffer));
			cbSubmitInfo.commandBuffer = cmd.transferCommandBuffer;
			submitInfo.commandBufferInfoCount = 1;
			submitInfo.pCommandBufferInfos = &cbSubmitInfo;

			if (!wait_cpu)
			{
				for (int q = 0; q < QUEUE_COUNT; ++q)
				{
					CommandQueue& queue = device->queues[q];
					if (queue.queue == nullptr)
						continue;

					auto& signal = signals.emplace_back();
					signal.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
					signal.semaphore = device->new_semaphore();
					signal.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				}
				submitInfo.pSignalSemaphoreInfos = signals.data();
				submitInfo.signalSemaphoreInfoCount = signals.size();
			}

			std::scoped_lock lock(*device->queue_init.locker);
			vulkan_check(vkQueueSubmit2(device->queue_init.queue, 1, &submitInfo, cmd.fence));
		}

		if (wait_cpu)
		{
			while (vulkan_check(vkWaitForFences(device->device, 1, &cmd.fence, VK_TRUE, timeout_value)) == VK_TIMEOUT)
			{
				wilog_error("[CopyAllocator::submit] vkWaitForFences resulted in VK_TIMEOUT");
				std::this_thread::yield();
			}

			std::scoped_lock lock(locker);
			freelist.push_back(cmd);
		}
		else
		{
			for (int q = 0; q < QUEUE_COUNT; ++q)
			{
				CommandQueue& queue = device->queues[q];
				if (queue.queue == nullptr)
					continue;
				std::scoped_lock lock(*queue.locker);
				queue.submit_waitSemaphoreInfos.push_back(signals.back());
				signals.pop_back();

				cmd.semaphores.push_back(queue.submit_waitSemaphoreInfos.back().semaphore);
			}
			
			std::scoped_lock lock(locker);
			async_worklist.push_back(cmd);
		}
	}

	void VulkanDevice::DescriptorBinder::flush(bool graphics, CommandList cmd)
	{
		if (dirty == DIRTY_NONE)
			return;

		CommandList_Vulkan& commandlist = device->GetCommandList(cmd);
		assert(commandlist.layout.pipeline_layout != VK_NULL_HANDLE); // No pipeline was set!

		uint32_t uniform_buffer_dynamic_offsets[DESCRIPTORBINDER_CBV_COUNT] = {};
		for (size_t i = 0; i < std::min((uint32_t)arraysize(uniform_buffer_dynamic_offsets), device->dynamic_cbv_count); ++i)
		{
			uniform_buffer_dynamic_offsets[i] = table.CBV[i].IsValid() ? (uint32_t)table.CBV_offset[i] : 0;
		}

		if (descriptorSet == VK_NULL_HANDLE || (dirty & DIRTY_DESCRIPTOR))
		{
			const PSOLayout& layout = commandlist.layout;
			auto& binder_pool = commandlist.binder_pools[device->GetBufferIndex()];
			descriptorSet = binder_pool.allocate(layout);

			struct DescriptorTableInfo
			{
				VkDescriptorBufferInfo CBV[DESCRIPTORBINDER_CBV_COUNT];
				VkDescriptorImageInfo SRV[DESCRIPTORBINDER_SRV_COUNT];
				VkDescriptorImageInfo UAV[DESCRIPTORBINDER_UAV_COUNT];
				VkDescriptorImageInfo SAM[DESCRIPTORBINDER_SAMPLER_COUNT];
				VkWriteDescriptorSetAccelerationStructureKHR AccelerationStructure[DESCRIPTORBINDER_SRV_COUNT];
			} infos = {};

			struct DescriptorTableWrites
			{
				VkWriteDescriptorSet CBV[DESCRIPTORBINDER_CBV_COUNT];
				VkWriteDescriptorSet SRV[DESCRIPTORBINDER_SRV_COUNT];
				VkWriteDescriptorSet UAV[DESCRIPTORBINDER_UAV_COUNT];
				VkWriteDescriptorSet SAM[DESCRIPTORBINDER_SAMPLER_COUNT];
			} writes = {};

			VkDescriptorBufferInfo nullBufferInfo = {};
			nullBufferInfo.buffer = device->nullBuffer;
			nullBufferInfo.range = VK_WHOLE_SIZE;

			for (uint32_t i = 0; i < arraysize(table.CBV); ++i)
			{
				const GPUBuffer& buffer = table.CBV[i];
				auto& info = infos.CBV[i];
				auto& write = writes.CBV[i];
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = layout.table.CBV[i].descriptorType;
				write.dstBinding = VULKAN_BINDING_SHIFT_B + i;
				write.descriptorCount = 1;
				write.dstSet = descriptorSet;
				write.pBufferInfo = &info;

				if (buffer.IsValid())
				{
					auto internal_state = to_internal(&buffer);
					info.buffer = internal_state->resource;
					info.offset = i < device->dynamic_cbv_count ? 0 : table.CBV_offset[i];
					info.range = std::min(dynamic_cbv_maxsize, buffer.desc.size - table.CBV_offset[i]);
				}
				else
				{
					write.pBufferInfo = &nullBufferInfo;
				}
			}

			for (uint32_t i = 0; i < arraysize(table.SRV); ++i)
			{
				const GPUResource& res = table.SRV[i];
				auto& write = writes.SRV[i];
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = layout.table.SRV[i].descriptorType;
				write.dstBinding = VULKAN_BINDING_SHIFT_T + i;
				write.descriptorCount = 1;
				write.dstSet = descriptorSet;

				if (res.IsBuffer() && res.IsValid() && (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER || write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER))
				{
					auto internal_state = to_internal<GPUBuffer>(&res);
					int subresource = table.SRV_index[i];
					auto& descriptor = subresource >= 0 ? internal_state->subresources_srv[subresource] : internal_state->srv;
					if (write.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
					{
						if (descriptor.buffer_view == VK_NULL_HANDLE)
						{
							write.pTexelBufferView = &device->nullBufferView;
						}
						else
						{
							write.pTexelBufferView = &descriptor.buffer_view;
						}
					}
					else
					{
						if (descriptor.buffer_info.buffer == VK_NULL_HANDLE)
						{
							write.pBufferInfo = &nullBufferInfo;
						}
						else
						{
							write.pBufferInfo = &descriptor.buffer_info;
						}
					}
				}
				else if (res.IsTexture() && res.IsValid() && write.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
				{
					auto internal_state = to_internal<Texture>(&res);
					int subresource = table.SRV_index[i];
					auto& descriptor = subresource >= 0 ? internal_state->subresources_srv[subresource] : internal_state->srv;
					auto& info = infos.SRV[i];
					info.imageView = descriptor.image_view;
					info.imageLayout = internal_state->defaultLayout;
					write.pImageInfo = &info;
				}
				else if (res.IsAccelerationStructure() && res.IsValid() && write.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
				{
					auto internal_state = to_internal<RaytracingAccelerationStructure>(&res);
					auto& info = infos.AccelerationStructure[i];
					info.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
					info.accelerationStructureCount = 1;
					info.pAccelerationStructures = &internal_state->resource;
					write.pNext = &info;
				}
				else
				{
					switch (write.descriptorType)
					{
					default:
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
						write.pBufferInfo = &nullBufferInfo;
						break;
					case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
						write.pTexelBufferView = &device->nullBufferView;
						break;
					case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					{
						auto& info = infos.SRV[i];
						info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						switch (layout.SRV_image_types[i])
						{
						default:
						case VK_IMAGE_VIEW_TYPE_1D:
							info.imageView = device->nullImageView1D;
							break;
						case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
							info.imageView = device->nullImageView1DArray;
							break;
						case VK_IMAGE_VIEW_TYPE_2D:
							info.imageView = device->nullImageView2D;
							break;
						case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
							info.imageView = device->nullImageView2DArray;
							break;
						case VK_IMAGE_VIEW_TYPE_3D:
							info.imageView = device->nullImageView3D;
							break;
						case VK_IMAGE_VIEW_TYPE_CUBE:
							info.imageView = device->nullImageViewCube;
							break;
						case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
							info.imageView = device->nullImageViewCubeArray;
							break;
						}
						write.pImageInfo = &info;
					}
					break;
					}
				}
			}

			for (uint32_t i = 0; i < arraysize(table.UAV); ++i)
			{
				const GPUResource& res = table.UAV[i];
				auto& write = writes.UAV[i];
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = layout.table.UAV[i].descriptorType;
				write.dstBinding = VULKAN_BINDING_SHIFT_U + i;
				write.descriptorCount = 1;
				write.dstSet = descriptorSet;

				if (res.IsBuffer() && res.IsValid() && (write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER || write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER))
				{
					auto internal_state = to_internal<GPUBuffer>(&res);
					int subresource = table.UAV_index[i];
					auto& descriptor = subresource >= 0 ? internal_state->subresources_uav[subresource] : internal_state->uav;
					if (write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
					{
						if (descriptor.buffer_view == VK_NULL_HANDLE)
						{
							write.pTexelBufferView = &device->nullBufferView;
						}
						else
						{
							write.pTexelBufferView = &descriptor.buffer_view;
						}
					}
					else
					{
						if (descriptor.buffer_info.buffer == VK_NULL_HANDLE)
						{
							write.pBufferInfo = &nullBufferInfo;
						}
						else
						{
							write.pBufferInfo = &descriptor.buffer_info;
						}
					}
				}
				else if (res.IsTexture() && res.IsValid() && write.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
				{
					auto internal_state = to_internal<Texture>(&res);
					int subresource = table.UAV_index[i];
					auto& descriptor = subresource >= 0 ? internal_state->subresources_uav[subresource] : internal_state->uav;
					auto& info = infos.UAV[i];
					info.imageView = descriptor.image_view;
					info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
					write.pImageInfo = &info;
				}
				else
				{
					switch (write.descriptorType)
					{
					default:
					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
						write.pBufferInfo = &nullBufferInfo;
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
						write.pTexelBufferView = &device->nullBufferView;
						break;
					case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					{
						auto& info = infos.UAV[i];
						info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						switch (layout.UAV_image_types[i])
						{
						default:
						case VK_IMAGE_VIEW_TYPE_1D:
							info.imageView = device->nullImageView1D;
							break;
						case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
							info.imageView = device->nullImageView1DArray;
							break;
						case VK_IMAGE_VIEW_TYPE_2D:
							info.imageView = device->nullImageView2D;
							break;
						case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
							info.imageView = device->nullImageView2DArray;
							break;
						case VK_IMAGE_VIEW_TYPE_3D:
							info.imageView = device->nullImageView3D;
							break;
						case VK_IMAGE_VIEW_TYPE_CUBE:
							info.imageView = device->nullImageViewCube;
							break;
						case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
							info.imageView = device->nullImageViewCubeArray;
							break;
						}
						write.pImageInfo = &info;
					}
					break;
					}
				}
			}

			for (uint32_t i = 0; i < arraysize(table.SAM); ++i)
			{
				const Sampler& sam = table.SAM[i];
				auto& write = writes.SAM[i];
				write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				write.descriptorType = layout.table.SAM[i].descriptorType;
				write.dstBinding = VULKAN_BINDING_SHIFT_S + i;
				write.descriptorCount = 1;
				write.dstSet = descriptorSet;
				auto& info = infos.SAM[i];
				write.pImageInfo = &info;

				if (sam.IsValid())
				{
					auto internal_state = to_internal(&sam);
					info.sampler = internal_state->resource;
				}
				else
				{
					info.sampler = device->nullSampler;
				}
			}

			vkUpdateDescriptorSets(
				device->device,
				sizeof(writes) / sizeof(VkWriteDescriptorSet),
				(const VkWriteDescriptorSet*)&writes,
				0,
				nullptr
			);
		}

		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		if (!graphics)
		{
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

			if (commandlist.active_cs->stage == ShaderStage_LIB)
			{
				bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
			}
		}

		DescriptorSets descriptor_sets = device->descriptor_sets;
		descriptor_sets.sets[DESCRIPTOR_SET_BINDINGS] = descriptorSet;

		vkCmdBindDescriptorSets(
			commandlist.GetCommandBuffer(),
			bindPoint,
			commandlist.layout.pipeline_layout,
			0,
			arraysize(descriptor_sets.sets),
			descriptor_sets.sets,
			device->dynamic_cbv_count,
			uniform_buffer_dynamic_offsets
		);

		dirty = DIRTY_NONE;
	}

	// Engine functions
	VulkanDevice::VulkanDevice(Protean::Platform::window_type window, ValidationMode validationMode_, GPUPreference preference)
	{
		Protean::Timer timer;
		capabilities |= GraphicsDeviceCapability_ALIASING_GENERIC;

		// This functionalty is missing from Vulkan but might be added in the future:
		//	Issue: https://github.com/KhronosGroup/Vulkan-Docs/issues/2079
		capabilities |= GraphicsDeviceCapability_COPY_BETWEEN_DIFFERENT_IMAGE_ASPECTS_NOT_SUPPORTED;

		Protean::unordered_map<uint32_t, Protean::Allocator::shared_ptr<std::mutex>> queue_lockers;

		TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(VkAccelerationStructureInstanceKHR);

		validationMode = validationMode_;

		VkResult res;

		res = vulkan_check(volkInitialize());
		if (res != VK_SUCCESS)
		{
			Protean::Helper::messageBox("volkInitialize failed! ERROR: " + std::string(string_VkResult(res)), "Error!");
			Protean::Platform::Exit();
		}

		// Fill out application info:
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Protean Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Protean";
		appInfo.engineVersion = VK_MAKE_VERSION(Protean::Version::GetMajor(), Protean::Version::GetMinor(), Protean::Version::GetRevision());
		appInfo.apiVersion = VK_API_VERSION_1_3;

		// Enumerate available layers and extensions:
		uint32_t instanceLayerCount;
		vulkan_check(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
		Protean::vector<VkLayerProperties> availableInstanceLayers(instanceLayerCount);
		vulkan_check(vkEnumerateInstanceLayerProperties(&instanceLayerCount, availableInstanceLayers.data()));

		uint32_t extensionCount = 0;
		vulkan_check(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
		Protean::vector<VkExtensionProperties> availableInstanceExtensions(extensionCount);
		vulkan_check(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableInstanceExtensions.data()));

		Protean::vector<const char*> instanceLayers;
		Protean::vector<const char*> instanceExtensions;

		for (auto& availableExtension : availableInstanceExtensions)
		{
			if (strcmp(availableExtension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
			{
				debugUtils = true;
				instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			}
			else if (strcmp(availableExtension.extensionName, VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME) == 0)
			{
				instanceExtensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
			}
			else if (strcmp(availableExtension.extensionName, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) == 0)
			{
				instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
			}
		}

		instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#if defined(VK_USE_PLATFORM_WIN32_KHR)
		instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(SDL2)
		if (window != nullptr) {
			uint32_t extensionCount;
			SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
			Protean::vector<const char *> extensionNames_sdl(extensionCount);
			SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames_sdl.data());
			instanceExtensions.reserve(instanceExtensions.size() + extensionNames_sdl.size());
			for (auto& x : extensionNames_sdl)
			{
				instanceExtensions.push_back(x);
			}
		}
#elif defined(SDL3)
		{
			Uint32 extensionCount = 0;
			char const* const* extensionNames_sdl = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
			if (extensionNames_sdl != nullptr)
			{
				instanceExtensions.reserve(instanceExtensions.size() + extensionCount);
				for (Uint32 i = 0; i < extensionCount; ++i)
				{
					instanceExtensions.push_back(extensionNames_sdl[i]);
				}
			}
		}
#endif // _WIN32

		if (validationMode != ValidationMode_Disabled)
		{
			// Determine the optimal validation layers to enable that are necessary for useful debugging
			static const Protean::vector<const char*> validationLayerPriorityList[] =
			{
				// The preferred validation layer is "VK_LAYER_KHRONOS_validation"
				{"VK_LAYER_KHRONOS_validation"},

				// Otherwise we fallback to using the LunarG meta layer
				{"VK_LAYER_LUNARG_standard_validation"},

				// Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it doesn't exist
				{
					"VK_LAYER_GOOGLE_threading",
					"VK_LAYER_LUNARG_parameter_validation",
					"VK_LAYER_LUNARG_object_tracker",
					"VK_LAYER_LUNARG_core_validation",
					"VK_LAYER_GOOGLE_unique_objects",
				},

				// Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
				{"VK_LAYER_LUNARG_core_validation"}
			};

			for (auto& validationLayers : validationLayerPriorityList)
			{
				if (ValidateLayers(validationLayers, availableInstanceLayers))
				{
					for (auto& x : validationLayers)
					{
						instanceLayers.push_back(x);
					}
					break;
				}
			}
		}

		// Create instance:
		{
			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
			createInfo.ppEnabledLayerNames = instanceLayers.data();
			createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
			createInfo.ppEnabledExtensionNames = instanceExtensions.data();

			VkDebugUtilsMessengerCreateInfoEXT debugUtilsCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };

			if (validationMode != ValidationMode_Disabled && debugUtils)
			{
				debugUtilsCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
				debugUtilsCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

				if (validationMode == ValidationMode_Verbose)
				{
					debugUtilsCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
					debugUtilsCreateInfo.messageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
				}

				debugUtilsCreateInfo.pfnUserCallback = debugUtilsMessengerCallback;
				createInfo.pNext = &debugUtilsCreateInfo;
			}

			res = vulkan_check(vkCreateInstance(&createInfo, nullptr, &instance));
			if (res != VK_SUCCESS)
			{
				Protean::Helper::messageBox("vkCreateInstance failed! ERROR: " + std::string(string_VkResult(res)), "Error!");
				Protean::Platform::Exit();
			}

			volkLoadInstanceOnly(instance);

			if (validationMode != ValidationMode_Disabled && debugUtils)
			{
				vulkan_check(vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsCreateInfo, nullptr, &debugUtilsMessenger));
			}
		}

		// Enumerating and creating devices:
		{
			uint32_t deviceCount = 0;
			vulkan_check(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
			if (deviceCount == 0)
			{
				wilog_messagebox("Failed to find GPU with Vulkan 1.3 support!");
				Protean::Platform::Exit();
			}

			Protean::vector<VkPhysicalDevice> devices(deviceCount);
			vulkan_check(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

			const Protean::vector<const char*> required_deviceExtensions = {
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			};
			Protean::vector<const char*> enabled_deviceExtensions;

			bool h264_decode_extension = false;
			bool h265_decode_extension = false;
			bool suitable = false;
			bool conservativeRasterization = false;

			auto checkPhysicalDeviceAndFillPropertiesFeatures = [&](VkPhysicalDevice dev) {
				uint32_t extensionCount;
				vulkan_check(vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr));
				Protean::vector<VkExtensionProperties> available_deviceExtensions(extensionCount);
				vulkan_check(vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, available_deviceExtensions.data()));

				for (auto& x : required_deviceExtensions)
				{
					if (!checkExtensionSupport(x, available_deviceExtensions))
					{
						return false;
					}
				}

				h264_decode_extension = false;
				h265_decode_extension = false;

				features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
				features_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
				features_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
				features_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
				features2.pNext = &features_1_1;
				features_1_1.pNext = &features_1_2;
				features_1_2.pNext = &features_1_3;
				void** features_chain = &features_1_3.pNext;
				acceleration_structure_features = {};
				raytracing_features = {};
				raytracing_query_features = {};
				fragment_shading_rate_features = {};
				mesh_shader_features = {};
				conditional_rendering_features = {};
				depth_clip_enable_features = {};

				properties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				properties_1_1.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
				properties_1_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
				properties_1_3.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
				properties2.pNext = &properties_1_1;
				properties_1_1.pNext = &properties_1_2;
				properties_1_2.pNext = &properties_1_3;
				void** properties_chain = &properties_1_3.pNext;
				sampler_minmax_properties = {};
				acceleration_structure_properties = {};
				raytracing_properties = {};
				fragment_shading_rate_properties = {};
				mesh_shader_properties = {};
				conservative_raster_properties = {};
				conservativeRasterization = false;

				sampler_minmax_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_FILTER_MINMAX_PROPERTIES;
				*properties_chain = &sampler_minmax_properties;
				properties_chain = &sampler_minmax_properties.pNext;

				depth_stencil_resolve_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_STENCIL_RESOLVE_PROPERTIES;
				*properties_chain = &depth_stencil_resolve_properties;
				properties_chain = &depth_stencil_resolve_properties.pNext;

				enabled_deviceExtensions = required_deviceExtensions;

				if (checkExtensionSupport(VK_EXT_IMAGE_VIEW_MIN_LOD_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_EXT_IMAGE_VIEW_MIN_LOD_EXTENSION_NAME);
					image_view_min_lod_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_MIN_LOD_FEATURES_EXT;
					*features_chain = &image_view_min_lod_features;
					features_chain = &image_view_min_lod_features.pNext;
				}
				if (checkExtensionSupport(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME);
					depth_clip_enable_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEPTH_CLIP_ENABLE_FEATURES_EXT;
					*features_chain = &depth_clip_enable_features;
					features_chain = &depth_clip_enable_features.pNext;
				}
				if (checkExtensionSupport(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME, available_deviceExtensions))
				{
					conservativeRasterization = true;
					enabled_deviceExtensions.push_back(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
					conservative_raster_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
					*properties_chain = &conservative_raster_properties;
					properties_chain = &conservative_raster_properties.pNext;
				}
				if (checkExtensionSupport(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
					assert(checkExtensionSupport(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, available_deviceExtensions));
					enabled_deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
					acceleration_structure_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
					*features_chain = &acceleration_structure_features;
					features_chain = &acceleration_structure_features.pNext;
					acceleration_structure_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
					*properties_chain = &acceleration_structure_properties;
					properties_chain = &acceleration_structure_properties.pNext;

					if (checkExtensionSupport(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, available_deviceExtensions))
					{
						enabled_deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
						enabled_deviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
						raytracing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
						*features_chain = &raytracing_features;
						features_chain = &raytracing_features.pNext;
						raytracing_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
						*properties_chain = &raytracing_properties;
						properties_chain = &raytracing_properties.pNext;
					}

					if (checkExtensionSupport(VK_KHR_RAY_QUERY_EXTENSION_NAME, available_deviceExtensions))
					{
						enabled_deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
						raytracing_query_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
						*features_chain = &raytracing_query_features;
						features_chain = &raytracing_query_features.pNext;
					}
				}

				if (checkExtensionSupport(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
					fragment_shading_rate_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
					*features_chain = &fragment_shading_rate_features;
					features_chain = &fragment_shading_rate_features.pNext;
					fragment_shading_rate_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
					*properties_chain = &fragment_shading_rate_properties;
					properties_chain = &fragment_shading_rate_properties.pNext;
				}

				if (checkExtensionSupport(VK_EXT_MESH_SHADER_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
					mesh_shader_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
					*features_chain = &mesh_shader_features;
					features_chain = &mesh_shader_features.pNext;
					mesh_shader_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT;
					*properties_chain = &mesh_shader_properties;
					properties_chain = &mesh_shader_properties.pNext;
				}

				if (checkExtensionSupport(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME);
					conditional_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONDITIONAL_RENDERING_FEATURES_EXT;
					*features_chain = &conditional_rendering_features;
					features_chain = &conditional_rendering_features.pNext;
				}

				if (checkExtensionSupport(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME, available_deviceExtensions) &&
					checkExtensionSupport(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
					enabled_deviceExtensions.push_back(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME);
					if (checkExtensionSupport(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME, available_deviceExtensions))
					{
						enabled_deviceExtensions.push_back(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME);
						h264_decode_extension = true;
					}
					if (checkExtensionSupport(VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME, available_deviceExtensions))
					{
						enabled_deviceExtensions.push_back(VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME);
						h265_decode_extension = true;
					}
				}

#if defined(VK_USE_PLATFORM_WIN32_KHR)
				if (checkExtensionSupport(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, available_deviceExtensions) &&
					checkExtensionSupport(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
					enabled_deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
				}
#elif defined(__linux__)
				if (checkExtensionSupport(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, available_deviceExtensions) &&
					checkExtensionSupport(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, available_deviceExtensions))
				{
					enabled_deviceExtensions.push_back(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
					enabled_deviceExtensions.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
				}
#endif

				*properties_chain = nullptr;
				*features_chain = nullptr;
				vkGetPhysicalDeviceProperties2(dev, &properties2);
				vkGetPhysicalDeviceFeatures2(dev, &features2);

				return true;
			};

			for (const auto& dev : devices)
			{
				if (!checkPhysicalDeviceAndFillPropertiesFeatures(dev))
					continue;

				bool priority = properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
				if (preference == GPUPreference_Integrated)
				{
					priority = properties2.properties.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
				}
				else if (preference == GPUPreference_AMD)
				{
					priority = Protean::Helper::toUpper(std::string(properties2.properties.deviceName)).find("AMD") != std::string::npos;
				}
				else if (preference == GPUPreference_Nvidia)
				{
					priority = Protean::Helper::toUpper(std::string(properties2.properties.deviceName)).find("NVIDIA") != std::string::npos;
				}
				else if (preference == GPUPreference_Intel)
				{
					priority = Protean::Helper::toUpper(std::string(properties2.properties.deviceName)).find("INTEL") != std::string::npos;
				}
				if (priority || physicalDevice == VK_NULL_HANDLE)
				{
					physicalDevice = dev;
					if (priority)
						break; // if this is prioritized GPU type, look no further
				}
			}

			if (physicalDevice == VK_NULL_HANDLE)
			{
				wilog_messagebox("Failed to find a suitable GPU!");
				Protean::Platform::Exit();
			}

			// This fills the properties and features again of the finally selected graphics card:
			checkPhysicalDeviceAndFillPropertiesFeatures(physicalDevice);

			assert(properties2.properties.limits.timestampComputeAndGraphics == VK_TRUE);

			assert(features2.features.imageCubeArray == VK_TRUE);
			assert(features2.features.independentBlend == VK_TRUE);
			assert(features2.features.geometryShader == VK_TRUE);
			assert(features2.features.samplerAnisotropy == VK_TRUE);
			assert(features2.features.shaderClipDistance == VK_TRUE);
			assert(features2.features.textureCompressionBC == VK_TRUE);
			assert(features2.features.occlusionQueryPrecise == VK_TRUE);
			assert(features_1_3.dynamicRendering == VK_TRUE);

			// Init adapter properties
			vendorId = properties2.properties.vendorID;
			deviceId = properties2.properties.deviceID;
			adapterName = properties2.properties.deviceName;

			driverDescription = properties_1_2.driverName;
			if (properties_1_2.driverInfo[0] != '\0')
			{
				driverDescription += std::string(": ") + properties_1_2.driverInfo;
			}

			switch (properties2.properties.deviceType)
			{
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				adapterType = AdapterType_IntegratedGpu;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				adapterType = AdapterType_DiscreteGpu;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				adapterType = AdapterType_VirtualGpu;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				adapterType = AdapterType_Cpu;
				break;
			default:
				adapterType = AdapterType_Other;
				break;
			}

			if (features2.features.tessellationShader == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability_TESSELLATION;
			}
			if (conservativeRasterization)
			{
				capabilities |= GraphicsDeviceCapability_CONSERVATIVE_RASTERIZATION;
			}
			if (features2.features.shaderStorageImageExtendedFormats == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability_UAV_LOAD_FORMAT_COMMON;
			}

			if (
				raytracing_features.rayTracingPipeline == VK_TRUE &&
				raytracing_query_features.rayQuery == VK_TRUE &&
				acceleration_structure_features.accelerationStructure == VK_TRUE &&
				features_1_2.bufferDeviceAddress == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability_RAYTRACING;
				SHADER_IDENTIFIER_SIZE = raytracing_properties.shaderGroupHandleSize;
			}
			if (mesh_shader_features.meshShader == VK_TRUE && mesh_shader_features.taskShader == VK_TRUE)
			{
				// Mesh shader still has issues on Vulkan: https://github.com/microsoft/DirectXShaderCompiler/issues/6862
				//capabilities |= GraphicsDeviceCapability_MESH_SHADER;
			}
			if (fragment_shading_rate_features.pipelineFragmentShadingRate == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability_VARIABLE_RATE_SHADING;
			}
			if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability_VARIABLE_RATE_SHADING_TIER2;
				VARIABLE_RATE_SHADING_TILE_SIZE = std::min(fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.width, fragment_shading_rate_properties.maxFragmentShadingRateAttachmentTexelSize.height);
			}

			VkFormatProperties formatProperties = {};
			vkGetPhysicalDeviceFormatProperties(physicalDevice, _ConvertFormat(Format_R11G11B10_FLOAT), &formatProperties);
			if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)
			{
				capabilities |= GraphicsDeviceCapability_UAV_LOAD_FORMAT_R11G11B10_FLOAT;
			}

			if (conditional_rendering_features.conditionalRendering == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability_PREDICATION;
			}

			if (features_1_2.samplerFilterMinmax == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability_SAMPLER_MINMAX;
			}

			if (features2.features.depthBounds == VK_TRUE)
			{
				capabilities |= GraphicsDeviceCapability_DEPTH_BOUNDS_TEST;
			}

			if (features2.features.sparseBinding == VK_TRUE && features2.features.sparseResidencyAliased == VK_TRUE)
			{
				if (properties2.properties.sparseProperties.residencyNonResidentStrict == VK_TRUE)
				{
					capabilities |= GraphicsDeviceCapability_SPARSE_NULL_MAPPING;
				}
				if (features2.features.sparseResidencyBuffer == VK_TRUE)
				{
					capabilities |= GraphicsDeviceCapability_SPARSE_BUFFER;
				}
				if (features2.features.sparseResidencyImage2D == VK_TRUE)
				{
					capabilities |= GraphicsDeviceCapability_SPARSE_TEXTURE2D;
				}
				if (features2.features.sparseResidencyImage3D == VK_TRUE)
				{
					capabilities |= GraphicsDeviceCapability_SPARSE_TEXTURE3D;
				}
			}

			if (
				(depth_stencil_resolve_properties.supportedDepthResolveModes & VK_RESOLVE_MODE_MIN_BIT) &&
				(depth_stencil_resolve_properties.supportedDepthResolveModes & VK_RESOLVE_MODE_MAX_BIT)
				)
			{
				capabilities |= GraphicsDeviceCapability_DEPTH_RESOLVE_MIN_MAX;
			}
			if (
				(depth_stencil_resolve_properties.supportedStencilResolveModes & VK_RESOLVE_MODE_MIN_BIT) &&
				(depth_stencil_resolve_properties.supportedStencilResolveModes & VK_RESOLVE_MODE_MAX_BIT)
				)
			{
				capabilities |= GraphicsDeviceCapability_STENCIL_RESOLVE_MIN_MAX;
			}

			if(h264_decode_extension)
			{
				decode_h264_profile.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PROFILE_INFO_KHR;
				decode_h264_profile.stdProfileIdc = STD_VIDEO_H264_PROFILE_IDC_HIGH;
				decode_h264_profile.pictureLayout = VK_VIDEO_DECODE_H264_PICTURE_LAYOUT_INTERLACED_INTERLEAVED_LINES_BIT_KHR;

				decode_h264_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_CAPABILITIES_KHR;

				video_capability_h264.profile.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
				video_capability_h264.profile.pNext = &decode_h264_profile;
				video_capability_h264.profile.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR;
				video_capability_h264.profile.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
				video_capability_h264.profile.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
				video_capability_h264.profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;

				video_capability_h264.decode_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR;

				video_capability_h264.video_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
				video_capability_h264.video_capabilities.pNext = &video_capability_h264.decode_capabilities;
				video_capability_h264.decode_capabilities.pNext = &decode_h264_capabilities;
				vulkan_check(vkGetPhysicalDeviceVideoCapabilitiesKHR(physicalDevice, &video_capability_h264.profile, &video_capability_h264.video_capabilities));

				if (video_capability_h264.decode_capabilities.flags)
				{
					capabilities |= GraphicsDeviceCapability_VIDEO_DECODE_H264;
					VIDEO_DECODE_BITSTREAM_ALIGNMENT = align(VIDEO_DECODE_BITSTREAM_ALIGNMENT, video_capability_h264.video_capabilities.minBitstreamBufferOffsetAlignment);
					VIDEO_DECODE_BITSTREAM_ALIGNMENT = align(VIDEO_DECODE_BITSTREAM_ALIGNMENT, video_capability_h264.video_capabilities.minBitstreamBufferSizeAlignment);
				}
			}

			if (h265_decode_extension)
			{
				decode_h265_profile.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_PROFILE_INFO_KHR;
				decode_h265_profile.stdProfileIdc = STD_VIDEO_H265_PROFILE_IDC_MAIN;

				decode_h265_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H265_CAPABILITIES_KHR;

				video_capability_h265.profile.sType = VK_STRUCTURE_TYPE_VIDEO_PROFILE_INFO_KHR;
				video_capability_h265.profile.pNext = &decode_h265_profile;
				video_capability_h265.profile.videoCodecOperation = VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR;
				video_capability_h265.profile.lumaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
				video_capability_h265.profile.chromaBitDepth = VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR;
				video_capability_h265.profile.chromaSubsampling = VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR;

				video_capability_h265.decode_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_CAPABILITIES_KHR;

				video_capability_h265.video_capabilities.sType = VK_STRUCTURE_TYPE_VIDEO_CAPABILITIES_KHR;
				video_capability_h265.video_capabilities.pNext = &video_capability_h265.decode_capabilities;
				video_capability_h265.decode_capabilities.pNext = &decode_h265_capabilities;
				vulkan_check(vkGetPhysicalDeviceVideoCapabilitiesKHR(physicalDevice, &video_capability_h265.profile, &video_capability_h265.video_capabilities));

				if (video_capability_h265.decode_capabilities.flags)
				{
					//capabilities |= GraphicsDeviceCapability_VIDEO_DECODE_H265; //TODO
					VIDEO_DECODE_BITSTREAM_ALIGNMENT = align(VIDEO_DECODE_BITSTREAM_ALIGNMENT, video_capability_h265.video_capabilities.minBitstreamBufferOffsetAlignment);
					VIDEO_DECODE_BITSTREAM_ALIGNMENT = align(VIDEO_DECODE_BITSTREAM_ALIGNMENT, video_capability_h265.video_capabilities.minBitstreamBufferSizeAlignment);
				}
			}

			// Find queue families:
			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, nullptr);

			queueFamilies.resize(queueFamilyCount);
			queueFamiliesVideo.resize(queueFamilyCount);
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				queueFamilies[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
				queueFamilies[i].pNext = &queueFamiliesVideo[i];
				queueFamiliesVideo[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR;
			}
			vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevice, &queueFamilyCount, queueFamilies.data());

			// Query base queue families:
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];
				auto& queueFamilyVideo = queueFamiliesVideo[i];

				if (graphicsFamily == VK_QUEUE_FAMILY_IGNORED && queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					graphicsFamily = i;
					if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
					{
						queues[QUEUE_GRAPHICS].sparse_binding_supported = true;
					}
				}

				if (copyFamily == VK_QUEUE_FAMILY_IGNORED && queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT)
				{
					copyFamily = i;
					initFamily = i;
				}

				if (computeFamily == VK_QUEUE_FAMILY_IGNORED && queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
				{
					computeFamily = i;
				}

				if (videoFamily == VK_QUEUE_FAMILY_IGNORED &&
					queueFamily.queueFamilyProperties.queueCount > 0 &&
					(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) &&
					(!h264_decode_extension || (queueFamilyVideo.videoCodecOperations & VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR)) &&
					(!h265_decode_extension || (queueFamilyVideo.videoCodecOperations & VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR))
					)
				{
					videoFamily = i;
				}
			}

			// Now try to find dedicated COPY queue:
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];

				if (queueFamily.queueFamilyProperties.queueCount > 0 &&
					queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT &&
					!(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					!(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
					)
				{
					copyFamily = i;
					initFamily = i;

					if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
					{
						queues[QUEUE_COPY].sparse_binding_supported = true;
					}
					break; // found it!
				}
			}

			// Now try to find dedicated COMPUTE queue:
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];

				if (queueFamily.queueFamilyProperties.queueCount > 0 &&
					queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT &&
					!(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					)
				{
					computeFamily = i;

					if (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
					{
						queues[QUEUE_COMPUTE].sparse_binding_supported = true;
					}
					break; // found it!
				}
			}

			// Now try to find dedicated transfer queue with only transfer and sparse flags:
			//	(This is a workaround for a driver bug with sparse updating from transfer queue)
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];

				if (queueFamily.queueFamilyProperties.queueCount > 0 && queueFamily.queueFamilyProperties.queueFlags == (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT))
				{
					copyFamily = i;
					queues[QUEUE_COPY].sparse_binding_supported = true;
				}
			}

			// Find sparse fallback:
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];

				if (queueFamily.queueFamilyProperties.queueCount > 0 && (queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT))
				{
					sparseFamily = i;
					break;
				}
			}

			// Try to find separate transfer queue for inits if available, otherwise it will use QUEUE_COPY
			for (uint32_t i = 0; i < queueFamilyCount; ++i)
			{
				auto& queueFamily = queueFamilies[i];

				if (queueFamily.queueFamilyProperties.queueCount > 0 &&
					queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT &&
					copyFamily != i &&
					!(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
					!(queueFamily.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
					// Video/optical flow families can also report TRANSFER (NVIDIA: one queue each), and the video family is also used for QUEUE_VIDEO_DECODE, so the init queue would share that VkQueue
					!(queueFamily.queueFamilyProperties.queueFlags & (VK_QUEUE_VIDEO_DECODE_BIT_KHR | VK_QUEUE_VIDEO_ENCODE_BIT_KHR | VK_QUEUE_OPTICAL_FLOW_BIT_NV))
					)
				{
					initFamily = i;
					break;
				}
			}

			Protean::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			Protean::unordered_set<uint32_t> uniqueQueueFamilies = { graphicsFamily,copyFamily,computeFamily,initFamily };
			if (videoFamily != VK_QUEUE_FAMILY_IGNORED)
			{
				uniqueQueueFamilies.insert(videoFamily);
			}
			if (sparseFamily != VK_QUEUE_FAMILY_IGNORED)
			{
				uniqueQueueFamilies.insert(sparseFamily);
			}

			float queuePriority = 1.0f;
			for (uint32_t queueFamily : uniqueQueueFamilies)
			{
				queue_lockers.emplace(queueFamily, Protean::Allocator::make_shared_single<std::mutex>());
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
				families.push_back(queueFamily);
			}

			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
			createInfo.pQueueCreateInfos = queueCreateInfos.data();
			createInfo.pEnabledFeatures = nullptr;
			createInfo.pNext = &features2;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(enabled_deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = enabled_deviceExtensions.data();

			res = vulkan_check(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));
			if (res != VK_SUCCESS)
			{
				Protean::Helper::messageBox("vkCreateDevice failed! ERROR: " + std::string(string_VkResult(res)), "Error!");
				Protean::Platform::Exit();
			}

			volkLoadDevice(device);
		}

		// queues:
		{
			vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
			vkGetDeviceQueue(device, computeFamily, 0, &computeQueue);
			vkGetDeviceQueue(device, copyFamily, 0, &copyQueue);
			vkGetDeviceQueue(device, initFamily, 0, &initQueue);
			if (videoFamily != VK_QUEUE_FAMILY_IGNORED)
			{
				vkGetDeviceQueue(device, videoFamily, 0, &videoQueue);
			}
			if (sparseFamily != VK_QUEUE_FAMILY_IGNORED)
			{
				vkGetDeviceQueue(device, sparseFamily, 0, &sparseQueue);
			}

			queues[QUEUE_GRAPHICS].queue = graphicsQueue;
			queues[QUEUE_GRAPHICS].locker = queue_lockers[graphicsFamily];
			queues[QUEUE_COMPUTE].queue = computeQueue;
			queues[QUEUE_COMPUTE].locker = queue_lockers[computeFamily];
			queues[QUEUE_COPY].queue = copyQueue;
			queues[QUEUE_COPY].locker = queue_lockers[copyFamily];
			queue_init.queue = initQueue;
			queue_init.locker = queue_lockers[initFamily];
			if (videoFamily != VK_QUEUE_FAMILY_IGNORED)
			{
				queues[QUEUE_VIDEO_DECODE].queue = videoQueue;
				queues[QUEUE_VIDEO_DECODE].locker = queue_lockers[videoFamily];
			}
			if (sparseFamily != VK_QUEUE_FAMILY_IGNORED)
			{
				queue_sparse.queue = sparseQueue;
				queue_sparse.locker = queue_lockers[sparseFamily];
				queue_sparse.sparse_binding_supported = true;
			}

		}

		memory_properties_2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &memory_properties_2);

		if (memory_properties_2.memoryProperties.memoryHeapCount == 1 &&
			memory_properties_2.memoryProperties.memoryHeaps[0].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
		{
			// https://registry.khronos.org/vulkan/specs/1.0-extensions/html/vkspec.html#memory-device
			//	"In a unified memory architecture (UMA) system there is often only a single memory heap which is
			//	considered to be equally “local” to the host and to the device, and such an implementation must advertise the heap as device-local."
			capabilities |= GraphicsDeviceCapability_CACHE_COHERENT_UMA;
		}

		allocationhandler = Protean::Allocator::make_shared_single<AllocationHandler>();
		allocationhandler->device = device;
		allocationhandler->instance = instance;

		// Initialize Vulkan Memory Allocator helper:
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;

		// Core in 1.1
		allocatorInfo.flags =
			VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT |
			VMA_ALLOCATOR_CREATE_KHR_BIND_MEMORY2_BIT;

		if (features_1_2.bufferDeviceAddress)
		{
			allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		}

#if VMA_DYNAMIC_VULKAN_FUNCTIONS
		static VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		allocatorInfo.pVulkanFunctions = &vulkanFunctions;
#endif
		res = vulkan_check(vmaCreateAllocator(&allocatorInfo, &allocationhandler->allocator));
		if (res != VK_SUCCESS)
		{
			Protean::Helper::messageBox("vmaCreateAllocator failed! ERROR: " + std::string(string_VkResult(res)), "Error!");
			Protean::Platform::Exit();
		}

		std::vector<VkExternalMemoryHandleTypeFlags> externalMemoryHandleTypes;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
		externalMemoryHandleTypes.resize(memory_properties_2.memoryProperties.memoryTypeCount, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT);
		allocatorInfo.pTypeExternalMemoryHandleTypes = externalMemoryHandleTypes.data();
#elif defined(__linux__)
		externalMemoryHandleTypes.resize(memory_properties_2.memoryProperties.memoryTypeCount, VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
		allocatorInfo.pTypeExternalMemoryHandleTypes = externalMemoryHandleTypes.data();
#endif

		res = vulkan_check(vmaCreateAllocator(&allocatorInfo, &allocationhandler->externalAllocator));
		if (res != VK_SUCCESS)
		{
			Protean::Helper::messageBox("Failed to create Vulkan external memory allocator, ERROR: " + std::string(string_VkResult(res)), "Error!");
			Protean::Platform::Exit();
		}

		copyAllocator.init(this);

		// Create frame resources:
		for (uint32_t fr = 0; fr < BUFFERCOUNT; ++fr)
		{
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				if (queues[queue].queue == VK_NULL_HANDLE)
					continue;

				VkFenceCreateInfo fenceInfo = {};
				fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
				//fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
				res = vulkan_check(vkCreateFence(device, &fenceInfo, nullptr, &frame_fence[fr][queue]));
				if (res != VK_SUCCESS)
				{
					Protean::Helper::messageBox("vkCreateFence[FRAME] failed! ERROR: " + std::string(string_VkResult(res)), "Error!");
					Protean::Platform::Exit();
				}
				switch (queue)
				{
				case QUEUE_GRAPHICS:
					set_fence_name(frame_fence[fr][queue], "frame_fence[QUEUE_GRAPHICS]");
					break;
				case QUEUE_COMPUTE:
					set_fence_name(frame_fence[fr][queue], "frame_fence[QUEUE_COMPUTE]");
					break;
				case QUEUE_COPY:
					set_fence_name(frame_fence[fr][queue], "frame_fence[QUEUE_COPY]");
					break;
				case QUEUE_VIDEO_DECODE:
					set_fence_name(frame_fence[fr][queue], "frame_fence[QUEUE_VIDEO_DECODE]");
					break;
				};
			}

			// Frame end semaphores:
			for (int queue1 = 0; queue1 < QUEUE_COUNT; ++queue1)
			{
				if (queues[queue1].queue == nullptr)
					continue;
				for (int queue2 = 0; queue2 < QUEUE_COUNT; ++queue2)
				{
					if (queue1 == queue2)
						continue;
					if (queues[queue2].queue == nullptr)
						continue;

					VkSemaphoreCreateInfo info = {};
					info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
					vulkan_check(vkCreateSemaphore(device, &info, nullptr, &queues[queue1].frame_semaphores[fr][queue2]));
					set_semaphore_name(queues[queue1].frame_semaphores[fr][queue2], "CommandQueue::frame_semaphores");
				}
			}
		}

		TIMESTAMP_FREQUENCY = uint64_t(1.0 / double(properties2.properties.limits.timestampPeriod) * 1000 * 1000 * 1000);

		dynamic_cbv_count = std::min(ROOT_CBV_COUNT, properties2.properties.limits.maxDescriptorSetUniformBuffersDynamic);

		// Dynamic PSO states:
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
		if (CheckCapability(GraphicsDeviceCapability_DEPTH_BOUNDS_TEST))
		{
			pso_dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
		}
		if (CheckCapability(GraphicsDeviceCapability_VARIABLE_RATE_SHADING))
		{
			pso_dynamicStates.push_back(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
		}
		pso_dynamicStates.push_back(VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE);

		dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo.pDynamicStates = pso_dynamicStates.data();
		dynamicStateInfo.dynamicStateCount = (uint32_t)pso_dynamicStates.size();

		dynamicStateInfo_MeshShader.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateInfo_MeshShader.pDynamicStates = pso_dynamicStates.data();
		dynamicStateInfo_MeshShader.dynamicStateCount = (uint32_t)pso_dynamicStates.size() - 1; // don't include VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE for mesh shader

		// Static samplers:
		{
			VkSamplerCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.compareEnable = false;
			createInfo.compareOp = VK_COMPARE_OP_NEVER;
			createInfo.minLod = 0;
			createInfo.maxLod = FLT_MAX;
			createInfo.mipLodBias = 0;
			createInfo.anisotropyEnable = false;
			createInfo.maxAnisotropy = 0;

			// sampler_linear_clamp:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[0]));

			// sampler_linear_wrap:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[1]));

			//sampler_linear_mirror:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[2]));

			// sampler_point_clamp:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[3]));

			// sampler_point_wrap:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[4]));

			// sampler_point_mirror:
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[5]));

			// sampler_aniso_clamp:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = 16;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[6]));

			// sampler_aniso_wrap:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = 16;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[7]));

			// sampler_aniso_mirror:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.anisotropyEnable = true;
			createInfo.maxAnisotropy = 16;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[8]));

			// sampler_cmp_depth:
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.anisotropyEnable = false;
			createInfo.maxAnisotropy = 0;
			createInfo.compareEnable = true;
			createInfo.compareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
			createInfo.minLod = 0;
			createInfo.maxLod = 0;
			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &immutable_samplers[9]));
		}

		// Create null descriptors for uninitialized bind slots and bindless safety fallback:
		{
			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = 4;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferInfo.flags = 0;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			vulkan_check(vmaCreateBuffer(allocationhandler->allocator, &bufferInfo, &allocInfo, &nullBuffer, &nullBufferAllocation, nullptr));

			VkBufferViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
			viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
			viewInfo.range = VK_WHOLE_SIZE;
			viewInfo.buffer = nullBuffer;
			vulkan_check(vkCreateBufferView(device, &viewInfo, nullptr, &nullBufferView));
		}
		{
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.extent.width = 1;
			imageInfo.extent.height = 1;
			imageInfo.extent.depth = 1;
			imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			imageInfo.arrayLayers = 1;
			imageInfo.mipLevels = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
			imageInfo.flags = 0;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

			imageInfo.imageType = VK_IMAGE_TYPE_1D;
			vulkan_check(vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage1D, &nullImageAllocation1D, nullptr));

			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			imageInfo.arrayLayers = 6;
			vulkan_check(vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage2D, &nullImageAllocation2D, nullptr));

			imageInfo.imageType = VK_IMAGE_TYPE_3D;
			imageInfo.flags = 0;
			imageInfo.arrayLayers = 1;
			vulkan_check(vmaCreateImage(allocationhandler->allocator, &imageInfo, &allocInfo, &nullImage3D, &nullImageAllocation3D, nullptr));

			// Transitions:
			{
				VkImageMemoryBarrier2 barrier = {};
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.oldLayout = imageInfo.initialLayout;
				barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
				barrier.srcAccessMask = 0;
				barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
				barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				barrier.subresourceRange.baseArrayLayer = 0;
				barrier.subresourceRange.baseMipLevel = 0;
				barrier.subresourceRange.levelCount = 1;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				barrier.image = nullImage1D;
				barrier.subresourceRange.layerCount = 1;
				init_transitions.push_back(barrier);

				barrier.image = nullImage2D;
				barrier.subresourceRange.layerCount = 6;
				init_transitions.push_back(barrier);

				barrier.image = nullImage3D;
				barrier.subresourceRange.layerCount = 1;
				init_transitions.push_back(barrier);
			}

			VkImageViewCreateInfo viewInfo = {};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;

			viewInfo.image = nullImage1D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D;
			vulkan_check(vkCreateImageView(device, &viewInfo, nullptr, &nullImageView1D));

			viewInfo.image = nullImage1D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
			vulkan_check(vkCreateImageView(device, &viewInfo, nullptr, &nullImageView1DArray));

			viewInfo.image = nullImage2D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			vulkan_check(vkCreateImageView(device, &viewInfo, nullptr, &nullImageView2D));

			viewInfo.image = nullImage2D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			vulkan_check(vkCreateImageView(device, &viewInfo, nullptr, &nullImageView2DArray));

			viewInfo.image = nullImage2D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			viewInfo.subresourceRange.layerCount = 6;
			vulkan_check(vkCreateImageView(device, &viewInfo, nullptr, &nullImageViewCube));

			viewInfo.image = nullImage2D;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			viewInfo.subresourceRange.layerCount = 6;
			vulkan_check(vkCreateImageView(device, &viewInfo, nullptr, &nullImageViewCubeArray));

			viewInfo.image = nullImage3D;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
			vulkan_check(vkCreateImageView(device, &viewInfo, nullptr, &nullImageView3D));
		}
		{
			VkSamplerCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

			vulkan_check(vkCreateSampler(device, &createInfo, nullptr, &nullSampler));
		}

		// Bindings:
		{
			for (uint32_t i = 0; i < arraysize(layout_template.table.CBV); ++i)
			{
				auto& binding = layout_template.table.CBV[i];
				binding.descriptorType = i < dynamic_cbv_count ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				binding.binding = VULKAN_BINDING_SHIFT_B + i;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_ALL;
			}
			for (uint32_t i = 0; i < arraysize(layout_template.table.SRV); ++i)
			{
				auto& binding = layout_template.table.SRV[i];
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				binding.binding = VULKAN_BINDING_SHIFT_T + i;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_ALL;
			}
			for (uint32_t i = 0; i < arraysize(layout_template.table.UAV); ++i)
			{
				auto& binding = layout_template.table.UAV[i];
				binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				binding.binding = VULKAN_BINDING_SHIFT_U + i;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_ALL;
			}
			for (uint32_t i = 0; i < arraysize(layout_template.table.SAM); ++i)
			{
				auto& binding = layout_template.table.SAM[i];
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				binding.binding = VULKAN_BINDING_SHIFT_S + i;
				binding.descriptorCount = 1;
				binding.stageFlags = VK_SHADER_STAGE_ALL;
			}
			for (uint32_t i = 0; i < arraysize(layout_template.table.IMMUTABLE_SAM); ++i)
			{
				auto& binding = layout_template.table.IMMUTABLE_SAM[i];
				binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				binding.binding = VULKAN_BINDING_SHIFT_S + STATIC_SAMPLER_SLOT_BEGIN + i;
				binding.descriptorCount = 1;
				binding.pImmutableSamplers = &immutable_samplers[i];
				binding.stageFlags = VK_SHADER_STAGE_ALL;
			}
		}

		// Bindless:
		{
			assert(features_1_2.descriptorIndexing == VK_TRUE);
			assert(features_1_2.runtimeDescriptorArray == VK_TRUE);
			assert(features_1_2.descriptorBindingVariableDescriptorCount == VK_TRUE);
			assert(features_1_2.descriptorBindingPartiallyBound == VK_TRUE);
			assert(features_1_2.descriptorBindingUpdateUnusedWhilePending == VK_TRUE);
			assert(features_1_2.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE);
			assert(features_1_2.descriptorBindingStorageImageUpdateAfterBind == VK_TRUE);
			assert(features_1_2.descriptorBindingStorageBufferUpdateAfterBind == VK_TRUE);
			assert(features_1_2.descriptorBindingUniformTexelBufferUpdateAfterBind == VK_TRUE);
			assert(features_1_2.descriptorBindingStorageTexelBufferUpdateAfterBind == VK_TRUE);
			assert(features_1_2.shaderSampledImageArrayNonUniformIndexing == VK_TRUE);
			assert(features_1_2.shaderStorageBufferArrayNonUniformIndexing == VK_TRUE);
			assert(features_1_2.shaderStorageImageArrayNonUniformIndexing == VK_TRUE);
			assert(features_1_2.shaderUniformTexelBufferArrayNonUniformIndexing == VK_TRUE);
			assert(features_1_2.shaderStorageTexelBufferArrayNonUniformIndexing == VK_TRUE);

			allocationhandler->bindlessSamplers.init(this, VK_DESCRIPTOR_TYPE_SAMPLER, std::min(BINDLESS_SAMPLER_CAPACITY, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages));
			descriptor_sets.sets[DESCRIPTOR_SET_BINDLESS_SAMPLER] = allocationhandler->bindlessSamplers.descriptorSet;

			allocationhandler->bindlessStorageBuffers.init(this, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, std::min(BINDLESS_RESOURCE_CAPACITY, properties_1_2.maxDescriptorSetUpdateAfterBindStorageBuffers));
			descriptor_sets.sets[DESCRIPTOR_SET_BINDLESS_STORAGE_BUFFER] = allocationhandler->bindlessStorageBuffers.descriptorSet;

			allocationhandler->bindlessUniformTexelBuffers.init(this, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, std::min(BINDLESS_RESOURCE_CAPACITY, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages / 2));
			descriptor_sets.sets[DESCRIPTOR_SET_BINDLESS_UNIFORM_TEXEL_BUFFER] = allocationhandler->bindlessUniformTexelBuffers.descriptorSet;

			allocationhandler->bindlessSampledImages.init(this, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, std::min(BINDLESS_RESOURCE_CAPACITY, properties_1_2.maxDescriptorSetUpdateAfterBindSampledImages / 2));
			descriptor_sets.sets[DESCRIPTOR_SET_BINDLESS_SAMPLED_IMAGE] = allocationhandler->bindlessSampledImages.descriptorSet;

			allocationhandler->bindlessStorageImages.init(this, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, std::min(BINDLESS_RESOURCE_CAPACITY, properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages / 2));
			descriptor_sets.sets[DESCRIPTOR_SET_BINDLESS_STORAGE_IMAGE] = allocationhandler->bindlessStorageImages.descriptorSet;

			allocationhandler->bindlessStorageTexelBuffers.init(this, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, std::min(BINDLESS_RESOURCE_CAPACITY, properties_1_2.maxDescriptorSetUpdateAfterBindStorageImages / 2));
			descriptor_sets.sets[DESCRIPTOR_SET_BINDLESS_STORAGE_TEXEL_BUFFER] = allocationhandler->bindlessStorageTexelBuffers.descriptorSet;

			if (CheckCapability(GraphicsDeviceCapability_RAYTRACING))
			{
				allocationhandler->bindlessAccelerationStructures.init(this, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 32);
				descriptor_sets.sets[DESCRIPTOR_SET_BINDLESS_ACCELERATION_STRUCTURE] = allocationhandler->bindlessAccelerationStructures.descriptorSet;
			}
			else
			{
				// Unused, set dummy sampler set:
				descriptor_sets.sets[DESCRIPTOR_SET_BINDLESS_ACCELERATION_STRUCTURE] = allocationhandler->bindlessSamplers.descriptorSet;
			}
		}

#ifdef VULKAN_PIPELINE_CACHE_ENABLED
		// Pipeline Cache
		{
			// Try to read pipeline cache file if exists.
			Protean::vector<uint8_t> pipelineData;
			if (!Protean::Helper::FileRead(get_shader_cache_path(), pipelineData))
			{
				pipelineData.clear();
			}

			// Verify cache validation.
			if (!pipelineData.empty())
			{
				uint32_t headerLength = 0;
				uint32_t cacheHeaderVersion = 0;
				uint32_t vendorID = 0;
				uint32_t deviceID = 0;
				uint8_t pipelineCacheUUID[VK_UUID_SIZE] = {};

				std::memcpy(&headerLength, (uint8_t*)pipelineData.data() + 0, 4);
				std::memcpy(&cacheHeaderVersion, (uint8_t*)pipelineData.data() + 4, 4);
				std::memcpy(&vendorID, (uint8_t*)pipelineData.data() + 8, 4);
				std::memcpy(&deviceID, (uint8_t*)pipelineData.data() + 12, 4);
				std::memcpy(pipelineCacheUUID, (uint8_t*)pipelineData.data() + 16, VK_UUID_SIZE);

				bool badCache = false;

				if (headerLength <= 0)
				{
					badCache = true;
				}

				if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
				{
					badCache = true;
				}

				if (vendorID != properties2.properties.vendorID)
				{
					badCache = true;
				}

				if (deviceID != properties2.properties.deviceID)
				{
					badCache = true;
				}

				if (std::memcmp(pipelineCacheUUID, properties2.properties.pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0)
				{
					badCache = true;
				}

				if (badCache)
				{
					// Don't submit initial cache data if any version info is incorrect
					pipelineData.clear();
				}
			}

			VkPipelineCacheCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
			createInfo.initialDataSize = pipelineData.size();
			createInfo.pInitialData = pipelineData.data();

			// Create Vulkan pipeline cache
			vulkan_check(vkCreatePipelineCache(device, &createInfo, nullptr, &pipelineCache));
		}
#endif

		wilog("Created VulkanDevice (%d ms)\nAdapter: %s", (int)std::round(timer.elapsed()), adapterName.c_str());
	}

	VulkanDevice::~VulkanDevice()
	{
		vulkan_check(vkDeviceWaitIdle(device));

		for (uint32_t fr = 0; fr < BUFFERCOUNT; ++fr)
		{
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				VkFence fence = frame_fence[fr][queue];
				if (fence == VK_NULL_HANDLE)
					continue;
				vkDestroyFence(device, fence, nullptr);
				for (VkSemaphore semaphore : queues[queue].frame_semaphores[fr])
				{
					if (semaphore != VK_NULL_HANDLE)
					{
						vkDestroySemaphore(device, semaphore, nullptr);
					}
				}
			}
		}

		copyAllocator.destroy();

		for (auto& x : transition_handlers)
		{
			vkDestroyCommandPool(device, x.commandPool, nullptr);
			for (auto& y : x.semaphores)
			{
				vkDestroySemaphore(device, y, nullptr);
			}
		}

		for (auto& commandlist : commandlists)
		{
			for (int buffer = 0; buffer < BUFFERCOUNT; ++buffer)
			{
				for (int queue = 0; queue < QUEUE_COUNT; ++queue)
				{
					vkDestroyCommandPool(device, commandlist->commandPools[buffer][queue], nullptr);
				}
			}
			for (auto& x : commandlist->binder_pools)
			{
				x.destroy();
			}
		}

		for (auto& x : semaphore_pool)
		{
			vkDestroySemaphore(device, x, nullptr);
		}

		vmaDestroyBuffer(allocationhandler->allocator, nullBuffer, nullBufferAllocation);
		vkDestroySampler(device, nullSampler, nullptr);

		for (VkSampler sam : immutable_samplers)
		{
			vkDestroySampler(device, sam, nullptr);
		}

		if (pipelineCache != VK_NULL_HANDLE)
		{
			// Get size of pipeline cache
			size_t size{};
			vulkan_check(vkGetPipelineCacheData(device, pipelineCache, &size, nullptr));

			// Get data of pipeline cache
			Protean::vector<uint8_t> data(size);
			vulkan_check(vkGetPipelineCacheData(device, pipelineCache, &size, data.data()));

			// Write pipeline cache data to a file in binary format
			Protean::Helper::FileWrite(get_shader_cache_path(), data.data(), size);

			// Destroy Vulkan pipeline cache
			vkDestroyPipelineCache(device, pipelineCache, nullptr);
			pipelineCache = VK_NULL_HANDLE;
		}

		if (debugUtilsMessenger != VK_NULL_HANDLE)
		{
			vkDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
		}
	}

	CommandList VulkanDevice::BeginCommandList(QUEUE_TYPE queue)
	{
		cmd_locker.lock();
		uint32_t cmd_current = cmd_count++;
		if (cmd_current >= commandlists.size())
		{
			commandlists.push_back(cmd_allocator.allocate());
		}
		CommandList cmd;
		cmd.internal_state = commandlists[cmd_current];
		cmd_locker.unlock();

		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.reset(GetBufferIndex());
		commandlist.queue = queue;
		commandlist.id = cmd_current;

		if (commandlist.GetCommandBuffer() == VK_NULL_HANDLE)
		{
			// need to create one more command list:

			for (uint32_t buffer = 0; buffer < BUFFERCOUNT; ++buffer)
			{
				VkCommandPoolCreateInfo poolInfo = {};
				poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				switch (queue)
				{
				case Protean::Graphics::QUEUE_GRAPHICS:
					poolInfo.queueFamilyIndex = graphicsFamily;
					break;
				case Protean::Graphics::QUEUE_COMPUTE:
					poolInfo.queueFamilyIndex = computeFamily;
					break;
				case Protean::Graphics::QUEUE_COPY:
					poolInfo.queueFamilyIndex = copyFamily;
					break;
				case Protean::Graphics::QUEUE_VIDEO_DECODE:
					poolInfo.queueFamilyIndex = videoFamily;
					break;
				default:
					assert(0); // queue type not handled
					break;
				}
				poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

				vulkan_check(vkCreateCommandPool(device, &poolInfo, nullptr, &commandlist.commandPools[buffer][queue]));

				VkCommandBufferAllocateInfo commandBufferInfo = {};
				commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
				commandBufferInfo.commandBufferCount = 1;
				commandBufferInfo.commandPool = commandlist.commandPools[buffer][queue];
				commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

				vulkan_check(vkAllocateCommandBuffers(device, &commandBufferInfo, &commandlist.commandBuffers[buffer][queue]));

				commandlist.binder_pools[buffer].init(this);
			}

			commandlist.binder.init(this);
		}

		vulkan_check(vkResetCommandPool(device, commandlist.GetCommandPool(), 0));

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vulkan_check(vkBeginCommandBuffer(commandlist.GetCommandBuffer(), &beginInfo));

		if (queue == QUEUE_GRAPHICS)
		{
			vkCmdSetRasterizerDiscardEnable(commandlist.GetCommandBuffer(), VK_FALSE);

			VkViewport vp = {};
			vp.width = 1;
			vp.height = 1;
			vp.maxDepth = 1;
			vkCmdSetViewportWithCount(commandlist.GetCommandBuffer(), 1, &vp);

			VkRect2D scissor;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			scissor.extent.width = 65535;
			scissor.extent.height = 65535;
			vkCmdSetScissorWithCount(commandlist.GetCommandBuffer(), 1, &scissor);

			float blendConstants[] = { 1,1,1,1 };
			vkCmdSetBlendConstants(commandlist.GetCommandBuffer(), blendConstants);

			vkCmdSetStencilReference(commandlist.GetCommandBuffer(), VK_STENCIL_FRONT_AND_BACK, commandlist.prev_stencilref);

			if (features2.features.depthBounds == VK_TRUE)
			{
				vkCmdSetDepthBounds(commandlist.GetCommandBuffer(), 0.0f, 1.0f);
			}

			const VkDeviceSize zero = {};
			vkCmdBindVertexBuffers2(commandlist.GetCommandBuffer(), 0, 1, &nullBuffer, &zero, &zero, &zero);

			if (CheckCapability(GraphicsDeviceCapability_VARIABLE_RATE_SHADING))
			{
				VkExtent2D fragmentSize = {};
				fragmentSize.width = 1;
				fragmentSize.height = 1;

				VkFragmentShadingRateCombinerOpKHR combiner[] = {
					VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
					VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR
				};

				vkCmdSetFragmentShadingRateKHR(
					commandlist.GetCommandBuffer(),
					&fragmentSize,
					combiner
				);
			}
		}

		return cmd;
	}

	void VulkanDevice::SubmitCommandLists()
	{
		// Submit resource initialization transitions:
		{
			TransitionHandler& transition_handler = GetTransitionHandler();
			std::scoped_lock lck(transitionLocker);
			if (!init_transitions.empty())
			{
				if (transition_handler.commandBuffer == VK_NULL_HANDLE)
				{
					VkCommandPoolCreateInfo poolInfo = {};
					poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
					poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
					poolInfo.queueFamilyIndex = graphicsFamily;
					vulkan_check(vkCreateCommandPool(device, &poolInfo, nullptr, &transition_handler.commandPool));

					VkCommandBufferAllocateInfo commandBufferInfo = {};
					commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
					commandBufferInfo.commandBufferCount = 1;
					commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
					commandBufferInfo.commandPool = transition_handler.commandPool;
					vulkan_check(vkAllocateCommandBuffers(device, &commandBufferInfo, &transition_handler.commandBuffer));

					for (int i = 0; i < arraysize(transition_handler.semaphores); ++i)
					{
						VkSemaphoreCreateInfo info = {};
						info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
						vulkan_check(vkCreateSemaphore(device, &info, nullptr, &transition_handler.semaphores[i]));
					}
				}
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				beginInfo.pInheritanceInfo = nullptr;
				vulkan_check(vkResetCommandPool(device, transition_handler.commandPool, 0));
				vulkan_check(vkBeginCommandBuffer(transition_handler.commandBuffer, &beginInfo));
				VkDependencyInfo dependencyInfo = {};
				dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
				dependencyInfo.imageMemoryBarrierCount = (uint32_t)init_transitions.size();
				dependencyInfo.pImageMemoryBarriers = init_transitions.data();
				vkCmdPipelineBarrier2(transition_handler.commandBuffer, &dependencyInfo);
				vulkan_check(vkEndCommandBuffer(transition_handler.commandBuffer));
				CommandQueue& queue = queues[QUEUE_GRAPHICS];
				VkCommandBufferSubmitInfo& cmd_submit = queue.submit_cmds.emplace_back();
				cmd_submit.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
				cmd_submit.commandBuffer = transition_handler.commandBuffer;
				for (int q = QUEUE_GRAPHICS + 1; q < QUEUE_COUNT; ++q)
				{
					if (queues[q].queue == VK_NULL_HANDLE)
						continue;
					VkSemaphore sema = transition_handler.semaphores[q - 1];
					queue.signal(sema);
					queues[q].wait(sema);
				}
				queue.submit(this, VK_NULL_HANDLE);
				init_transitions.clear();
			}
		}

		// Submit current frame:
		{
			uint32_t cmd_last = cmd_count;
			cmd_count = 0;
			for (uint32_t cmd = 0; cmd < cmd_last; ++cmd)
			{
				CommandList_Vulkan& commandlist = *commandlists[cmd];
				vulkan_check(vkEndCommandBuffer(commandlist.GetCommandBuffer()));

				CommandQueue& queue = queues[commandlist.queue];
				const bool dependency = !commandlist.signals.empty() || !commandlist.waits.empty();

				if (dependency)
				{
					// If the current commandlist must resolve a dependency, then previous ones will be submitted before doing that:
					//	This improves GPU utilization because not the whole batch of command lists will need to synchronize, but only the one that handles it
					queue.submit(this, VK_NULL_HANDLE);
				}

				VkCommandBufferSubmitInfo& cbSubmitInfo = queue.submit_cmds.emplace_back();
				cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
				cbSubmitInfo.commandBuffer = commandlist.GetCommandBuffer();

				queue.swapchain_updates = commandlist.prev_swapchains;
				for (auto& swapchain : commandlist.prev_swapchains)
				{
					auto internal_state = to_internal(&swapchain);

					VkSemaphoreSubmitInfo& waitSemaphore = queue.submit_waitSemaphoreInfos.emplace_back();
					waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
					waitSemaphore.semaphore = internal_state->swapchainAcquireSemaphores[internal_state->swapChainAcquireSemaphoreIndex];
					waitSemaphore.value = 0; // not a timeline semaphore
					waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

					VkSemaphoreSubmitInfo& signalSemaphore = queue.submit_signalSemaphoreInfos.emplace_back();
					signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
					signalSemaphore.semaphore = internal_state->swapchainReleaseSemaphores[internal_state->swapChainImageIndex];
					signalSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
					signalSemaphore.value = 0; // not a timeline semaphore

					queue.swapchains.push_back(internal_state->swapChain);
					queue.swapchainImageIndices.push_back(internal_state->swapChainImageIndex);
					queue.swapchainWaitSemaphores.push_back(signalSemaphore.semaphore);
				}

				if (dependency)
				{
					for (auto& semaphore : commandlist.waits)
					{
						// Wait for command list dependency:
						queue.wait(semaphore);

						// semaphore is not recycled here, only the signals recycle themselves because wait will use the same
					}
					commandlist.waits.clear();

					for (auto& semaphore : commandlist.signals)
					{
						// Signal this command list's completion:
						queue.signal(semaphore);

						// recycle semaphore
						free_semaphore(semaphore);
					}
					commandlist.signals.clear();

					queue.submit(this, VK_NULL_HANDLE);
				}

				for (auto& x : commandlist.pipelines_worker)
				{
					if (pipelines_global.count(x.first) == 0)
					{
						pipelines_global[x.first] = x.second;
					}
				}
				commandlist.pipelines_worker.clear();
			}

			// final submits with fences:
			for (int q = 0; q < QUEUE_COUNT; ++q)
			{
				queues[q].submit(this, frame_fence[GetBufferIndex()][q]);
			}
		}

		// Sync up every queue to every other queue at the end of the frame:
		//	Note: it disables overlapping queues into the next frame
		//	Note: it's not submitted immediately here, but the waits are recorded before next frame submits
		for (int queue1 = 0; queue1 < QUEUE_COUNT; ++queue1)
		{
			if (queues[queue1].queue == nullptr)
				continue;
			for (int queue2 = 0; queue2 < QUEUE_COUNT; ++queue2)
			{
				if (queue1 == queue2)
					continue;
				VkSemaphore semaphore = queues[queue2].frame_semaphores[GetBufferIndex()][queue1];
				if (semaphore == VK_NULL_HANDLE)
					continue;
				queues[queue1].wait(semaphore);
			}
		}

		// From here, we begin a new frame, this affects GetBufferIndex()!
		FRAMECOUNT++;

		// Initiate stalling CPU when GPU is not yet finished with next frame:
		if (FRAMECOUNT >= BUFFERCOUNT)
		{
			const uint32_t bufferindex = GetBufferIndex();
			VkFence waitFences[QUEUE_COUNT] = {};
			uint32_t waitFenceCount = 0;
			VkFence resetFences[QUEUE_COUNT] = {};
			uint32_t resetFenceCount = 0;
			for (int queue = 0; queue < QUEUE_COUNT; ++queue)
			{
				VkFence fence = frame_fence[bufferindex][queue];
				if (fence == VK_NULL_HANDLE)
					continue;
				resetFences[resetFenceCount++] = fence;
				if (vkGetFenceStatus(device, fence) == VK_SUCCESS)
					continue;
				waitFences[waitFenceCount++] = fence;
			}
			if (waitFenceCount > 0)
			{
				while (vulkan_check(vkWaitForFences(device, waitFenceCount, waitFences, VK_TRUE, timeout_value)) == VK_TIMEOUT)
				{
					wilog_error(
						"[SubmitCommandLists] vkWaitForFences resulted in VK_TIMEOUT, fence statuses:\nQUEUE_GRAPHICS = %s\nQUEUE_COMPUTE = %s\nQUEUE_COPY = %s\nQUEUE_VIDEO_DECODE = %s",
						frame_fence[bufferindex][QUEUE_GRAPHICS] == VK_NULL_HANDLE ? "OK" : string_VkResult(vkGetFenceStatus(device, frame_fence[bufferindex][QUEUE_GRAPHICS])),
						frame_fence[bufferindex][QUEUE_COMPUTE] == VK_NULL_HANDLE ? "OK" : string_VkResult(vkGetFenceStatus(device, frame_fence[bufferindex][QUEUE_COMPUTE])),
						frame_fence[bufferindex][QUEUE_COPY] == VK_NULL_HANDLE ? "OK" : string_VkResult(vkGetFenceStatus(device, frame_fence[bufferindex][QUEUE_COPY])),
						frame_fence[bufferindex][QUEUE_VIDEO_DECODE] == VK_NULL_HANDLE ? "OK" : string_VkResult(vkGetFenceStatus(device, frame_fence[bufferindex][QUEUE_VIDEO_DECODE]))
					);
					std::this_thread::yield();
				}
			}
			if (resetFenceCount > 0)
			{
				vulkan_check(vkResetFences(device, resetFenceCount, resetFences));
			}
		}

		for (int q = 0; q < QUEUE_COUNT; ++q)
		{
			if (queues[q].queue == VK_NULL_HANDLE)
			{
				queues[q].clear();
			}
		}

		allocationhandler->Update(FRAMECOUNT, BUFFERCOUNT);

		// Reusing completed async copies:
		{
			std::scoped_lock lck(copyAllocator.locker);
			for (auto& sema : copyAllocator.async_semaphore_recycle[GetBufferIndex()]) // first free this buffer's recycleable semaphores, only after add to it!
			{
				free_semaphore(sema);
			}
			copyAllocator.async_semaphore_recycle[GetBufferIndex()].clear();
			while (!copyAllocator.async_worklist.empty() && vkGetFenceStatus(device, copyAllocator.async_worklist.front().fence) == VK_SUCCESS)
			{
				for (auto& sema : copyAllocator.async_worklist.front().semaphores)
				{
					copyAllocator.async_semaphore_recycle[GetBufferIndex()].push_back(sema); // delayed recycling with frame buffering
				}
				copyAllocator.async_worklist.front().semaphores.clear();
				copyAllocator.freelist.push_back(std::move(copyAllocator.async_worklist.front()));
				copyAllocator.async_worklist.pop_front();
			}
		}
	}

	void VulkanDevice::WaitForGPU() const
	{
		vulkan_check(vkDeviceWaitIdle(device));
	}

	void VulkanDevice::ClearPipelineStateCache()
	{
		layout_locker.lock();
		pso_layouts.clear();
		layout_locker.unlock();

		pipelines_global.clear();

		for (auto& x : commandlists)
		{
			x->pipelines_worker.clear();
		}

		if (pipelineCache != VK_NULL_HANDLE)
		{
			vkDestroyPipelineCache(device, pipelineCache, nullptr);
			pipelineCache = VK_NULL_HANDLE;

			VkPipelineCacheCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
			createInfo.initialDataSize = 0;
			createInfo.pInitialData = nullptr;
			vulkan_check(vkCreatePipelineCache(device, &createInfo, nullptr, &pipelineCache));
		}
	}

	VkDevice VulkanDevice::GetDevice()
	{
		return device;
	}

	VkImage VulkanDevice::GetTextureInternalResource(const Texture* texture)
	{
		return to_internal(texture)->resource;
	}

	VkPhysicalDevice VulkanDevice::GetPhysicalDevice()
	{
		return physicalDevice;
	}

	VkInstance VulkanDevice::GetInstance()
	{
		return instance;
	}

	VkQueue VulkanDevice::GetGraphicsCommandQueue()
	{
		return queues[QUEUE_GRAPHICS].queue;
	}

	uint32_t VulkanDevice::GetGraphicsFamilyIndex()
	{
		return graphicsFamily;
	}
}

#endif // WICKEDENGINE_BUILD_VULKAN
