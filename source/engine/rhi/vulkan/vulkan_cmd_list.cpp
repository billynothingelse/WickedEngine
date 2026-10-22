#include "rhi/vulkan/vulkan_internal.h"
// Vulkan backend: Command list recording

#ifdef WICKEDENGINE_BUILD_VULKAN

namespace Protean::Graphics
{
	using namespace VulkanInternal;

	void VulkanDevice::pso_validate(CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (!commandlist.dirty_pso)
			return;

		const PipelineState* pso = commandlist.active_pso;
		const PipelineHash& pipeline_hash = commandlist.prev_pipeline_hash;
		auto internal_state = to_internal(pso);

		VkPipeline pipeline = VK_NULL_HANDLE;
		auto it = pipelines_global.find(pipeline_hash);
		if (it == pipelines_global.end())
		{
			for (auto& x : commandlist.pipelines_worker)
			{
				if (pipeline_hash == x.first)
				{
					auto pipeline_internal = to_internal(&x.second);
					pipeline = pipeline_internal->pipeline;
					break;
				}
			}

			if (pipeline == VK_NULL_HANDLE)
			{
				PipelineState just_in_time_pso;
				bool success = CreatePipelineState(&pso->desc, &just_in_time_pso, &commandlist.renderpass_info);
				assert(success);

				commandlist.pipelines_worker.push_back(std::make_pair(pipeline_hash, just_in_time_pso));

				auto pipeline_internal = to_internal(&just_in_time_pso);
				pipeline = pipeline_internal->pipeline;
			}
		}
		else
		{
			auto pipeline_internal = to_internal(&it->second);
			pipeline = pipeline_internal->pipeline;
		}
		assert(pipeline != VK_NULL_HANDLE);

		vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		commandlist.dirty_pso = false;
	}

	void VulkanDevice::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(true, cmd);
	}

	void VulkanDevice::predispatch(CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(false, cmd);
	}

	void VulkanDevice::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		CommandList_Vulkan& commandlist_wait_for = GetCommandList(wait_for);
		assert(commandlist_wait_for.id < commandlist.id); // can't wait for future command list!
		VkSemaphore semaphore = new_semaphore();
		commandlist.waits.push_back(semaphore);
		commandlist_wait_for.signals.push_back(semaphore);
	}

	void VulkanDevice::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();
		auto internal_state = to_internal(swapchain);

		internal_state->locker.lock();
		internal_state->swapChainAcquireSemaphoreIndex = (internal_state->swapChainAcquireSemaphoreIndex + 1) % internal_state->swapchainAcquireSemaphores.size();
		VkResult res;
		do {
			res = vkAcquireNextImageKHR(
				device,
				internal_state->swapChain,
				timeout_value,
				internal_state->swapchainAcquireSemaphores[internal_state->swapChainAcquireSemaphoreIndex],
				VK_NULL_HANDLE,
				&internal_state->swapChainImageIndex
			);
			if (res == VK_TIMEOUT)
			{
				wilog_error("vkAcquireNextImageKHR resulted in VK_TIMEOUT, retrying");
				std::this_thread::yield();
			}
		} while (res == VK_TIMEOUT);
		internal_state->locker.unlock();

		if (res != VK_SUCCESS)
		{
			// Handle outdated error in acquire:
			if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
			{
				// we need to create a new semaphore or jump through a few hoops to
				// wait for the current one to be unsignalled before we can use it again
				// creating a new one is easiest. See also:
				// https://github.com/KhronosGroup/Vulkan-Docs/issues/152
				// https://www.khronos.org/blog/resolving-longstanding-issues-with-wsi
				{
					std::scoped_lock lock(allocationhandler->destroylocker);
					for (auto& x : internal_state->swapchainAcquireSemaphores)
					{
						allocationhandler->destroyer_semaphores.emplace_back(x, allocationhandler->framecount);
					}
				}
				internal_state->swapchainAcquireSemaphores.clear();
				if (CreateSwapChainInternal(internal_state, physicalDevice, device, allocationhandler))
				{
					RenderPassBegin(swapchain, cmd);
					return;
				}
			}
			assert(0);
		}
		commandlist.prev_swapchains.push_back(*swapchain);

		VkRenderingInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		info.renderArea.offset.x = 0;
		info.renderArea.offset.y = 0;
		info.renderArea.extent.width = std::min(swapchain->desc.width, internal_state->swapChainExtent.width);
		info.renderArea.extent.height = std::min(swapchain->desc.height, internal_state->swapChainExtent.height);
		info.layerCount = 1;

		VkRenderingAttachmentInfo color_attachment = {};
		color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_attachment.imageView = internal_state->textures[internal_state->swapChainImageIndex]->rtv.image_view;
		color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.clearValue.color.float32[0] = swapchain->desc.clear_color[0];
		color_attachment.clearValue.color.float32[1] = swapchain->desc.clear_color[1];
		color_attachment.clearValue.color.float32[2] = swapchain->desc.clear_color[2];
		color_attachment.clearValue.color.float32[3] = swapchain->desc.clear_color[3];

		info.colorAttachmentCount = 1;
		info.pColorAttachments = &color_attachment;

		VkImageMemoryBarrier2 barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.image = internal_state->textures[internal_state->swapChainImageIndex]->resource;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		VkDependencyInfo dependencyInfo = {};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;
		vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);

		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_NONE;
		commandlist.renderpass_barriers_end.push_back(barrier);

		vkCmdBeginRendering(commandlist.GetCommandBuffer(), &info);

		commandlist.renderpass_info = RenderPassInfo::from(swapchain->desc);
	}

	void VulkanDevice::RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();

		VkRenderingInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		if (has_flag(flags, RenderPassFlags_SUSPENDING))
		{
			info.flags |= VK_RENDERING_SUSPENDING_BIT;
		}
		if (has_flag(flags, RenderPassFlags_RESUMING))
		{
			info.flags |= VK_RENDERING_RESUMING_BIT;
		}
		info.layerCount = 1;
		info.renderArea.offset.x = 0;
		info.renderArea.offset.y = 0;
		info.renderArea.extent.width = properties2.properties.limits.maxFramebufferWidth;
		info.renderArea.extent.height = properties2.properties.limits.maxFramebufferHeight;
		VkRenderingAttachmentInfo color_attachments[8] = {};
		VkRenderingAttachmentInfo depth_attachment = {};
		VkRenderingAttachmentInfo stencil_attachment = {};
		VkRenderingFragmentShadingRateAttachmentInfoKHR shading_rate_attachment = {};
		bool color = false;
		bool depth = false;
		bool stencil = false;
		uint32_t color_resolve_count = 0;
		for (uint32_t i = 0; i < image_count; ++i)
		{
			const RenderPassImage& image = images[i];
			const Texture* texture = image.texture;
			const TextureDesc& desc = texture->GetDesc();
			int subresource = image.subresource;
			auto internal_state = to_internal(texture);

			if (image.type == RenderPassImage::Type_RENDERTARGET || image.type == RenderPassImage::Type_DEPTH_STENCIL)
			{
				info.renderArea.extent.width = std::min(info.renderArea.extent.width, desc.width);
				info.renderArea.extent.height = std::min(info.renderArea.extent.height, desc.height);
			}

			VkAttachmentLoadOp loadOp;
			switch (image.loadop)
			{
			default:
			case RenderPassImage::LoadOp_LOAD:
				loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				break;
			case RenderPassImage::LoadOp_CLEAR:
				loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				break;
			case RenderPassImage::LoadOp_DONTCARE:
				loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				break;
			}

			VkAttachmentStoreOp storeOp;
			switch (image.storeop)
			{
			default:
			case RenderPassImage::StoreOp_STORE:
				storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				break;
			case RenderPassImage::StoreOp_DONTCARE:
				storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				break;
			}

			Texture_Vulkan::TextureSubresource descriptor;

			switch (image.type)
			{
			case RenderPassImage::Type_RENDERTARGET:
			{
				descriptor = subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[subresource];
				VkRenderingAttachmentInfo& color_attachment = color_attachments[info.colorAttachmentCount++];
				color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				color_attachment.imageView = descriptor.image_view;
				color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				color_attachment.loadOp = loadOp;
				color_attachment.storeOp = storeOp;
				color_attachment.clearValue.color.float32[0] = desc.clear.color[0];
				color_attachment.clearValue.color.float32[1] = desc.clear.color[1];
				color_attachment.clearValue.color.float32[2] = desc.clear.color[2];
				color_attachment.clearValue.color.float32[3] = desc.clear.color[3];
				color = true;
			}
			break;

			case RenderPassImage::Type_RESOLVE:
			{
				descriptor = subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[subresource];
				VkRenderingAttachmentInfo& color_attachment = color_attachments[color_resolve_count++];
				color_attachment.resolveImageView = descriptor.image_view;
				color_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				color_attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
			}
			break;

			case RenderPassImage::Type_DEPTH_STENCIL:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				depth_attachment.imageView = descriptor.image_view;
				if (image.layout == ResourceState_DEPTHSTENCIL_READONLY)
				{
					depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				}
				else
				{
					depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				}
				depth_attachment.loadOp = loadOp;
				depth_attachment.storeOp = storeOp;
				depth_attachment.clearValue.depthStencil.depth = desc.clear.depth_stencil.depth;
				depth = true;
				if (IsFormatStencilSupport(desc.format))
				{
					stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
					stencil_attachment.imageView = subresource < 0 ? internal_state->dsv.image_view : internal_state->subresources_dsv[subresource].image_view;
					if (image.layout == ResourceState_DEPTHSTENCIL_READONLY)
					{
						stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
					}
					else
					{
						stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
					}
					stencil_attachment.loadOp = loadOp;
					stencil_attachment.storeOp = storeOp;
					stencil_attachment.clearValue.depthStencil.stencil = desc.clear.depth_stencil.stencil;
					stencil = true;
				}
			}
			break;

			case RenderPassImage::Type_RESOLVE_DEPTH:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				depth_attachment.resolveImageView = descriptor.image_view;
				stencil_attachment.resolveImageView = descriptor.image_view;
				depth_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				stencil_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
				switch (image.depth_resolve_mode)
				{
				default:
				case RenderPassImage::DepthResolveMode_Min:
					depth_attachment.resolveMode = VK_RESOLVE_MODE_MIN_BIT;
					stencil_attachment.resolveMode = VK_RESOLVE_MODE_MIN_BIT;
					break;
				case RenderPassImage::DepthResolveMode_Max:
					depth_attachment.resolveMode = VK_RESOLVE_MODE_MAX_BIT;
					stencil_attachment.resolveMode = VK_RESOLVE_MODE_MAX_BIT;
					break;
				}
			}
			break;

			case RenderPassImage::Type_SHADING_RATE_SOURCE:
				descriptor = subresource < 0 ? internal_state->uav : internal_state->subresources_uav[subresource];
				shading_rate_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
				shading_rate_attachment.imageView = descriptor.image_view;
				shading_rate_attachment.imageLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
				shading_rate_attachment.shadingRateAttachmentTexelSize.width = VARIABLE_RATE_SHADING_TILE_SIZE;
				shading_rate_attachment.shadingRateAttachmentTexelSize.height = VARIABLE_RATE_SHADING_TILE_SIZE;
				info.pNext = &shading_rate_attachment;
				break;
			default:
				break;
			}

			if (image.layout_before != image.layout)
			{
				VkImageMemoryBarrier2& barrier = commandlist.renderpass_barriers_begin.emplace_back();
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.image = internal_state->resource;
				barrier.oldLayout = _ConvertImageLayout(image.layout_before);
				barrier.newLayout = _ConvertImageLayout(image.layout);

				assert(barrier.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

				barrier.srcStageMask = _ConvertPipelineStage(image.layout_before);
				barrier.dstStageMask = _ConvertPipelineStage(image.layout);
				barrier.srcAccessMask = _ParseResourceState(image.layout_before);
				barrier.dstAccessMask = _ParseResourceState(image.layout);

				if (IsFormatDepthSupport(desc.format))
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				barrier.subresourceRange.baseMipLevel = descriptor.firstMip;
				barrier.subresourceRange.levelCount = descriptor.mipCount;
				barrier.subresourceRange.baseArrayLayer = descriptor.firstSlice;
				barrier.subresourceRange.layerCount = descriptor.sliceCount;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			if (image.layout != image.layout_after)
			{
				VkImageMemoryBarrier2& barrier = commandlist.renderpass_barriers_end.emplace_back();
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.image = internal_state->resource;
				barrier.oldLayout = _ConvertImageLayout(image.layout);
				barrier.newLayout = _ConvertImageLayout(image.layout_after);

				assert(barrier.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

				barrier.srcStageMask = _ConvertPipelineStage(image.layout);
				barrier.dstStageMask = _ConvertPipelineStage(image.layout_after);
				barrier.srcAccessMask = _ParseResourceState(image.layout);
				barrier.dstAccessMask = _ParseResourceState(image.layout_after);

				if (IsFormatDepthSupport(desc.format))
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				barrier.subresourceRange.baseMipLevel = descriptor.firstMip;
				barrier.subresourceRange.levelCount = descriptor.mipCount;
				barrier.subresourceRange.baseArrayLayer = descriptor.firstSlice;
				barrier.subresourceRange.layerCount = descriptor.sliceCount;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			info.layerCount = std::min(desc.array_size, std::max(info.layerCount, descriptor.sliceCount));
		}
		info.pColorAttachments = color ? color_attachments : nullptr;
		info.pDepthAttachment = depth ? &depth_attachment : nullptr;
		info.pStencilAttachment = stencil ? &stencil_attachment : nullptr;

		if (!commandlist.renderpass_barriers_begin.empty())
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(commandlist.renderpass_barriers_begin.size());
			dependencyInfo.pImageMemoryBarriers = commandlist.renderpass_barriers_begin.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);
		}

		vkCmdBeginRendering(commandlist.GetCommandBuffer(), &info);

		commandlist.renderpass_info = RenderPassInfo::from(images, image_count);
	}

	void VulkanDevice::RenderPassEnd(CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdEndRendering(commandlist.GetCommandBuffer());

		if (!commandlist.renderpass_barriers_end.empty())
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(commandlist.renderpass_barriers_end.size());
			dependencyInfo.pImageMemoryBarriers = commandlist.renderpass_barriers_end.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);
			commandlist.renderpass_barriers_end.clear();
		}

		commandlist.renderpass_info = {};
	}

	void VulkanDevice::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd)
	{
		assert(rects != nullptr);
		VkRect2D scissors[16];
		assert(numRects <= arraysize(scissors));
		assert(numRects <= properties2.properties.limits.maxViewports);
		for(uint32_t i = 0; i < numRects; ++i)
		{
			scissors[i].extent.width = abs(rects[i].right - rects[i].left);
			scissors[i].extent.height = abs(rects[i].top - rects[i].bottom);
			scissors[i].offset.x = std::max(0, rects[i].left);
			scissors[i].offset.y = std::max(0, rects[i].top);
		}
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdSetScissorWithCount(commandlist.GetCommandBuffer(), numRects, scissors);
	}

	void VulkanDevice::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(pViewports != nullptr);
		VkViewport vp[16];
		assert(NumViewports <= arraysize(vp));
		assert(NumViewports <= properties2.properties.limits.maxViewports);
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			vp[i].x = pViewports[i].top_left_x;
			vp[i].y = pViewports[i].top_left_y + pViewports[i].height;
			vp[i].width = std::max(1.0f, pViewports[i].width); // must be > 0 according to validation layer
			vp[i].height = -pViewports[i].height;
			vp[i].minDepth = pViewports[i].min_depth;
			vp[i].maxDepth = pViewports[i].max_depth;
		}
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdSetViewportWithCount(commandlist.GetCommandBuffer(), NumViewports, vp);
	}

	void VulkanDevice::BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SRV_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.SRV[slot].internal_state != resource->internal_state || binder.table.SRV_index[slot] != subresource)
		{
			binder.table.SRV[slot] = *resource;
			binder.table.SRV_index[slot] = subresource;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}

	void VulkanDevice::BindResources(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(resources[i], slot + i, cmd, -1);
			}
		}
	}

	void VulkanDevice::BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_UAV_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.UAV[slot].internal_state != resource->internal_state || binder.table.UAV_index[slot] != subresource)
		{
			binder.table.UAV[slot] = *resource;
			binder.table.UAV_index[slot] = subresource;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}

	void VulkanDevice::BindUAVs(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(resources[i], slot + i, cmd, -1);
			}
		}
	}

	void VulkanDevice::BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SAMPLER_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.SAM[slot].internal_state != sampler->internal_state)
		{
			binder.table.SAM[slot] = *sampler;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}

	void VulkanDevice::BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_CBV_COUNT);
		auto& binder = commandlist.binder;

		if (binder.table.CBV[slot].internal_state != buffer->internal_state)
		{
			binder.table.CBV[slot] = *buffer;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}

		if (binder.table.CBV_offset[slot] != offset)
		{
			binder.table.CBV_offset[slot] = offset;
			if (slot < dynamic_cbv_count)
			{
				binder.dirty |= DescriptorBinder::DIRTY_SET_OR_OFFSET;
			}
			else
			{
				binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
			}
		}
	}

	void VulkanDevice::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		VkDeviceSize voffsets[8] = {};
		VkDeviceSize vstrides[8] = {};
		VkBuffer vbuffers[8] = {};
		assert(count <= 8);
		for (uint32_t i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] == nullptr || !vertexBuffers[i]->IsValid())
			{
				vbuffers[i] = nullBuffer;
			}
			else
			{
				auto internal_state = to_internal(vertexBuffers[i]);
				vbuffers[i] = internal_state->resource;
				if (offsets != nullptr)
				{
					voffsets[i] = offsets[i];
				}
				if (strides != nullptr)
				{
					vstrides[i] = strides[i];
				}
			}
		}

		vkCmdBindVertexBuffers2(commandlist.GetCommandBuffer(), slot, count, vbuffers, voffsets, nullptr, vstrides);
	}

	void VulkanDevice::BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd)
	{
		if (indexBuffer != nullptr)
		{
			auto internal_state = to_internal(indexBuffer);
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdBindIndexBuffer(commandlist.GetCommandBuffer(), internal_state->resource, offset, format == IndexBufferFormat_UINT16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		}
	}

	void VulkanDevice::BindStencilRef(uint32_t value, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (commandlist.prev_stencilref != value)
		{
			commandlist.prev_stencilref = value;
			vkCmdSetStencilReference(commandlist.GetCommandBuffer(), VK_STENCIL_FRONT_AND_BACK, value);
		}
	}

	void VulkanDevice::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		float blendConstants[] = { r, g, b, a };
		vkCmdSetBlendConstants(commandlist.GetCommandBuffer(), blendConstants);
	}

	void VulkanDevice::BindShadingRate(ShadingRate rate, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (CheckCapability(GraphicsDeviceCapability_VARIABLE_RATE_SHADING) && commandlist.prev_shadingrate != rate)
		{
			commandlist.prev_shadingrate = rate;

			VkExtent2D fragmentSize;
			switch (rate)
			{
			case ShadingRate_RATE_1X1:
				fragmentSize.width = 1;
				fragmentSize.height = 1;
				break;
			case ShadingRate_RATE_1X2:
				fragmentSize.width = 1;
				fragmentSize.height = 2;
				break;
			case ShadingRate_RATE_2X1:
				fragmentSize.width = 2;
				fragmentSize.height = 1;
				break;
			case ShadingRate_RATE_2X2:
				fragmentSize.width = 2;
				fragmentSize.height = 2;
				break;
			case ShadingRate_RATE_2X4:
				fragmentSize.width = 2;
				fragmentSize.height = 4;
				break;
			case ShadingRate_RATE_4X2:
				fragmentSize.width = 4;
				fragmentSize.height = 2;
				break;
			case ShadingRate_RATE_4X4:
				fragmentSize.width = 4;
				fragmentSize.height = 4;
				break;
			default:
				break;
			}

			VkFragmentShadingRateCombinerOpKHR combiner[] = {
				VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
				VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR
			};

			if (fragment_shading_rate_properties.fragmentShadingRateNonTrivialCombinerOps == VK_TRUE)
			{
				if (fragment_shading_rate_features.primitiveFragmentShadingRate == VK_TRUE)
				{
					combiner[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
				}
				if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
				{
					combiner[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
				}
			}
			else
			{
				if (fragment_shading_rate_features.primitiveFragmentShadingRate == VK_TRUE)
				{
					combiner[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
				}
				if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
				{
					combiner[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
				}
			}

			vkCmdSetFragmentShadingRateKHR(
				commandlist.GetCommandBuffer(),
				&fragmentSize,
				combiner
			);
		}
	}

	void VulkanDevice::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (commandlist.active_cs != nullptr)
		{
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_SET_OR_OFFSET; // need to refresh bind point at least when gfx/compute changes!
		}
		commandlist.active_cs = nullptr;
		commandlist.active_rt = nullptr;

		auto internal_state = to_internal(pso);

		if (commandlist.layout.pipeline_layout != internal_state->layout.pipeline_layout)
		{
			commandlist.layout = internal_state->layout;
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}

		if (internal_state->pipeline != VK_NULL_HANDLE)
		{
			if (commandlist.active_pso != pso)
			{
				vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, internal_state->pipeline);
			}
			else
				return; // early exit for static pso

			commandlist.prev_pipeline_hash = {};
			commandlist.dirty_pso = false;
		}
		else
		{
			PipelineHash pipeline_hash;
			pipeline_hash.pso = pso;
			pipeline_hash.renderpass_hash = commandlist.renderpass_info.get_hash();
			if (commandlist.prev_pipeline_hash == pipeline_hash)
			{
				commandlist.active_pso = pso;
				return; // early exit for dynamic pso|renderpass
			}
			commandlist.prev_pipeline_hash = pipeline_hash;
			commandlist.dirty_pso = true;
		}

		commandlist.active_pso = pso;
	}

	void VulkanDevice::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (commandlist.active_cs == cs)
			return;
		if (commandlist.active_pso != nullptr)
		{
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_SET_OR_OFFSET; // need to refresh bind point at least when gfx/compute changes!
		}
		commandlist.active_pso = nullptr;
		commandlist.active_rt = nullptr;

		assert(cs->stage == ShaderStage_CS || cs->stage == ShaderStage_LIB);

		auto internal_state = to_internal(cs);

		if (cs->stage == ShaderStage_CS)
		{
			vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, internal_state->pipeline_cs);
		}

		if (commandlist.layout.pipeline_layout != internal_state->layout.pipeline_layout)
		{
			commandlist.layout = internal_state->layout;
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}

		commandlist.active_cs = cs;
	}

	void VulkanDevice::BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd)
	{
		if (features2.features.depthBounds == VK_TRUE)
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdSetDepthBounds(commandlist.GetCommandBuffer(), min_bounds, max_bounds);
		}
	}

	void VulkanDevice::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDraw(commandlist.GetCommandBuffer(), vertexCount, 1, startVertexLocation, 0);
	}

	void VulkanDevice::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexed(commandlist.GetCommandBuffer(), indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}

	void VulkanDevice::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDraw(commandlist.GetCommandBuffer(), vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}

	void VulkanDevice::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexed(commandlist.GetCommandBuffer(), indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}

	void VulkanDevice::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, (uint32_t)sizeof(VkDrawIndirectCommand));
	}

	void VulkanDevice::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexedIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, sizeof(VkDrawIndexedIndirectCommand));
	}

	void VulkanDevice::DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndirectCount(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDrawIndirectCommand));
	}

	void VulkanDevice::DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexedIndirectCount(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDrawIndexedIndirectCommand));
	}

	void VulkanDevice::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDispatch(commandlist.GetCommandBuffer(), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void VulkanDevice::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDispatchIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset);
	}

	void VulkanDevice::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksEXT(commandlist.GetCommandBuffer(), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void VulkanDevice::DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksIndirectEXT(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, sizeof(VkDispatchIndirectCommand));
	}

	void VulkanDevice::DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksIndirectCountEXT(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDispatchIndirectCommand));
	}

	void VulkanDevice::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (pDst->type == GPUResource::Type_TEXTURE && pSrc->type == GPUResource::Type_TEXTURE)
		{
			auto internal_state_src = to_internal<Texture>(pSrc);
			auto internal_state_dst = to_internal<Texture>(pDst);

			const TextureDesc& src_desc = ((const Texture*)pSrc)->GetDesc();
			const TextureDesc& dst_desc = ((const Texture*)pDst)->GetDesc();

			if (src_desc.usage == Usage_UPLOAD && dst_desc.usage == Usage_DEFAULT)
			{
				// CPU (buffer) -> GPU (texture)
				VkBufferImageCopy copy = {};
				const uint32_t block_size = GetFormatBlockSize(dst_desc.format);
				const uint32_t planes = GetFormatPlaneCount(dst_desc.format);
				for (uint32_t plane = 0; plane < planes; ++plane)
				{
					const uint32_t data_stride = GetFormatStride(dst_desc.format, plane);
					const ImageAspect aspect = GetImageAspect(dst_desc.format, plane); // dst aspect!
					copy.imageSubresource.aspectMask = _ConvertImageAspect(aspect);
					for (uint32_t slice = 0; slice < dst_desc.array_size; ++slice)
					{
						copy.imageSubresource.baseArrayLayer = slice;
						copy.imageSubresource.layerCount = 1;
						uint32_t mip_width = dst_desc.width;
						uint32_t mip_height = dst_desc.height;
						uint32_t mip_depth = dst_desc.depth;
						for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
						{
							const uint32_t src_subresource = ComputeSubresource(mip, slice, aspect, src_desc.mip_levels, src_desc.array_size);
							const SubresourceData& src_subresourcedata = internal_state_dst->mapped_subresources[src_subresource];
							const uint32_t num_blocks_x = mip_width / block_size;
							const uint32_t num_blocks_y = mip_height / block_size;
							copy.imageExtent.width = mip_width;
							copy.imageExtent.height = mip_height;
							copy.imageExtent.depth = mip_depth;
							copy.imageSubresource.mipLevel = mip;
							copy.bufferRowLength = src_subresourcedata.row_pitch / data_stride * block_size;
							vkCmdCopyBufferToImage(
								commandlist.GetCommandBuffer(),
								internal_state_src->staging_resource,
								internal_state_dst->resource,
								_ConvertImageLayout(ResourceState_COPY_DST),
								1,
								&copy
							);

							copy.bufferOffset += num_blocks_x * num_blocks_y * mip_depth * data_stride;
							mip_width = std::max(1u, mip_width / 2);
							mip_height = std::max(1u, mip_height / 2);
							mip_depth = std::max(1u, mip_depth / 2);
						}
					}
				}
			}
			else if (src_desc.usage == Usage_DEFAULT && dst_desc.usage == Usage_READBACK)
			{
				// GPU (texture) -> CPU (buffer)
				VkBufferImageCopy copy = {};
				const uint32_t block_size = GetFormatBlockSize(dst_desc.format);
				const uint32_t planes = GetFormatPlaneCount(dst_desc.format);
				for (uint32_t plane = 0; plane < planes; ++plane)
				{
					const uint32_t data_stride = GetFormatStride(dst_desc.format, plane);
					const ImageAspect aspect = GetImageAspect(src_desc.format, plane); // src aspect!
					copy.imageSubresource.aspectMask = _ConvertImageAspect(aspect);
					for (uint32_t slice = 0; slice < dst_desc.array_size; ++slice)
					{
						copy.imageSubresource.baseArrayLayer = slice;
						copy.imageSubresource.layerCount = 1;
						uint32_t mip_width = dst_desc.width;
						uint32_t mip_height = dst_desc.height;
						uint32_t mip_depth = dst_desc.depth;
						for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
						{
							const uint32_t dst_subresource = ComputeSubresource(mip, slice, aspect, dst_desc.mip_levels, dst_desc.array_size);
							const SubresourceData& dst_subresourcedata = internal_state_dst->mapped_subresources[dst_subresource];
							const uint32_t num_blocks_x = mip_width / block_size;
							const uint32_t num_blocks_y = mip_height / block_size;
							copy.imageExtent.width = mip_width;
							copy.imageExtent.height = mip_height;
							copy.imageExtent.depth = mip_depth;
							copy.imageSubresource.mipLevel = mip;
							copy.bufferRowLength = dst_subresourcedata.row_pitch / data_stride * block_size;
							vkCmdCopyImageToBuffer(
								commandlist.GetCommandBuffer(),
								internal_state_src->resource,
								_ConvertImageLayout(ResourceState_COPY_SRC),
								internal_state_dst->staging_resource,
								1,
								&copy
							);

							copy.bufferOffset += num_blocks_x * num_blocks_y * mip_depth * data_stride;
							mip_width = std::max(1u, mip_width / 2);
							mip_height = std::max(1u, mip_height / 2);
							mip_depth = std::max(1u, mip_depth / 2);
						}
					}
				}
			}
			else if (src_desc.usage == Usage_DEFAULT && dst_desc.usage == Usage_DEFAULT)
			{
				// GPU (texture) -> GPU (texture)
				VkImageCopy copy = {};
				copy.extent.width = dst_desc.width;
				copy.extent.height = dst_desc.height;
				copy.extent.depth = std::max(1u, dst_desc.depth);

				copy.srcOffset.x = 0;
				copy.srcOffset.y = 0;
				copy.srcOffset.z = 0;

				copy.dstOffset.x = 0;
				copy.dstOffset.y = 0;
				copy.dstOffset.z = 0;

				if (IsFormatDepthSupport(src_desc.format))
				{
					copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(src_desc.format))
					{
						copy.srcSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				copy.srcSubresource.baseArrayLayer = 0;
				copy.srcSubresource.layerCount = src_desc.array_size;
				copy.srcSubresource.mipLevel = 0;

				if (has_flag(dst_desc.bind_flags, BindFlag_DEPTH_STENCIL))
				{
					copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(dst_desc.format))
					{
						copy.dstSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				copy.dstSubresource.baseArrayLayer = 0;
				copy.dstSubresource.layerCount = dst_desc.array_size;
				copy.dstSubresource.mipLevel = 0;

				vkCmdCopyImage(commandlist.GetCommandBuffer(),
					internal_state_src->resource, _ConvertImageLayout(ResourceState_COPY_SRC),
					internal_state_dst->resource, _ConvertImageLayout(ResourceState_COPY_DST),
					1, &copy
				);
			}
			else
			{
				// CPU (buffer) -> CPU (buffer)
				VkBufferCopy copy = {};
				copy.srcOffset = 0;
				copy.dstOffset = 0;
				copy.size = std::min(pSrc->mapped_size, pSrc->mapped_size);

				vkCmdCopyBuffer(commandlist.GetCommandBuffer(),
					internal_state_src->staging_resource,
					internal_state_dst->staging_resource,
					1, &copy
				);
			}
		}
		else if (pDst->type == GPUResource::Type_BUFFER && pSrc->type == GPUResource::Type_BUFFER)
		{
			auto internal_state_src = to_internal<GPUBuffer>(pSrc);
			auto internal_state_dst = to_internal<GPUBuffer>(pDst);

			const GPUBufferDesc& src_desc = ((const GPUBuffer*)pSrc)->GetDesc();
			const GPUBufferDesc& dst_desc = ((const GPUBuffer*)pDst)->GetDesc();

			VkBufferCopy copy = {};
			copy.srcOffset = 0;
			copy.dstOffset = 0;
			copy.size = std::min(src_desc.size, dst_desc.size);

			vkCmdCopyBuffer(commandlist.GetCommandBuffer(),
				internal_state_src->resource,
				internal_state_dst->resource,
				1, &copy
			);
		}
	}

	void VulkanDevice::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);

		VkBufferCopy copy = {};
		copy.srcOffset = src_offset;
		copy.dstOffset = dst_offset;
		copy.size = size;

		vkCmdCopyBuffer(commandlist.GetCommandBuffer(),
			internal_state_src->resource,
			internal_state_dst->resource,
			1, &copy
		);
	}

	void VulkanDevice::CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto src_internal = to_internal(src);
		auto dst_internal = to_internal(dst);

		const TextureDesc& src_desc = src->GetDesc();
		const TextureDesc& dst_desc = dst->GetDesc();

		if (src_desc.usage == Usage_UPLOAD && dst_desc.usage == Usage_DEFAULT)
		{
			// CPU (buffer) -> GPU (texture)
			VkBufferImageCopy copy = {};
			const uint32_t subresource_index = ComputeSubresource(srcMip, srcSlice, src_aspect, src_desc.mip_levels, src_desc.array_size);
			assert(src->mapped_subresource_count > subresource_index);
			const SubresourceData& data0 = src->mapped_subresources[0];
			const SubresourceData& data = src->mapped_subresources[subresource_index];
			const uint32_t data_stride = GetFormatStride(src_desc.format);
			const uint32_t block_size = GetFormatBlockSize(src_desc.format);
			copy.bufferOffset = (uint64_t)data.data_ptr - (uint64_t)data0.data_ptr;
			copy.bufferRowLength = data.row_pitch / data_stride * block_size;
			copy.bufferImageHeight = std::max(1u, src_desc.height >> srcMip);
			copy.imageSubresource.mipLevel = dstMip;
			copy.imageSubresource.baseArrayLayer = dstSlice;
			copy.imageSubresource.layerCount = 1;
			copy.imageSubresource.aspectMask = _ConvertImageAspect(dst_aspect);
			copy.imageOffset.x = dstX;
			copy.imageOffset.y = dstY;
			copy.imageOffset.z = dstZ;
			if (srcbox == nullptr)
			{
				copy.imageExtent.width = src_desc.width;
				copy.imageExtent.height = src_desc.height;
				copy.imageExtent.depth = src_desc.depth;
			}
			else
			{
				copy.imageExtent.width = srcbox->right - srcbox->left;
				copy.imageExtent.height = srcbox->bottom - srcbox->top;
				copy.imageExtent.depth = srcbox->back - srcbox->front;
				copy.bufferOffset += srcbox->top * data.row_pitch + srcbox->left * GetFormatStride(src_desc.format) / GetFormatBlockSize(src_desc.format);
			}
			vkCmdCopyBufferToImage(commandlist.GetCommandBuffer(), src_internal->staging_resource, dst_internal->resource, _ConvertImageLayout(ResourceState_COPY_DST), 1, &copy);
		}
		else if (src_desc.usage == Usage_DEFAULT && dst_desc.usage == Usage_READBACK)
		{
			// GPU (texture) -> CPU (buffer)
			VkBufferImageCopy copy = {};
			const uint32_t subresource_index = ComputeSubresource(dstMip, dstSlice, dst_aspect, dst_desc.mip_levels, dst_desc.array_size);
			assert(dst->mapped_subresource_count > subresource_index);
			const SubresourceData& data0 = dst->mapped_subresources[0];
			const SubresourceData& data = dst->mapped_subresources[subresource_index];
			const uint32_t data_stride = GetFormatStride(dst_desc.format);
			const uint32_t block_size = GetFormatBlockSize(dst_desc.format);
			copy.bufferOffset = (uint64_t)data.data_ptr - (uint64_t)data0.data_ptr;
			copy.bufferRowLength = data.row_pitch / data_stride * block_size;
			copy.bufferImageHeight = std::max(1u, dst_desc.height >> dstMip);
			copy.imageSubresource.mipLevel = srcMip;
			copy.imageSubresource.baseArrayLayer = srcSlice;
			copy.imageSubresource.layerCount = 1;
			copy.imageSubresource.aspectMask = _ConvertImageAspect(src_aspect);
			if (srcbox == nullptr)
			{
				copy.imageExtent.width = src_desc.width;
				copy.imageExtent.height = src_desc.height;
				copy.imageExtent.depth = src_desc.depth;
			}
			else
			{
				copy.imageExtent.width = srcbox->right - srcbox->left;
				copy.imageExtent.height = srcbox->bottom - srcbox->top;
				copy.imageExtent.depth = srcbox->back - srcbox->front;
				copy.imageOffset.x = srcbox->left;
				copy.imageOffset.y = srcbox->top;
				copy.imageOffset.z = srcbox->front;
			}
			vkCmdCopyImageToBuffer(commandlist.GetCommandBuffer(), src_internal->resource, _ConvertImageLayout(ResourceState_COPY_SRC), dst_internal->staging_resource, 1, &copy);
		}
		else if (src_desc.usage == Usage_DEFAULT && dst_desc.usage == Usage_DEFAULT)
		{
			// GPU (texture) -> GPU (texture)
			VkImageCopy copy = {};
			copy.dstSubresource.aspectMask = _ConvertImageAspect(dst_aspect);
			copy.dstSubresource.baseArrayLayer = dstSlice;
			copy.dstSubresource.layerCount = 1;
			copy.dstSubresource.mipLevel = dstMip;
			copy.dstOffset.x = dstX;
			copy.dstOffset.y = dstY;
			copy.dstOffset.z = dstZ;

			copy.srcSubresource.aspectMask = _ConvertImageAspect(src_aspect);
			copy.srcSubresource.baseArrayLayer = srcSlice;
			copy.srcSubresource.layerCount = 1;
			copy.srcSubresource.mipLevel = srcMip;

			if (srcbox == nullptr)
			{
				copy.srcOffset.x = 0;
				copy.srcOffset.y = 0;
				copy.srcOffset.z = 0;
				if (src->desc.format == Format_NV12 && src_aspect == ImageAspect_CHROMINANCE)
				{
					copy.extent.width = std::min(dst->desc.width, src->desc.width / 2);
					copy.extent.height = std::min(dst->desc.height, src->desc.height / 2);
				}
				else
				{
					copy.extent.width = std::min(dst->desc.width, src->desc.width);
					copy.extent.height = std::min(dst->desc.height, src->desc.height);
				}
				copy.extent.depth = std::min(dst->desc.depth, src->desc.depth);

				copy.extent.width = std::max(1u, copy.extent.width >> srcMip);
				copy.extent.height = std::max(1u, copy.extent.height >> srcMip);
				copy.extent.depth = std::max(1u, copy.extent.depth >> srcMip);
			}
			else
			{
				copy.srcOffset.x = srcbox->left;
				copy.srcOffset.y = srcbox->top;
				copy.srcOffset.z = srcbox->front;
				copy.extent.width = srcbox->right - srcbox->left;
				copy.extent.height = srcbox->bottom - srcbox->top;
				copy.extent.depth = srcbox->back - srcbox->front;
			}

			vkCmdCopyImage(
				commandlist.GetCommandBuffer(),
				src_internal->resource,
				_ConvertImageLayout(ResourceState_COPY_SRC),
				dst_internal->resource,
				_ConvertImageLayout(ResourceState_COPY_DST),
				1,
				&copy
			);
		}
		else
		{
			assert(0); // not supported
		}
	}

	void VulkanDevice::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType_OCCLUSION_BINARY:
			vkCmdBeginQuery(commandlist.GetCommandBuffer(), internal_state->pool, index, 0);
			break;
		case GpuQueryType_OCCLUSION:
			vkCmdBeginQuery(commandlist.GetCommandBuffer(), internal_state->pool, index, VK_QUERY_CONTROL_PRECISE_BIT);
			break;
		case GpuQueryType_TIMESTAMP:
			break;
		}
	}

	void VulkanDevice::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType_TIMESTAMP:
			vkCmdWriteTimestamp2(commandlist.GetCommandBuffer(), VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, internal_state->pool, index);
			break;
		case GpuQueryType_OCCLUSION_BINARY:
		case GpuQueryType_OCCLUSION:
			vkCmdEndQuery(commandlist.GetCommandBuffer(), internal_state->pool, index);
			break;
		}
	}

	void VulkanDevice::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);

		VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT;
		flags |= VK_QUERY_RESULT_WAIT_BIT;

		switch (heap->desc.type)
		{
		case GpuQueryType_OCCLUSION_BINARY:
			flags |= VK_QUERY_RESULT_PARTIAL_BIT;
			break;
		default:
			break;
		}

		vkCmdCopyQueryPoolResults(
			commandlist.GetCommandBuffer(),
			internal_state->pool,
			index,
			count,
			dst_internal->resource,
			dest_offset,
			sizeof(uint64_t),
			flags
		);

	}

	void VulkanDevice::QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);

		vkCmdResetQueryPool(
			commandlist.GetCommandBuffer(),
			internal_state->pool,
			index,
			count
		);

	}

	void VulkanDevice::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto& memoryBarriers = commandlist.frame_memoryBarriers;
		auto& imageBarriers = commandlist.frame_imageBarriers;
		auto& bufferBarriers = commandlist.frame_bufferBarriers;

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];

			if (barrier.type == GPUBarrier::Type_IMAGE && (barrier.image.texture == nullptr || !barrier.image.texture->IsValid()))
				continue;
			if (barrier.type == GPUBarrier::Type_BUFFER && (barrier.buffer.buffer == nullptr || !barrier.buffer.buffer->IsValid()))
				continue;

			switch (barrier.type)
			{
			default:
			case GPUBarrier::Type_MEMORY:
			case GPUBarrier::Type_ALIASING:
			{
				VkMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrierdesc.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrierdesc.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
				barrierdesc.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;

				if (CheckCapability(GraphicsDeviceCapability_RAYTRACING))
				{
					barrierdesc.srcStageMask |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
					barrierdesc.dstStageMask |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
					barrierdesc.srcAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
					barrierdesc.dstAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
				}

				if (CheckCapability(GraphicsDeviceCapability_PREDICATION))
				{
					barrierdesc.srcStageMask |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
					barrierdesc.dstStageMask |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
					barrierdesc.srcAccessMask |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
					barrierdesc.dstAccessMask |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
				}

				memoryBarriers.push_back(barrierdesc);
			}
			break;
			case GPUBarrier::Type_IMAGE:
			{
				const TextureDesc& desc = barrier.image.texture->desc;
				auto internal_state = to_internal(barrier.image.texture);

				VkImageMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.image = internal_state->resource;
				barrierdesc.oldLayout = _ConvertImageLayout(barrier.image.layout_before);
				barrierdesc.newLayout = _ConvertImageLayout(barrier.image.layout_after);
				barrierdesc.srcStageMask = _ConvertPipelineStage(barrier.image.layout_before);
				barrierdesc.dstStageMask = _ConvertPipelineStage(barrier.image.layout_after);
				barrierdesc.srcAccessMask = _ParseResourceState(barrier.image.layout_before);
				barrierdesc.dstAccessMask = _ParseResourceState(barrier.image.layout_after);
				if (IsFormatDepthSupport(desc.format))
				{
					barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrierdesc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				if (barrier.image.aspect != nullptr)
				{
					barrierdesc.subresourceRange.aspectMask = _ConvertImageAspect(*barrier.image.aspect);
				}
				if (barrier.image.mip >= 0 || barrier.image.slice >= 0)
				{
					barrierdesc.subresourceRange.baseMipLevel = (uint32_t)std::max(0, barrier.image.mip);
					barrierdesc.subresourceRange.levelCount = 1;
					barrierdesc.subresourceRange.baseArrayLayer = (uint32_t)std::max(0, barrier.image.slice);
					barrierdesc.subresourceRange.layerCount = 1;
				}
				else
				{
					barrierdesc.subresourceRange.baseMipLevel = 0;
					barrierdesc.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
					barrierdesc.subresourceRange.baseArrayLayer = 0;
					barrierdesc.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
				}
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				if (commandlist.queue == QUEUE_COPY)
				{
					// Simplified barrier on copy queue:
					barrierdesc.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					barrierdesc.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					barrierdesc.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
					barrierdesc.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
				}
				else if (commandlist.queue == QUEUE_VIDEO_DECODE)
				{
					// Simplified barrier on video queue:
					barrierdesc.srcStageMask = VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
					barrierdesc.dstStageMask = VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
					barrierdesc.srcAccessMask = VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR | VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
					barrierdesc.dstAccessMask = VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR | VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
				}

				imageBarriers.push_back(barrierdesc);
			}
			break;
			case GPUBarrier::Type_BUFFER:
			{
				const GPUBufferDesc& desc = barrier.buffer.buffer->desc;
				auto internal_state = to_internal(barrier.buffer.buffer);

				VkBufferMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.buffer = internal_state->resource;
				barrierdesc.size = desc.size;
				barrierdesc.offset = 0;
				barrierdesc.srcStageMask = _ConvertPipelineStage(barrier.buffer.state_before);
				barrierdesc.dstStageMask = _ConvertPipelineStage(barrier.buffer.state_after);
				barrierdesc.srcAccessMask = _ParseResourceState(barrier.buffer.state_before);
				barrierdesc.dstAccessMask = _ParseResourceState(barrier.buffer.state_after);
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				if (has_flag(desc.misc_flags, ResourceMiscFlag_RAY_TRACING))
				{
					assert(CheckCapability(GraphicsDeviceCapability_RAYTRACING));
					barrierdesc.srcStageMask |= _ConvertPipelineStage(ResourceState_RAYTRACING_ACCELERATION_STRUCTURE);
					barrierdesc.dstStageMask |= _ConvertPipelineStage(ResourceState_RAYTRACING_ACCELERATION_STRUCTURE);
				}

				if (has_flag(desc.misc_flags, ResourceMiscFlag_PREDICATION))
				{
					assert(CheckCapability(GraphicsDeviceCapability_PREDICATION));
					barrierdesc.srcStageMask |= _ConvertPipelineStage(ResourceState_PREDICATION);
					barrierdesc.dstStageMask |= _ConvertPipelineStage(ResourceState_PREDICATION);
				}

				if (commandlist.queue == QUEUE_COPY)
				{
					// Simplified barrier on copy queue:
					barrierdesc.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					barrierdesc.dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
					barrierdesc.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
					barrierdesc.dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
				}
				else if (commandlist.queue == QUEUE_VIDEO_DECODE)
				{
					// Simplified barrier on video queue:
					barrierdesc.srcStageMask = VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
					barrierdesc.dstStageMask = VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
					barrierdesc.srcAccessMask = VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR | VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
					barrierdesc.dstAccessMask = VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR | VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
				}

				bufferBarriers.push_back(barrierdesc);
			}
			break;
			}
		}

		if (!memoryBarriers.empty() ||
			!bufferBarriers.empty() ||
			!imageBarriers.empty()
			)
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size());
			dependencyInfo.pMemoryBarriers = memoryBarriers.data();
			dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size());
			dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size());
			dependencyInfo.pImageMemoryBarriers = imageBarriers.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);

			memoryBarriers.clear();
			imageBarriers.clear();
			bufferBarriers.clear();
		}
	}

	void VulkanDevice::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto dst_internal = to_internal(dst);

		VkAccelerationStructureBuildGeometryInfoKHR info = dst_internal->buildInfo;
		info.dstAccelerationStructure = dst_internal->resource;
		info.srcAccelerationStructure = VK_NULL_HANDLE;
		info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		info.scratchData.deviceAddress = dst_internal->scratch_address;

		if (src != nullptr && (dst->desc.flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE))
		{
			info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;

			auto src_internal = to_internal(src);
			info.srcAccelerationStructure = src_internal->resource;
		}

		commandlist.accelerationstructure_build_geometries = dst_internal->geometries; // copy!
		commandlist.accelerationstructure_build_ranges.clear();

		info.type = dst_internal->createInfo.type;
		info.geometryCount = (uint32_t)commandlist.accelerationstructure_build_geometries.size();
		commandlist.accelerationstructure_build_ranges.reserve(info.geometryCount);

		switch (dst->desc.type)
		{
		case RaytracingAccelerationStructureDesc::Type_BOTTOMLEVEL:
		{
			size_t i = 0;
			for (auto& x : dst->desc.bottom_level.geometries)
			{
				auto& geometry = commandlist.accelerationstructure_build_geometries[i];

				auto& range = commandlist.accelerationstructure_build_ranges.emplace_back();
				range = {};

				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE)
				{
					geometry.flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
				}
				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
				{
					geometry.flags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
				}

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_TRIANGLES)
				{
					geometry.geometry.triangles.vertexData.deviceAddress = to_internal(&x.triangles.vertex_buffer)->address +
						x.triangles.vertex_byte_offset;

					geometry.geometry.triangles.indexData.deviceAddress = to_internal(&x.triangles.index_buffer)->address +
						x.triangles.index_offset * (x.triangles.index_format == IndexBufferFormat_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.geometry.triangles.transformData.deviceAddress = to_internal(&x.triangles.transform_3x4_buffer)->address;
						range.transformOffset = x.triangles.transform_3x4_buffer_offset;
					}

					range.primitiveCount = x.triangles.index_count / 3;
					range.primitiveOffset = 0;
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_PROCEDURAL_AABBS)
				{
					geometry.geometry.aabbs.data.deviceAddress = to_internal(&x.aabbs.aabb_buffer)->address;

					range.primitiveCount = x.aabbs.count;
					range.primitiveOffset = x.aabbs.offset;
				}

				i++;
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::Type_TOPLEVEL:
		{
			auto& geometry = commandlist.accelerationstructure_build_geometries.back();
			geometry.geometry.instances.data.deviceAddress = to_internal(&dst->desc.top_level.instance_buffer)->address;

			auto& range = commandlist.accelerationstructure_build_ranges.emplace_back();
			range = {};
			range.primitiveCount = dst->desc.top_level.count;
			range.primitiveOffset = dst->desc.top_level.offset;
		}
		break;
		}

		info.pGeometries = commandlist.accelerationstructure_build_geometries.data();

		VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = commandlist.accelerationstructure_build_ranges.data();

		vkCmdBuildAccelerationStructuresKHR(
			commandlist.GetCommandBuffer(),
			1,
			&info,
			&pRangeInfo
		);
	}

	void VulkanDevice::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.prev_pipeline_hash = {};

		if (commandlist.active_rt == nullptr)
		{
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_SET_OR_OFFSET;
		}

		commandlist.active_rt = rtpso;

		BindComputeShader(rtpso->desc.shader_libraries.front().shader, cmd);

		vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, to_internal(rtpso)->pipeline);
	}

	void VulkanDevice::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		predispatch(cmd);

		VkStridedDeviceAddressRegionKHR raygen = {};
		raygen.deviceAddress = desc->ray_generation.buffer ? to_internal(desc->ray_generation.buffer)->address : 0;
		raygen.deviceAddress += desc->ray_generation.offset;
		raygen.size = desc->ray_generation.size;
		raygen.stride = raygen.size; // raygen specifically must be size == stride

		VkStridedDeviceAddressRegionKHR miss = {};
		miss.deviceAddress = desc->miss.buffer ? to_internal(desc->miss.buffer)->address : 0;
		miss.deviceAddress += desc->miss.offset;
		miss.size = desc->miss.size;
		miss.stride = desc->miss.stride;

		VkStridedDeviceAddressRegionKHR hitgroup = {};
		hitgroup.deviceAddress = desc->hit_group.buffer ? to_internal(desc->hit_group.buffer)->address : 0;
		hitgroup.deviceAddress += desc->hit_group.offset;
		hitgroup.size = desc->hit_group.size;
		hitgroup.stride = desc->hit_group.stride;

		VkStridedDeviceAddressRegionKHR callable = {};
		callable.deviceAddress = desc->callable.buffer ? to_internal(desc->callable.buffer)->address : 0;
		callable.deviceAddress += desc->callable.offset;
		callable.size = desc->callable.size;
		callable.stride = desc->callable.stride;

		vkCmdTraceRaysKHR(
			commandlist.GetCommandBuffer(),
			&raygen,
			&miss,
			&hitgroup,
			&callable,
			desc->width,
			desc->height,
			desc->depth
		);
	}

	void VulkanDevice::PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(commandlist.layout.pipeline_layout != VK_NULL_HANDLE); // No pipeline was set!

		vkCmdPushConstants(
			commandlist.GetCommandBuffer(),
			commandlist.layout.pipeline_layout,
			VK_SHADER_STAGE_ALL,
			offset,
			size,
			data
		);
	}

	void VulkanDevice::PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability_PREDICATION))
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			auto internal_state = to_internal(buffer);

			VkConditionalRenderingBeginInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
			if (op == PredicationOp_NOT_EQUAL_ZERO)
			{
				info.flags = VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT;
			}
			info.offset = offset;
			info.buffer = internal_state->resource;
			vkCmdBeginConditionalRenderingEXT(commandlist.GetCommandBuffer(), &info);
		}
	}

	void VulkanDevice::PredicationEnd(CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability_PREDICATION))
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdEndConditionalRenderingEXT(commandlist.GetCommandBuffer());
		}
	}

	void VulkanDevice::ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		if (resource->IsBuffer())
		{
			auto internal_state = to_internal<GPUBuffer>(resource);
			vkCmdFillBuffer(
				commandlist.GetCommandBuffer(),
				internal_state->resource,
				0,
				VK_WHOLE_SIZE,
				value
			);
		}
		else if (resource->IsTexture())
		{
			VkClearColorValue color = {};
			color.uint32[0] = value;
			color.uint32[1] = value;
			color.uint32[2] = value;
			color.uint32[3] = value;

			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseArrayLayer = 0;
			range.baseMipLevel = 0;
			range.layerCount = VK_REMAINING_ARRAY_LAYERS;
			range.levelCount = VK_REMAINING_MIP_LEVELS;

			auto internal_state = to_internal<Texture>(resource);
			vkCmdClearColorImage(
				commandlist.GetCommandBuffer(),
				internal_state->resource,
				VK_IMAGE_LAYOUT_GENERAL, // "ClearUAV" so must be in UNORDERED_ACCESS state, that's a given
				&color,
				1,
				&range
			);
		}

	}

	void VulkanDevice::VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto decoder_internal = to_internal(video_decoder);
		auto stream_internal = to_internal(op->stream);
		auto dpb_internal = to_internal(op->DPB);

		if (video_decoder->desc.profile == VideoProfile_H264)
		{
			const h264::SliceHeader* slice_header = (const h264::SliceHeader*)op->slice_header;
			const h264::PPS* pps = (const h264::PPS*)op->pps;
			const h264::SPS* sps = (const h264::SPS*)op->sps;

			StdVideoDecodeH264PictureInfo std_picture_info_h264 = {};
			std_picture_info_h264.pic_parameter_set_id = slice_header->pic_parameter_set_id;
			std_picture_info_h264.seq_parameter_set_id = pps->seq_parameter_set_id;
			std_picture_info_h264.frame_num = slice_header->frame_num;
			std_picture_info_h264.PicOrderCnt[0] = op->poc[0];
			std_picture_info_h264.PicOrderCnt[1] = op->poc[1];
			std_picture_info_h264.idr_pic_id = slice_header->idr_pic_id;
			std_picture_info_h264.flags.is_intra = op->frame_type == VideoFrameType_Intra ? 1 : 0;
			std_picture_info_h264.flags.is_reference = op->reference_priority > 0 ? 1 : 0;
			std_picture_info_h264.flags.IdrPicFlag = std_picture_info_h264.flags.is_intra ? 1 : 0;
			std_picture_info_h264.flags.field_pic_flag = slice_header->field_pic_flag;
			std_picture_info_h264.flags.bottom_field_flag = slice_header->bottom_field_flag;
			std_picture_info_h264.flags.complementary_field_pair = 0;

			VkVideoReferenceSlotInfoKHR reference_slot_infos[17] = {};
			VkVideoPictureResourceInfoKHR reference_slot_pictures[17] = {};
			VkVideoDecodeH264DpbSlotInfoKHR dpb_slots_h264[17] = {};
			StdVideoDecodeH264ReferenceInfo reference_infos_h264[17] = {};
			for (uint32_t i = 0; i < op->DPB->desc.array_size; ++i)
			{
				VkVideoReferenceSlotInfoKHR& slot = reference_slot_infos[i];
				VkVideoPictureResourceInfoKHR& pic = reference_slot_pictures[i];
				VkVideoDecodeH264DpbSlotInfoKHR& dpb = dpb_slots_h264[i];
				StdVideoDecodeH264ReferenceInfo& ref = reference_infos_h264[i];

				slot.sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
				slot.pPictureResource = &pic;
				slot.slotIndex = i;
				slot.pNext = &dpb;

				pic.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
				pic.codedOffset.x = 0;
				pic.codedOffset.y = 0;
				pic.codedExtent.width = op->DPB->desc.width;
				pic.codedExtent.height = op->DPB->desc.height;
				pic.baseArrayLayer = i;
				pic.imageViewBinding = dpb_internal->video_decode_view;

				dpb.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR;
				dpb.pStdReferenceInfo = &ref;

				ref.flags.bottom_field_flag = 0;
				ref.flags.top_field_flag = 0;
				ref.flags.is_non_existing = 0;
				ref.flags.used_for_long_term_reference = 0;
				ref.FrameNum = op->dpb_framenum[i];
				ref.PicOrderCnt[0] = op->dpb_poc[i];
				ref.PicOrderCnt[1] = op->dpb_poc[i];
			}

			VkVideoReferenceSlotInfoKHR reference_slots[17] = {};
			for (size_t i = 0; i < op->dpb_reference_count; ++i)
			{
				uint32_t ref_slot = op->dpb_reference_slots[i];
				assert(ref_slot != op->current_dpb);
				reference_slots[i] = reference_slot_infos[ref_slot];
			}
			reference_slots[op->dpb_reference_count] = reference_slot_infos[op->current_dpb];
			reference_slots[op->dpb_reference_count].slotIndex = -1;

			VkVideoBeginCodingInfoKHR begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR;
			begin_info.videoSession = decoder_internal->video_session;
			begin_info.videoSessionParameters = decoder_internal->session_parameters;
			begin_info.referenceSlotCount = op->dpb_reference_count + 1; // add in the current reconstructed DPB image
			begin_info.pReferenceSlots = begin_info.referenceSlotCount == 0 ? nullptr : reference_slots;
			vkCmdBeginVideoCodingKHR(commandlist.GetCommandBuffer(), &begin_info);

			if (op->flags & VideoDecodeOperation::FLAG_SESSION_RESET)
			{
				VkVideoCodingControlInfoKHR control_info = {};
				control_info.sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR;
				control_info.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR;
				vkCmdControlVideoCodingKHR(commandlist.GetCommandBuffer(), &control_info);
			}

			VkVideoDecodeInfoKHR decode_info = {};
			decode_info.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR;
			decode_info.srcBuffer = stream_internal->resource;
			decode_info.srcBufferOffset = (VkDeviceSize)op->stream_offset;
			decode_info.srcBufferRange = (VkDeviceSize)align(op->stream_size, VIDEO_DECODE_BITSTREAM_ALIGNMENT);
			if (op->output == nullptr)
			{
				decode_info.dstPictureResource = *reference_slot_infos[op->current_dpb].pPictureResource;
			}
			else
			{
				auto output_internal = to_internal(op->output);
				decode_info.dstPictureResource.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
				decode_info.dstPictureResource.codedOffset.x = 0;
				decode_info.dstPictureResource.codedOffset.y = 0;
				decode_info.dstPictureResource.codedExtent.width = op->DPB->desc.width;
				decode_info.dstPictureResource.codedExtent.height = op->DPB->desc.height;
				decode_info.dstPictureResource.baseArrayLayer = 0;
				decode_info.dstPictureResource.imageViewBinding = output_internal->video_decode_view;
			}
			decode_info.referenceSlotCount = op->dpb_reference_count;
			decode_info.pReferenceSlots = decode_info.referenceSlotCount == 0 ? nullptr : reference_slots;
			decode_info.pSetupReferenceSlot = &reference_slot_infos[op->current_dpb];

			uint32_t slice_offset = 0;

			// https://vulkan.lunarg.com/doc/view/1.3.239.0/windows/1.3-extensions/vkspec.html#_h_264_decoding_parameters
			VkVideoDecodeH264PictureInfoKHR picture_info_h264 = {};
			picture_info_h264.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR;
			picture_info_h264.pStdPictureInfo = &std_picture_info_h264;
			picture_info_h264.sliceCount = 1;
			picture_info_h264.pSliceOffsets = &slice_offset;
			decode_info.pNext = &picture_info_h264;

			vkCmdDecodeVideoKHR(commandlist.GetCommandBuffer(), &decode_info);

			VkVideoEndCodingInfoKHR end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR;
			vkCmdEndVideoCodingKHR(commandlist.GetCommandBuffer(), &end_info);
		}
		else if (video_decoder->desc.profile == VideoProfile_H265)
		{
			assert(0); // TODO
		}
	}

	void VulkanDevice::EventBegin(const char* name, CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		label.pLabelName = name;
		label.color[0] = 0.0f;
		label.color[1] = 0.0f;
		label.color[2] = 0.0f;
		label.color[3] = 1.0f;
		vkCmdBeginDebugUtilsLabelEXT(commandlist.GetCommandBuffer(), &label);
	}

	void VulkanDevice::EventEnd(CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		vkCmdEndDebugUtilsLabelEXT(commandlist.GetCommandBuffer());
	}

	void VulkanDevice::SetMarker(const char* name, CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		VkDebugUtilsLabelEXT label { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		label.pLabelName = name;
		label.color[0] = 0.0f;
		label.color[1] = 0.0f;
		label.color[2] = 0.0f;
		label.color[3] = 1.0f;
		vkCmdInsertDebugUtilsLabelEXT(commandlist.GetCommandBuffer(), &label);
	}
}

#endif // WICKEDENGINE_BUILD_VULKAN
