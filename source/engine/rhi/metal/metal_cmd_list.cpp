#include "rhi/metal/metal_internal.h"
// Metal backend: Command list recording

#ifdef __APPLE__

namespace Protean::Graphics
{
	using namespace MetalInternal;

	void MetalDevice::binder_flush(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		if (commandlist.dirty_root)
		{
			// root CBVs:
			for (uint32_t i = 0; i < arraysize(RootLayout::root_cbvs); ++i)
			{
				if (!commandlist.binding_table.CBV[i].IsValid())
					continue;
				auto internal_state = to_internal(&commandlist.binding_table.CBV[i]);
				commandlist.root.root_cbvs[i] = internal_state->gpu_address + commandlist.binding_table.CBV_offset[i];
				assert(is_aligned(commandlist.root.root_cbvs[i], MTL::GPUAddress(256)));
			}
			
			if (commandlist.dirty_resource)
			{
				commandlist.dirty_resource = false;
				ResourceTable resource_table = {};
				GPUAllocation resource_table_allocation = AllocateGPU(sizeof(ResourceTable), cmd);
				auto resource_table_allocation_internal = to_internal(&resource_table_allocation.buffer);
				commandlist.root.resource_table_ptr = resource_table_allocation_internal->gpu_address + resource_table_allocation.offset;
				
				for (uint32_t i = arraysize(RootLayout::root_cbvs); i < arraysize(DescriptorBindingTable::CBV); ++i)
				{
					if (!commandlist.binding_table.CBV[i].IsValid())
						continue;
					
					auto internal_state = to_internal(&commandlist.binding_table.CBV[i]);
					const uint64_t offset = commandlist.binding_table.CBV_offset[i];
					const MTL::GPUAddress gpu_address = internal_state->gpu_address + offset;
					const uint64_t metadata = commandlist.binding_table.CBV[i].desc.size;
					IRDescriptorTableSetBuffer(&resource_table.cbvs[i - arraysize(RootLayout::root_cbvs)], gpu_address, metadata);
				}
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.SRV); ++i)
				{
					if (!commandlist.binding_table.SRV[i].IsValid())
						continue;
					
					if (commandlist.binding_table.SRV[i].IsBuffer())
					{
						auto internal_state = to_internal<GPUBuffer>(&commandlist.binding_table.SRV[i]);
						const int subresource_index = commandlist.binding_table.SRV_index[i];
						const auto& subresource = subresource_index < 0 ? internal_state->srv : internal_state->subresources_srv[subresource_index];
						resource_table.srvs[i] = subresource.entry;
					}
					else if (commandlist.binding_table.SRV[i].IsTexture())
					{
						auto internal_state = to_internal<Texture>(&commandlist.binding_table.SRV[i]);
						const int subresource_index = commandlist.binding_table.SRV_index[i];
						const auto& subresource = subresource_index < 0 ? internal_state->srv : internal_state->subresources_srv[subresource_index];
						resource_table.srvs[i] = subresource.entry;
					}
					else if (commandlist.binding_table.SRV[i].IsAccelerationStructure())
					{
						auto internal_state = to_internal<RaytracingAccelerationStructure>(&commandlist.binding_table.SRV[i]);
						resource_table.srvs[i] = internal_state->tlas_entry;
					}
				}
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.UAV); ++i)
				{
					if (!commandlist.binding_table.UAV[i].IsValid())
						continue;
					
					if (commandlist.binding_table.UAV[i].IsBuffer())
					{
						auto internal_state = to_internal<GPUBuffer>(&commandlist.binding_table.UAV[i]);
						const int subresource_index = commandlist.binding_table.UAV_index[i];
						const auto& subresource = subresource_index < 0 ? internal_state->uav : internal_state->subresources_uav[subresource_index];
						resource_table.uavs[i] = subresource.entry;
					}
					else if (commandlist.binding_table.UAV[i].IsTexture())
					{
						auto internal_state = to_internal<Texture>(&commandlist.binding_table.UAV[i]);
						const int subresource_index = commandlist.binding_table.UAV_index[i];
						const auto& subresource = subresource_index < 0 ? internal_state->uav : internal_state->subresources_uav[subresource_index];
						resource_table.uavs[i] = subresource.entry;
					}
				}
				
				std::memcpy(resource_table_allocation.data, &resource_table, sizeof(resource_table));
			}
			if (commandlist.dirty_sampler)
			{
				commandlist.dirty_sampler = false;
				SamplerTable sampler_table = {};
				sampler_table.static_samplers = static_sampler_descriptors;
				GPUAllocation sampler_table_allocation = AllocateGPU(sizeof(SamplerTable), cmd);
				auto sampler_table_allocation_internal = to_internal(&sampler_table_allocation.buffer);
				commandlist.root.sampler_table_ptr = sampler_table_allocation_internal->gpu_address + sampler_table_allocation.offset;
				
				for (uint32_t i = 0; i < arraysize(commandlist.binding_table.SAM); ++i)
				{
					if (!commandlist.binding_table.SAM[i].IsValid())
						continue;
					auto internal_state = to_internal(&commandlist.binding_table.SAM[i]);
					sampler_table.samplers[i] = internal_state->entry;
				}
				
				std::memcpy(sampler_table_allocation.data, &sampler_table, sizeof(sampler_table));
			}
			
			auto alloc = AllocateGPU(sizeof(commandlist.root), cmd);
			std::memcpy(alloc.data, &commandlist.root, sizeof(commandlist.root));
			auto alloc_internal = to_internal(&alloc.buffer);
			commandlist.argument_table->setAddress(alloc_internal->gpu_address + alloc.offset, kIRArgumentBufferBindPoint);
		}
		if (commandlist.dirty_vb && commandlist.active_pso != nullptr && commandlist.active_pso->desc.il != nullptr)
		{
			const InputLayout& il = *commandlist.active_pso->desc.il;
			for (size_t i = 0; i < il.elements.size(); ++i)
			{
				auto& element = il.elements[i];
				auto& vb = commandlist.vertex_buffers[element.input_slot];
				if (vb.gpu_address == 0)
					continue;
				commandlist.argument_table->setAddress(vb.gpu_address, vb.stride, kIRVertexBufferBindPoint + i);
			}
		}
		
		// Flushing argument table updates to encoder:
		if (commandlist.dirty_root || commandlist.dirty_vb || commandlist.dirty_drawargs)
		{
			commandlist.dirty_root = false;
			commandlist.dirty_vb = false;
			commandlist.dirty_drawargs = false;
			
			if (commandlist.render_encoder.get() != nullptr)
			{
				commandlist.render_encoder->setArgumentTable(commandlist.argument_table.get(), MTL::RenderStageVertex | MTL::RenderStageObject | MTL::RenderStageMesh | MTL::RenderStageFragment);
			}
			else if (commandlist.compute_encoder.get() != nullptr)
			{
				commandlist.compute_encoder->setArgumentTable(commandlist.argument_table.get());
			}
		}
	}

	void MetalDevice::barrier_flush(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.barriers.empty())
			return;
		MTL4::VisibilityOptions visibility_options = MTL4::VisibilityOptionNone;
		for (auto& x : commandlist.barriers)
		{
			if (x.type == GPUBarrier::Type_ALIASING)
			{
				visibility_options |= MTL4::VisibilityOptionResourceAlias;
			}
		}
		if (commandlist.render_encoder.get() != nullptr)
		{
			commandlist.render_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, visibility_options);
			commandlist.render_encoder->barrierAfterEncoderStages(MTL::StageVertex | MTL::StageObject | MTL::StageMesh, MTL::StageVertex | MTL::StageObject | MTL::StageMesh | MTL::StageFragment, visibility_options);
		}
		else if (commandlist.compute_encoder.get() != nullptr)
		{
			commandlist.compute_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, visibility_options);
			commandlist.compute_encoder->barrierAfterEncoderStages(MTL::StageDispatch | MTL::StageBlit | MTL::StageAccelerationStructure, MTL::StageDispatch | MTL::StageBlit | MTL::StageAccelerationStructure, visibility_options);
		}
		commandlist.barriers.clear();
	}

	void MetalDevice::clear_flush(CommandList cmd)
{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.texture_clears.empty())
			return;
		
		if (commandlist.compute_encoder.get() != nullptr)
		{
			commandlist.compute_encoder->endEncoding();
			commandlist.compute_encoder.reset();
		}
		NS::SharedPtr<NS::AutoreleasePool> autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
		for (auto& x : commandlist.texture_clears)
		{
			const uint64_t width0 = x.first->width();
			const uint64_t height0 = x.first->height();
			const uint64_t slices = x.first->arrayLength();
			const uint64_t mips = x.first->mipmapLevelCount();
			for (uint64_t slice = 0; slice < slices; ++slice)
			{
				for (uint64_t mip = 0; mip < mips; ++mip)
				{
					NS::SharedPtr<MTL::RenderPassColorAttachmentDescriptor> color_attachment_descriptor = NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()->init());
					const uint32_t value = x.second;
					color_attachment_descriptor->setTexture(x.first.get());
					color_attachment_descriptor->setClearColor(MTL::ClearColor::Make((value & 0xFF) / 255.0f, ((value >> 8u) & 0xFF) / 255.0f, ((value >> 16u) & 0xFF) / 255.0f, ((value >> 24u) & 0xFF) / 255.0f));
					color_attachment_descriptor->setLoadAction(MTL::LoadActionClear);
					color_attachment_descriptor->setStoreAction(MTL::StoreActionStore);
					color_attachment_descriptor->setLevel(mip);
					color_attachment_descriptor->setSlice(slice);
					
					CommandList_Metal::TextureClearBatchItem& batchitem = commandlist.texture_clear_batching.emplace_back();
					batchitem.attachment = std::move(color_attachment_descriptor);
					const uint64_t width = std::max(uint64_t(1), width0 >> mip);
					const uint64_t height = std::max(uint64_t(1), height0 >> mip);
					batchitem.resolutionPacked = uint32_t(width & 0xFFFF) | (uint32_t(height & 0xFFFF) << 16u);
				}
			}
		}
		commandlist.texture_clears.clear();
		
		if (commandlist.texture_clear_batching.empty())
			return;
		
		std::sort(commandlist.texture_clear_batching.begin(), commandlist.texture_clear_batching.end(), std::less<CommandList_Metal::TextureClearBatchItem>()); // sort them based on resolution so that more can be batched
		
		while (!commandlist.texture_clear_batching.empty())
		{
			uint32_t currentResolutionPacked = commandlist.texture_clear_batching.back().resolutionPacked;
			uint32_t slot = 0;
			
			NS::SharedPtr<MTL4::RenderPassDescriptor> descriptor = NS::TransferPtr(MTL4::RenderPassDescriptor::alloc()->init());
			
			while (!commandlist.texture_clear_batching.empty() && (slot < 8)) // batch up to 8 slots with renderpass clears
			{
				if (commandlist.texture_clear_batching.back().resolutionPacked != currentResolutionPacked)
				{
					break; // batching must be broken when an attachment arrives that doesn't match the resolution of the current batch
				}
				descriptor->colorAttachments()->setObject(commandlist.texture_clear_batching.back().attachment.get(), slot++);
				commandlist.texture_clear_batching.pop_back();
			}
			
			MTL4::RenderCommandEncoder* encoder = commandlist.commandbuffer->renderCommandEncoder(descriptor.get());
			encoder->setLabel(NS::String::string("ClearUAV", NS::UTF8StringEncoding));
			if (commandlist.texture_clear_batching.empty())
			{
				// Last clear batch will issue sync for all clears
				encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, MTL4::VisibilityOptionNone);
			}
			encoder->endEncoding();
		}
	}

	void MetalDevice::pso_validate(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (!commandlist.dirty_pso)
			return;
		
		assert(commandlist.render_encoder.get() != nullptr); // We must be inside renderpass at this point!
		
		PipelineHash pipeline_hash = commandlist.pipeline_hash;
		const PipelineState* pso = commandlist.active_pso;
		auto internal_state = to_internal(pso);
		
		if (internal_state->render_pipeline.get() == nullptr)
		{
			// Just in time PSO:
			MTL::RenderPipelineState* pipeline = nullptr;
			MTL::DepthStencilState* depth_stencil_state = nullptr;
			auto it = pipelines_global.find(pipeline_hash);
			if (it != pipelines_global.end())
			{
				// Exists in global just in time PSO map:
				pipeline = it->second.pipeline.get();
				depth_stencil_state = it->second.depth_stencil_state.get();
			}
			else
			{
				// Doesn't yet exist in global just in time PSO map, but maybe exists on this thread temporary PSO array:
				for (auto& x : commandlist.pipelines_worker)
				{
					if (pipeline_hash == x.first)
					{
						pipeline = x.second.pipeline.get();
						depth_stencil_state = x.second.depth_stencil_state.get();
						break;
					}
				}
			}
			if (pipeline == nullptr)
			{
				RenderPassInfo renderpass_info = GetRenderPassInfo(cmd);
				PipelineState newPSO;
				bool success = CreatePipelineState(&pso->desc, &newPSO, &renderpass_info);
				assert(success);
				assert(newPSO.IsValid());

				auto internal_new = to_internal(&newPSO);
				assert(internal_new->render_pipeline.get() != nullptr);
				assert(internal_new->depth_stencil_state.get() != nullptr);
				JustInTimePSO just_in_time_pso;
				just_in_time_pso.pipeline = internal_new->render_pipeline;
				just_in_time_pso.depth_stencil_state = internal_new->depth_stencil_state;
				commandlist.pipelines_worker.push_back(std::make_pair(pipeline_hash, std::move(just_in_time_pso)));
				pipeline = internal_new->render_pipeline.get();
				depth_stencil_state = internal_new->depth_stencil_state.get();
			}
			assert(pipeline != nullptr);
			assert(depth_stencil_state != nullptr);
			commandlist.render_encoder->setRenderPipelineState(pipeline);
			commandlist.render_encoder->setDepthStencilState(depth_stencil_state);
		}
		else
		{
			// Precompiled PSO:
			commandlist.render_encoder->setRenderPipelineState(internal_state->render_pipeline.get());
			commandlist.render_encoder->setDepthStencilState(internal_state->depth_stencil_state.get());
		}

		const RasterizerState& rs_desc = pso->desc.rs != nullptr ? *pso->desc.rs : default_rasterizerstate;
		MTL::CullMode cull_mode = {};
		switch (rs_desc.cull_mode)
		{
			case CullMode_BACK:
				cull_mode = MTL::CullModeBack;
				break;
			case CullMode_FRONT:
				cull_mode = MTL::CullModeFront;
				break;
			case CullMode_NONE:
				cull_mode = MTL::CullModeNone;
				break;
		}
		commandlist.render_encoder->setCullMode(cull_mode);
		commandlist.render_encoder->setDepthBias((float)rs_desc.depth_bias, rs_desc.slope_scaled_depth_bias, rs_desc.depth_bias_clamp);
		commandlist.render_encoder->setDepthClipMode(rs_desc.depth_clip_enable ? MTL::DepthClipModeClip : MTL::DepthClipModeClamp);
		MTL::TriangleFillMode fill_mode = {};
		switch (rs_desc.fill_mode)
		{
			case FillMode_SOLID:
				fill_mode = MTL::TriangleFillModeFill;
				break;
			case FillMode_WIREFRAME:
				fill_mode = MTL::TriangleFillModeLines;
				break;
		}
		commandlist.render_encoder->setTriangleFillMode(fill_mode);
		commandlist.render_encoder->setFrontFacingWinding(rs_desc.front_counter_clockwise ? MTL::WindingCounterClockwise : MTL::WindingClockwise);
		
		switch (pso->desc.pt)
		{
			case PrimitiveTopology_TRIANGLELIST:
				commandlist.primitive_type = MTL::PrimitiveTypeTriangle;
				commandlist.ir_primitive_type = IRRuntimePrimitiveTypeTriangle;
				break;
			case PrimitiveTopology_TRIANGLESTRIP:
				commandlist.primitive_type = MTL::PrimitiveTypeTriangleStrip;
				commandlist.ir_primitive_type = IRRuntimePrimitiveTypeTriangleStrip;
				break;
			case PrimitiveTopology_LINELIST:
				commandlist.primitive_type = MTL::PrimitiveTypeLine;
				commandlist.ir_primitive_type = IRRuntimePrimitiveTypeLine;
				break;
			case PrimitiveTopology_LINESTRIP:
				commandlist.primitive_type = MTL::PrimitiveTypeLineStrip;
				commandlist.ir_primitive_type = IRRuntimePrimitiveTypeLineStrip;
				break;
			case PrimitiveTopology_POINTLIST:
				commandlist.primitive_type = MTL::PrimitiveTypePoint;
				commandlist.ir_primitive_type = IRRuntimePrimitiveTypePoint;
				break;
			case PrimitiveTopology_PATCHLIST:
				commandlist.primitive_type = MTL::PrimitiveTypeTriangle;
				commandlist.ir_primitive_type = IRRuntimePrimitiveType3ControlPointPatchlist;
				break;
			default:
				break;
		}
		
		commandlist.dirty_pso = false;
	}

	void MetalDevice::predraw(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.active_cs = nullptr;
		assert(commandlist.render_encoder.get() != nullptr);
		pso_validate(cmd);
		binder_flush(cmd);
		
		if (commandlist.dirty_scissor && commandlist.scissor_count > 0)
		{
			commandlist.dirty_scissor = false;
			MTL::ScissorRect scissors[arraysize(commandlist.scissors)];
			std::memcpy(&scissors, commandlist.scissors, sizeof(commandlist.scissors));
			for (uint32_t i = 0; i < commandlist.scissor_count; ++i)
			{
				MTL::ScissorRect& scissor = scissors[i];
				scissor.x = clamp(scissor.x, NS::UInteger(0), NS::UInteger(commandlist.render_width));
				scissor.y = clamp(scissor.y, NS::UInteger(0), NS::UInteger(commandlist.render_height));
				scissor.width = clamp(scissor.width, NS::UInteger(0), NS::UInteger(commandlist.render_width) - scissor.x);
				scissor.height = clamp(scissor.height, NS::UInteger(0), NS::UInteger(commandlist.render_height) - scissor.y);
			}
			commandlist.render_encoder->setScissorRects(scissors, commandlist.scissor_count);
		}
		if (commandlist.dirty_viewport && commandlist.viewport_count > 0)
		{
			commandlist.dirty_viewport = false;
			commandlist.render_encoder->setViewports(commandlist.viewports, commandlist.viewport_count);
		}
	}

	void MetalDevice::predispatch(CommandList cmd)
	{
		clear_flush(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.compute_encoder.get() == nullptr)
		{
			NS::SharedPtr<NS::AutoreleasePool> autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
			commandlist.compute_encoder = NS::TransferPtr(commandlist.commandbuffer->computeCommandEncoder()->retain());
		}
		commandlist.active_pso = nullptr;
		commandlist.dirty_vb = true;
		commandlist.dirty_root = true;
		commandlist.dirty_sampler = true;
		commandlist.dirty_resource = true;
		commandlist.dirty_scissor = true;
		commandlist.dirty_viewport = true;
		commandlist.dirty_cs = true;
		
		if (commandlist.dirty_cs && commandlist.active_cs != nullptr)
		{
			commandlist.dirty_cs = false;
			auto internal_state = to_internal(commandlist.active_cs);
			commandlist.compute_encoder->setComputePipelineState(internal_state->compute_pipeline.get());
		}
		
		binder_flush(cmd);
		barrier_flush(cmd);
	}

	void MetalDevice::precopy(CommandList cmd)
	{
		clear_flush(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.compute_encoder.get() == nullptr)
		{
			NS::SharedPtr<NS::AutoreleasePool> autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
			commandlist.compute_encoder = NS::TransferPtr(commandlist.commandbuffer->computeCommandEncoder()->retain());
		}
		
		barrier_flush(cmd);
	}

	void MetalDevice::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		CommandList_Metal& commandlist_wait_for = GetCommandList(wait_for);
		assert(commandlist_wait_for.id < commandlist.id); // can't wait for future command list!
		Semaphore sema = new_semaphore();
		commandlist.waits.push_back(sema);
		commandlist_wait_for.signals.push_back(sema);
	}

	void MetalDevice::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		clear_flush(cmd);
		NS::SharedPtr<NS::AutoreleasePool> autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.compute_encoder.get() != nullptr)
		{
			commandlist.compute_encoder->endEncoding();
			commandlist.compute_encoder.reset();
			commandlist.compute_encoder = nullptr;
		}
		
		auto internal_state = to_internal(swapchain);
		
		CA::MetalDrawable* drawable = internal_state->layer->nextDrawable();
		while (drawable == nullptr)
		{
			drawable = internal_state->layer->nextDrawable();
		}
		internal_state->current_drawable = NS::TransferPtr(drawable->retain());
		internal_state->current_texture = NS::TransferPtr(drawable->texture()->retain());
		
		commandlist.presents.push_back(internal_state->current_drawable);
		
		NS::SharedPtr<MTL4::RenderPassDescriptor> descriptor = NS::TransferPtr(MTL4::RenderPassDescriptor::alloc()->init());
		NS::SharedPtr<MTL::RenderPassColorAttachmentDescriptor> color_attachment_descriptor = NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()->init());
		
		CGSize size = internal_state->layer->drawableSize();
		descriptor->setRenderTargetWidth(size.width);
		descriptor->setRenderTargetHeight(size.height);
		descriptor->setDefaultRasterSampleCount(1);
		
		color_attachment_descriptor->setTexture(internal_state->current_drawable->texture());
		color_attachment_descriptor->setClearColor(MTL::ClearColor::Make(swapchain->desc.clear_color[0], swapchain->desc.clear_color[1], swapchain->desc.clear_color[2], swapchain->desc.clear_color[3]));
		color_attachment_descriptor->setLoadAction(MTL::LoadActionClear);
		color_attachment_descriptor->setStoreAction(MTL::StoreActionStore);
		descriptor->colorAttachments()->setObject(color_attachment_descriptor.get(), 0);
		
		commandlist.render_encoder = NS::TransferPtr(commandlist.commandbuffer->renderCommandEncoder(descriptor.get())->retain());
		commandlist.dirty_vb = true;
		commandlist.dirty_root = true;
		commandlist.dirty_sampler = true;
		commandlist.dirty_resource = true;
		commandlist.dirty_scissor = true;
		commandlist.dirty_viewport = true;
		commandlist.dirty_pso = true;
		
		commandlist.render_width = size.width;
		commandlist.render_height = size.height;
		
		commandlist.renderpass_info = RenderPassInfo::from(swapchain->desc);
		
		barrier_flush(cmd);
	}

	void MetalDevice::RenderPassBegin(const RenderPassImage* images, uint32_t image_count, const GPUQueryHeap* occlusionqueries, CommandList cmd, RenderPassFlags flags)
	{
		clear_flush(cmd);
		NS::SharedPtr<NS::AutoreleasePool> autorelease_pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.compute_encoder.get() != nullptr)
		{
			commandlist.compute_encoder->endEncoding();
			commandlist.compute_encoder.reset();
			commandlist.compute_encoder = nullptr;
		}
		
		NS::SharedPtr<MTL4::RenderPassDescriptor> descriptor = NS::TransferPtr(MTL4::RenderPassDescriptor::alloc()->init());
		NS::SharedPtr<MTL::RenderPassColorAttachmentDescriptor> color_attachment_descriptors[8];
		NS::SharedPtr<MTL::RenderPassDepthAttachmentDescriptor> depth_attachment_descriptor;
		NS::SharedPtr<MTL::RenderPassStencilAttachmentDescriptor> stencil_attachment_descriptor;
		
		if (image_count > 0)
		{
			descriptor->setRenderTargetWidth(images[0].texture->desc.width);
			descriptor->setRenderTargetHeight(images[0].texture->desc.height);
			descriptor->setRenderTargetArrayLength(images[0].texture->desc.array_size);
			descriptor->setDefaultRasterSampleCount(images[0].texture->desc.sample_count);
			commandlist.render_width = images[0].texture->desc.width;
			commandlist.render_height = images[0].texture->desc.height;
		}
		else
		{
			descriptor->setRenderTargetWidth(commandlist.viewports[0].width);
			descriptor->setRenderTargetHeight(commandlist.viewports[0].height);
			descriptor->setDefaultRasterSampleCount(1);
		}
		
		uint32_t color_attachment_index = 0;
		uint32_t resolve_index = 0;
		for (uint32_t i = 0; i < image_count; ++i)
		{
			const RenderPassImage& image = images[i];
			auto internal_state = to_internal(image.texture);
			
			MTL::LoadAction load_action;
			switch (image.loadop) {
				case RenderPassImage::LoadOp_DONTCARE:
					load_action = MTL::LoadActionDontCare;
					break;
				case RenderPassImage::LoadOp_LOAD:
					load_action = MTL::LoadActionLoad;
					break;
				case RenderPassImage::LoadOp_CLEAR:
					load_action = MTL::LoadActionClear;
					break;
				default:
					break;
			}
			MTL::StoreAction store_action;
			switch (image.storeop) {
			  case RenderPassImage::StoreOp_DONTCARE:
				  store_action = MTL::StoreActionDontCare;
				  break;
			  case RenderPassImage::StoreOp_STORE:
				  store_action = MTL::StoreActionStore;
				  break;
			  default:
				  break;
			}
			
			switch (image.type) {
				case RenderPassImage::Type_RENDERTARGET:
				{
					Texture_Metal::Subresource& subresource = image.subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[image.subresource];
					auto& color_attachment_descriptor = color_attachment_descriptors[color_attachment_index];
					color_attachment_descriptor = NS::TransferPtr(MTL::RenderPassColorAttachmentDescriptor::alloc()->init());
					color_attachment_descriptor->init();
					color_attachment_descriptor->setTexture(internal_state->texture.get());
					color_attachment_descriptor->setLevel(subresource.firstMip);
					color_attachment_descriptor->setSlice(subresource.firstSlice);
					color_attachment_descriptor->setClearColor(MTL::ClearColor::Make(image.texture->desc.clear.color[0], image.texture->desc.clear.color[1], image.texture->desc.clear.color[2], image.texture->desc.clear.color[3]));
					color_attachment_descriptor->setLoadAction(load_action);
					color_attachment_descriptor->setStoreAction(store_action);
					color_attachment_descriptor->setResolveTexture(nullptr);
					color_attachment_descriptor->setResolveLevel(0);
					color_attachment_descriptor->setResolveSlice(0);
					descriptor->colorAttachments()->setObject(color_attachment_descriptor.get(), color_attachment_index);
					color_attachment_index++;
					break;
				}
				case RenderPassImage::Type_DEPTH_STENCIL:
				{
					Texture_Metal::Subresource& subresource = image.subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[image.subresource];
					depth_attachment_descriptor = NS::TransferPtr(MTL::RenderPassDepthAttachmentDescriptor::alloc()->init());
					depth_attachment_descriptor->setTexture(internal_state->texture.get());
					depth_attachment_descriptor->setLevel(subresource.firstMip);
					depth_attachment_descriptor->setSlice(subresource.firstSlice);
					depth_attachment_descriptor->setClearDepth(image.texture->desc.clear.depth_stencil.depth);
					depth_attachment_descriptor->setLoadAction(load_action);
					depth_attachment_descriptor->setStoreAction(store_action);
					depth_attachment_descriptor->setResolveTexture(nullptr);
					descriptor->setDepthAttachment(depth_attachment_descriptor.get());
					if (IsFormatStencilSupport(image.texture->desc.format))
					{
						stencil_attachment_descriptor = NS::TransferPtr(MTL::RenderPassStencilAttachmentDescriptor::alloc()->init());
						stencil_attachment_descriptor->setTexture(internal_state->texture.get());
						stencil_attachment_descriptor->setClearStencil(image.texture->desc.clear.depth_stencil.stencil);
						stencil_attachment_descriptor->setLoadAction(load_action);
						stencil_attachment_descriptor->setStoreAction(store_action);
						descriptor->setStencilAttachment(stencil_attachment_descriptor.get());
					}
					break;
				}
				case RenderPassImage::Type_RESOLVE:
				{
					Texture_Metal::Subresource& subresource = image.subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[image.subresource];
					auto& color_attachment_descriptor = color_attachment_descriptors[resolve_index];
					color_attachment_descriptor->setResolveTexture(internal_state->texture.get());
					color_attachment_descriptor->setResolveLevel(subresource.firstMip);
					color_attachment_descriptor->setResolveSlice(subresource.firstSlice);
					color_attachment_descriptor->setStoreAction(color_attachment_descriptor->storeAction() == MTL::StoreActionStore ? MTL::StoreActionStoreAndMultisampleResolve : MTL::StoreActionMultisampleResolve);
					descriptor->colorAttachments()->setObject(color_attachment_descriptor.get(), resolve_index);
					resolve_index++;
					break;
				}
				case RenderPassImage::Type_RESOLVE_DEPTH:
				{
					Texture_Metal::Subresource& subresource = image.subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[image.subresource];
					depth_attachment_descriptor->setResolveTexture(internal_state->texture.get());
					depth_attachment_descriptor->setResolveLevel(subresource.firstMip);
					depth_attachment_descriptor->setResolveSlice(subresource.firstSlice);
					depth_attachment_descriptor->setStoreAction(depth_attachment_descriptor->storeAction() == MTL::StoreActionStore ? MTL::StoreActionStoreAndMultisampleResolve : MTL::StoreActionMultisampleResolve);
					descriptor->setDepthAttachment(depth_attachment_descriptor.get());
					break;
				}
				default:
					assert(0);
					break;
			}
		}
		
		if (occlusionqueries != nullptr && occlusionqueries->IsValid())
		{
			auto occlusionquery_internal = to_internal(occlusionqueries);
			descriptor->setVisibilityResultBuffer(occlusionquery_internal->buffer.get());
			descriptor->setVisibilityResultType(MTL::VisibilityResultTypeReset);
		}
		
		MTL4::RenderEncoderOptions options = MTL4::RenderEncoderOptionNone;
		if (has_flag(flags, RenderPassFlags_SUSPENDING))
		{
			options |= MTL4::RenderEncoderOptionSuspending;
		}
		if (has_flag(flags, RenderPassFlags_RESUMING))
		{
			options |= MTL4::RenderEncoderOptionResuming;
		}
		commandlist.render_encoder = NS::TransferPtr(commandlist.commandbuffer->renderCommandEncoder(descriptor.get(), options)->retain());
		if (occlusionqueries != nullptr && occlusionqueries->IsValid())
		{
			commandlist.render_encoder->setVisibilityResultMode(MTL::VisibilityResultModeDisabled, 0);
		}
		commandlist.dirty_vb = true;
		commandlist.dirty_root = true;
		commandlist.dirty_sampler = true;
		commandlist.dirty_resource = true;
		commandlist.dirty_scissor = true;
		commandlist.dirty_viewport = true;
		commandlist.dirty_pso = true;
		
		commandlist.renderpass_info = RenderPassInfo::from(images, image_count);
		
		barrier_flush(cmd);
	}

	void MetalDevice::RenderPassEnd(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(commandlist.render_encoder.get() != nullptr);
		
		commandlist.render_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, MTL4::VisibilityOptionNone);
		commandlist.render_encoder->endEncoding();
		commandlist.render_encoder.reset();
		commandlist.dirty_pso = true;
		
		commandlist.render_width = 0;
		commandlist.render_height = 0;

		commandlist.renderpass_info = {};
		commandlist.render_encoder = nullptr;
	}

	void MetalDevice::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd)
	{
		assert(rects != nullptr);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		for (uint32_t i = 0; i < numRects; ++i)
		{
			commandlist.scissors[i].x = uint32_t(rects[i].left);
			commandlist.scissors[i].y = uint32_t(rects[i].top);
			commandlist.scissors[i].width = uint32_t(rects[i].right - rects[i].left);
			commandlist.scissors[i].height = uint32_t(rects[i].bottom - rects[i].top);
		}
		commandlist.scissor_count = numRects;
		commandlist.dirty_scissor = true;
	}

	void MetalDevice::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(pViewports != nullptr);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			commandlist.viewports[i].originX = pViewports[i].top_left_x;
			commandlist.viewports[i].originY = pViewports[i].top_left_y;
			commandlist.viewports[i].width = pViewports[i].width;
			commandlist.viewports[i].height = pViewports[i].height;
			commandlist.viewports[i].znear = pViewports[i].min_depth;
			commandlist.viewports[i].zfar = pViewports[i].max_depth;
		}
		commandlist.viewport_count = NumViewports;
		commandlist.dirty_viewport = true;
	}

	void MetalDevice::BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SRV_COUNT);
		if (commandlist.binding_table.SRV[slot].internal_state == resource->internal_state && commandlist.binding_table.SRV_index[slot] == subresource)
			return;
		commandlist.dirty_root = true;
		commandlist.dirty_resource = true;
		commandlist.binding_table.SRV[slot] = *resource;
		commandlist.binding_table.SRV_index[slot] = subresource;
	}

	void MetalDevice::BindResources(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(resources[i], slot + i, cmd, -1);
			}
		}
	}

	void MetalDevice::BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_UAV_COUNT);
		if (commandlist.binding_table.UAV[slot].internal_state == resource->internal_state && commandlist.binding_table.UAV_index[slot] == subresource)
			return;
		commandlist.dirty_root = true;
		commandlist.dirty_resource = true;
		commandlist.binding_table.UAV[slot] = *resource;
		commandlist.binding_table.UAV_index[slot] = subresource;
	}

	void MetalDevice::BindUAVs(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(resources[i], slot + i, cmd, -1);
			}
		}
	}

	void MetalDevice::BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SAMPLER_COUNT);
		if (commandlist.binding_table.SAM[slot].internal_state == sampler->internal_state)
			return;
		commandlist.dirty_root = true;
		commandlist.dirty_sampler = true;
		commandlist.binding_table.SAM[slot] = *sampler;
	}

	void MetalDevice::BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_CBV_COUNT);
		if (commandlist.binding_table.CBV[slot].internal_state == buffer->internal_state && commandlist.binding_table.CBV_offset[slot] == offset)
			return;
		commandlist.dirty_root = true;
		if (slot >= arraysize(RootLayout::root_cbvs))
		{
			commandlist.dirty_resource = true;
		}
		commandlist.binding_table.CBV[slot] = *buffer;
		commandlist.binding_table.CBV_offset[slot] = offset;
	}

	void MetalDevice::BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.dirty_vb = true;
		for (uint32_t i = 0; i < count; ++i)
		{
			auto& vb = commandlist.vertex_buffers[slot + i];
			if (vertexBuffers[i] == nullptr || !vertexBuffers[i]->IsValid())
				continue;
			auto internal_state = to_internal(vertexBuffers[i]);
			const uint64_t offset = offsets == nullptr ? 0 : offsets[i];
			vb.gpu_address = internal_state->gpu_address + offset;
			vb.stride = strides[i];
		}
	}

	void MetalDevice::BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd)
	{
		if (indexBuffer == nullptr || !indexBuffer->IsValid())
			return;
		auto internal_state = to_internal(indexBuffer);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.index_buffer.bufferAddress = internal_state->gpu_address + offset;
		commandlist.index_buffer.length = internal_state->buffer->length();
		commandlist.index_type = format == IndexBufferFormat_UINT32 ? MTL::IndexTypeUInt32 : MTL::IndexTypeUInt16;
	}

	void MetalDevice::BindStencilRef(uint32_t value, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->setStencilReferenceValue(value);
	}

	void MetalDevice::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->setBlendColor(r, g, b, a);
	}

	void MetalDevice::BindShadingRate(ShadingRate rate, CommandList cmd)
	{
	}

	void MetalDevice::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(pso);
		
		if (internal_state->render_pipeline.get() == nullptr)
		{
			// Just in time pso:
			PipelineHash pipeline_hash;
			pipeline_hash.pso = pso;
			pipeline_hash.renderpass_hash = commandlist.renderpass_info.get_hash();
			if (commandlist.pipeline_hash != pipeline_hash)
			{
				commandlist.dirty_pso = true;
				commandlist.pipeline_hash = pipeline_hash;
			}
		}
		else
		{
			// Precompiled pso:
			if (commandlist.active_pso != pso)
			{
				commandlist.active_pso = pso;
				commandlist.dirty_pso = true;
				commandlist.pipeline_hash = {};
			}
		}
		commandlist.active_pso = pso;
		commandlist.drawargs_required = internal_state->needs_draw_params;
		commandlist.numthreads_as = internal_state->numthreads_as;
		commandlist.numthreads_ms = internal_state->numthreads_ms;
		commandlist.gs_desc = internal_state->gs_desc;
	}

	void MetalDevice::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.dirty_cs = commandlist.active_cs != cs;
		commandlist.active_cs = cs;
	}

	void MetalDevice::BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->setDepthTestBounds(min_bounds, max_bounds);
	}

	void MetalDevice::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		if (commandlist.gs_desc.basePipelineDescriptor != nullptr)
		{
			// IRRuntimeDrawPrimitivesGeometryEmulation Metal4 port:
			IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSEmulation(commandlist.ir_primitive_type,
																				  (indextype_t)-1,
																				  commandlist.gs_desc.pipelineConfig.gsVertexSizeInBytes,
																				  commandlist.gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
																				  1);
			drawInfo.indexType = kIRNonIndexedDraw;
			
			mtlsize_t objectThreadgroupCount = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(vertexCount,
																												  drawInfo.objectThreadgroupVertexStride,
																												  commandlist.ir_primitive_type,
																												  1);
			
			uint32_t objectThreadgroupSize,meshThreadgroupSize;
			IRRuntimeCalculateThreadgroupSizeForGeometry(commandlist.ir_primitive_type,
														 commandlist.gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
														 drawInfo.objectThreadgroupVertexStride,
														 &objectThreadgroupSize,
														 &meshThreadgroupSize);
			
			IRRuntimeDrawArgument da = {
				.vertexCountPerInstance = vertexCount,
				.instanceCount = 1,
				.startVertexLocation = startVertexLocation,
				.startInstanceLocation = 0
			};
			IRRuntimeDrawParams dp = { .draw = da };
			
			auto alloc = AllocateGPU(sizeof(drawInfo), cmd);
			std::memcpy(alloc.data, &drawInfo, sizeof(drawInfo));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
			alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			
			predraw(cmd);
			commandlist.render_encoder->drawMeshThreadgroups(objectThreadgroupCount, MTL::Size::Make(objectThreadgroupSize, 1, 1), MTL::Size::Make(meshThreadgroupSize, 1, 1));
			return;
		}
		else if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			IRRuntimeDrawArgument da = {
				.vertexCountPerInstance = vertexCount,
				.instanceCount = 1,
				.startVertexLocation = startVertexLocation,
				.startInstanceLocation = 0
			};
			IRRuntimeDrawParams dp = { .draw = da };
			auto alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			alloc = AllocateGPU(sizeof(uint16_t), cmd);
			std::memcpy(alloc.data, &kIRNonIndexedDraw, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawPrimitives(commandlist.primitive_type, startVertexLocation, vertexCount);
	}

	void MetalDevice::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		const uint64_t index_stride = commandlist.index_type == MTL::IndexTypeUInt32 ? sizeof(uint32_t) : sizeof(uint16_t);
		const uint64_t indexBufferOffset = startIndexLocation * index_stride;
		
		if (commandlist.gs_desc.basePipelineDescriptor != nullptr)
		{
			// IRRuntimeDrawIndexedPrimitivesGeometryEmulation Metal4 port:
			IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSEmulation(commandlist.ir_primitive_type,
																				  commandlist.index_type,
																				  commandlist.gs_desc.pipelineConfig.gsVertexSizeInBytes,
																				  commandlist.gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
																				  1);
			
			mtlsize_t objectThreadgroupCount = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(indexCount,
																												  drawInfo.objectThreadgroupVertexStride,
																												  commandlist.ir_primitive_type,
																												  1);
			
			uint32_t objectThreadgroupSize,meshThreadgroupSize;
			IRRuntimeCalculateThreadgroupSizeForGeometry(commandlist.ir_primitive_type,
														 commandlist.gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
														 drawInfo.objectThreadgroupVertexStride,
														 &objectThreadgroupSize,
														 &meshThreadgroupSize);
			
			IRRuntimeDrawIndexedArgument da = {
				.indexCountPerInstance = indexCount,
				.instanceCount = 1,
				.startIndexLocation = startIndexLocation,
				.baseVertexLocation = baseVertexLocation,
				.startInstanceLocation = 0
			};
			IRRuntimeDrawParams dp = { .drawIndexed = da };
			
			drawInfo.indexBuffer = commandlist.index_buffer.bufferAddress + indexBufferOffset;
			
			auto alloc = AllocateGPU(sizeof(drawInfo), cmd);
			std::memcpy(alloc.data, &drawInfo, sizeof(drawInfo));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
			alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			
			predraw(cmd);
			commandlist.render_encoder->drawMeshThreadgroups(objectThreadgroupCount, MTL::Size::Make(objectThreadgroupSize, 1, 1), MTL::Size::Make(meshThreadgroupSize, 1, 1));
			return;
		}
		else if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			IRRuntimeDrawIndexedArgument da = {
				.indexCountPerInstance = indexCount,
				.instanceCount = 1,
				.startIndexLocation = startIndexLocation,
				.baseVertexLocation = baseVertexLocation,
				.startInstanceLocation = 0
			};
			IRRuntimeDrawParams dp = { .drawIndexed = da };
			auto alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			alloc = AllocateGPU(sizeof(uint16_t), cmd);
			uint16_t irindextype = IRMetalIndexToIRIndex(commandlist.index_type);
			std::memcpy(alloc.data, &irindextype, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawIndexedPrimitives(commandlist.primitive_type, indexCount, commandlist.index_type, commandlist.index_buffer.bufferAddress + indexBufferOffset, commandlist.index_buffer.length);
	}

	void MetalDevice::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		if (commandlist.gs_desc.basePipelineDescriptor != nullptr)
		{
			// IRRuntimeDrawPrimitivesGeometryEmulation Metal4 port:
			IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSEmulation(commandlist.ir_primitive_type,
																				  (indextype_t)-1,
																				  commandlist.gs_desc.pipelineConfig.gsVertexSizeInBytes,
																				  commandlist.gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
																				  instanceCount);
			drawInfo.indexType = kIRNonIndexedDraw;
			
			mtlsize_t objectThreadgroupCount = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(vertexCount,
																												  drawInfo.objectThreadgroupVertexStride,
																												  commandlist.ir_primitive_type,
																												  instanceCount);
			
			uint32_t objectThreadgroupSize,meshThreadgroupSize;
			IRRuntimeCalculateThreadgroupSizeForGeometry(commandlist.ir_primitive_type,
														 commandlist.gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
														 drawInfo.objectThreadgroupVertexStride,
														 &objectThreadgroupSize,
														 &meshThreadgroupSize);
			
			IRRuntimeDrawArgument da = {
				.vertexCountPerInstance = vertexCount,
				.instanceCount = instanceCount,
				.startVertexLocation = startVertexLocation,
				.startInstanceLocation = startInstanceLocation
			};
			IRRuntimeDrawParams dp = { .draw = da };
			
			auto alloc = AllocateGPU(sizeof(drawInfo), cmd);
			std::memcpy(alloc.data, &drawInfo, sizeof(drawInfo));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
			alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			
			predraw(cmd);
			commandlist.render_encoder->drawMeshThreadgroups(objectThreadgroupCount, MTL::Size::Make(objectThreadgroupSize, 1, 1), MTL::Size::Make(meshThreadgroupSize, 1, 1));
			return;
		}
		else if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			IRRuntimeDrawArgument da = {
				.vertexCountPerInstance = vertexCount,
				.instanceCount = instanceCount,
				.startVertexLocation = startVertexLocation,
				.startInstanceLocation = startInstanceLocation
			};
			IRRuntimeDrawParams dp = { .draw = da };
			auto alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			alloc = AllocateGPU(sizeof(uint16_t), cmd);
			std::memcpy(alloc.data, &kIRNonIndexedDraw, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawPrimitives(commandlist.primitive_type, startVertexLocation, vertexCount, instanceCount, startInstanceLocation);
	}

	void MetalDevice::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		const uint64_t index_stride = commandlist.index_type == MTL::IndexTypeUInt32 ? sizeof(uint32_t) : sizeof(uint16_t);
		const uint64_t indexBufferOffset = startIndexLocation * index_stride;
		
		if (commandlist.gs_desc.basePipelineDescriptor != nullptr)
		{
			// IRRuntimeDrawIndexedPrimitivesGeometryEmulation Metal4 port:
			IRRuntimeDrawInfo drawInfo = IRRuntimeCalculateDrawInfoForGSEmulation(commandlist.ir_primitive_type,
																				  commandlist.index_type,
																				  commandlist.gs_desc.pipelineConfig.gsVertexSizeInBytes,
																				  commandlist.gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
																				  instanceCount);
			
			mtlsize_t objectThreadgroupCount = IRRuntimeCalculateObjectTgCountForTessellationAndGeometryEmulation(indexCount,
																												  drawInfo.objectThreadgroupVertexStride,
																												  commandlist.ir_primitive_type,
																												  instanceCount);
			
			uint32_t objectThreadgroupSize,meshThreadgroupSize;
			IRRuntimeCalculateThreadgroupSizeForGeometry(commandlist.ir_primitive_type,
														 commandlist.gs_desc.pipelineConfig.gsMaxInputPrimitivesPerMeshThreadgroup,
														 drawInfo.objectThreadgroupVertexStride,
														 &objectThreadgroupSize,
														 &meshThreadgroupSize);
			
			IRRuntimeDrawIndexedArgument da = {
				.indexCountPerInstance = indexCount,
				.instanceCount = 1,
				.startIndexLocation = startIndexLocation,
				.baseVertexLocation = baseVertexLocation,
				.startInstanceLocation = startInstanceLocation
			};
			IRRuntimeDrawParams dp = { .drawIndexed = da };
			
			drawInfo.indexBuffer = commandlist.index_buffer.bufferAddress + indexBufferOffset;
			
			auto alloc = AllocateGPU(sizeof(drawInfo), cmd);
			std::memcpy(alloc.data, &drawInfo, sizeof(drawInfo));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
			alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			predraw(cmd);
			commandlist.render_encoder->drawMeshThreadgroups(objectThreadgroupCount, MTL::Size::Make(objectThreadgroupSize, 1, 1), MTL::Size::Make(meshThreadgroupSize, 1, 1));
			return;
		}
		else if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			IRRuntimeDrawIndexedArgument da = {
				.indexCountPerInstance = indexCount,
				.instanceCount = instanceCount,
				.startIndexLocation = startIndexLocation,
				.baseVertexLocation = baseVertexLocation,
				.startInstanceLocation = startInstanceLocation
			};
			IRRuntimeDrawParams dp = { .drawIndexed = da };
			auto alloc = AllocateGPU(sizeof(dp), cmd);
			std::memcpy(alloc.data, &dp, sizeof(dp));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferDrawArgumentsBindPoint);
			alloc = AllocateGPU(sizeof(uint16_t), cmd);
			uint16_t irindextype = IRMetalIndexToIRIndex(commandlist.index_type);
			std::memcpy(alloc.data, &irindextype, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawIndexedPrimitives(commandlist.primitive_type, indexCount, commandlist.index_type, commandlist.index_buffer.bufferAddress + indexBufferOffset, commandlist.index_buffer.length, instanceCount, baseVertexLocation, startInstanceLocation);
	}

	void MetalDevice::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		assert(commandlist.gs_desc.basePipelineDescriptor == nullptr); // indirect geometry shader emulation not supported because instance count is not available to compute emulation info
		
		if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			commandlist.argument_table->setAddress(internal_state->gpu_address + args_offset, kIRArgumentBufferDrawArgumentsBindPoint);
			auto alloc = AllocateGPU(sizeof(uint16_t), cmd);
			std::memcpy(alloc.data, &kIRNonIndexedDraw, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawPrimitives(commandlist.primitive_type, internal_state->gpu_address + args_offset);
	}

	void MetalDevice::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		
		assert(commandlist.gs_desc.basePipelineDescriptor == nullptr); // indirect geometry shader emulation not supported because instance count is not available to compute emulation info
		
		if (commandlist.drawargs_required)
		{
			commandlist.dirty_drawargs = true;
			commandlist.argument_table->setAddress(internal_state->gpu_address + args_offset, kIRArgumentBufferDrawArgumentsBindPoint);
			auto alloc = AllocateGPU(sizeof(uint16_t), cmd);
			uint16_t irindextype = IRMetalIndexToIRIndex(commandlist.index_type);
			std::memcpy(alloc.data, &irindextype, sizeof(uint16_t));
			commandlist.argument_table->setAddress(to_internal(&alloc.buffer)->gpu_address + alloc.offset, kIRArgumentBufferUniformsBindPoint);
		}
		
		predraw(cmd);
		commandlist.render_encoder->drawIndexedPrimitives(commandlist.primitive_type, commandlist.index_type, commandlist.index_buffer.bufferAddress, commandlist.index_buffer.length, internal_state->gpu_address + args_offset);
	}

	void MetalDevice::DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		// TODO
	}

	void MetalDevice::DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		// TODO
	}

	void MetalDevice::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto cs_internal = to_internal(commandlist.active_cs);
		commandlist.compute_encoder->dispatchThreadgroups({threadGroupCountX, threadGroupCountY, threadGroupCountZ}, cs_internal->additional_data.numthreads);
	}

	void MetalDevice::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto cs_internal = to_internal(commandlist.active_cs);
		auto internal_state = to_internal(args);
		commandlist.compute_encoder->dispatchThreadgroups(internal_state->gpu_address + args_offset, cs_internal->additional_data.numthreads);
	}

	void MetalDevice::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->drawMeshThreadgroups({threadGroupCountX, threadGroupCountY, threadGroupCountZ}, commandlist.numthreads_as, commandlist.numthreads_ms);
	}

	void MetalDevice::DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		commandlist.render_encoder->drawMeshThreadgroups(internal_state->gpu_address + args_offset, commandlist.numthreads_as, commandlist.numthreads_ms);
	}

	void MetalDevice::DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		// TODO
	}

	void MetalDevice::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (pDst->IsTexture() && pSrc->IsTexture())
		{
			const Texture& srctex = *(Texture*)pSrc;
			const Texture& dsttex = *(Texture*)pDst;
			auto src_internal = to_internal<Texture>(pSrc);
			auto dst_internal = to_internal<Texture>(pDst);
			if (src_internal->texture.get() != nullptr && dst_internal->texture.get() != nullptr)
			{
				// Normal texture->texture copy
				commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), dst_internal->texture.get());
				if (src_internal->texture_chroma.get() != nullptr && dst_internal->texture_chroma.get() != nullptr)
				{
					commandlist.compute_encoder->copyFromTexture(src_internal->texture_chroma.get(), dst_internal->texture_chroma.get());
				}
			}
			else if(src_internal->texture.get() != nullptr && dst_internal->buffer.get() != nullptr)
			{
				// Texture->linear copy:
				const uint64_t data_begin = (uint64_t)dsttex.mapped_subresources[0].data_ptr;
				const uint32_t planes = GetFormatPlaneCount(srctex.desc.format); // src plane count!
				for (uint32_t plane = 0; plane < planes; ++plane)
				{
					for (uint32_t slice = 0; slice < srctex.desc.array_size; ++slice)
					{
						for (uint32_t mip = 0; mip < srctex.desc.mip_levels; ++mip)
						{
							const uint32_t mip_width = std::max(1u, srctex.desc.width >> mip);
							const uint32_t mip_height = std::max(1u, srctex.desc.height >> mip);
							const uint32_t mip_depth = std::max(1u, srctex.desc.depth >> mip);
							const uint32_t subresource = ComputeSubresource(mip, slice, plane, srctex.desc.mip_levels, srctex.desc.array_size);
							const SubresourceData& subresource_data = dsttex.mapped_subresources[subresource];
							uint64_t data_offset = uint64_t((uint64_t)subresource_data.data_ptr - data_begin);
							commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), slice, mip, { 0,0,0 }, { mip_width, mip_height, mip_depth }, dst_internal->buffer.get(), data_offset, subresource_data.row_pitch, subresource_data.slice_pitch * mip_depth);
						}
					}
				}
			}
			else if(src_internal->texture.get() != nullptr && dst_internal->buffer.get() != nullptr)
			{
				// Linear->texture copy:
				const uint64_t data_begin = (uint64_t)srctex.mapped_subresources[0].data_ptr;
				const uint32_t planes = GetFormatPlaneCount(dsttex.desc.format); // dst plane count!
				for (uint32_t plane = 0; plane < planes; ++plane)
				{
					for (uint32_t slice = 0; slice < dsttex.desc.array_size; ++slice)
					{
						for (uint32_t mip = 0; mip < dsttex.desc.mip_levels; ++mip)
						{
							const uint32_t mip_width = std::max(1u, dsttex.desc.width >> mip);
							const uint32_t mip_height = std::max(1u, dsttex.desc.height >> mip);
							const uint32_t mip_depth = std::max(1u, dsttex.desc.depth >> mip);
							const uint32_t subresource = ComputeSubresource(mip, slice, plane, dsttex.desc.mip_levels, dsttex.desc.array_size);
							const SubresourceData& subresource_data = srctex.mapped_subresources[subresource];
							uint64_t data_offset = uint64_t((uint64_t)subresource_data.data_ptr - data_begin);
							commandlist.compute_encoder->copyFromBuffer(src_internal->buffer.get(), data_offset, subresource_data.row_pitch, subresource_data.slice_pitch * mip_depth, { mip_width,mip_height,mip_depth }, dst_internal->texture.get(), slice, mip, { 0,0,0 });
						}
					}
				}
			}
		}
		else if (pDst->IsBuffer() && pSrc->IsBuffer())
		{
			auto src_internal = to_internal<GPUBuffer>(pSrc);
			auto dst_internal = to_internal<GPUBuffer>(pDst);
			commandlist.compute_encoder->copyFromBuffer(src_internal->buffer.get(), 0, dst_internal->buffer.get(), 0, dst_internal->buffer->length());
		}
	}

	void MetalDevice::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);
		commandlist.compute_encoder->copyFromBuffer(internal_state_src->buffer.get(), src_offset, internal_state_dst->buffer.get(), dst_offset, size);
	}

	void MetalDevice::CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect)
	{
		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		const uint32_t srcWidth = std::max(1u, src->desc.width >> srcMip);
		const uint32_t srcHeight = std::max(1u, src->desc.height >> srcMip);
		const uint32_t srcDepth = std::max(1u, src->desc.depth >> srcMip);
		const uint32_t dstWidth = std::max(1u, dst->desc.width >> dstMip);
		const uint32_t dstHeight = std::max(1u, dst->desc.height >> dstMip);
		const uint32_t dstDepth = std::max(1u, dst->desc.depth >> dstMip);
		auto src_internal = to_internal(src);
		auto dst_internal = to_internal(dst);
		MTL::Origin srcOrigin = {};
		MTL::Size srcSize = {};
		MTL::Origin dstOrigin = { dstX, dstY, dstZ };
		if (srcbox == nullptr)
		{
			srcSize.width = srcWidth;
			srcSize.height = srcHeight;
			srcSize.depth = srcDepth;
		}
		else
		{
			srcOrigin.x = srcbox->left;
			srcOrigin.y = srcbox->top;
			srcOrigin.z = srcbox->front;
			srcSize.width = srcbox->right - srcbox->left;
			srcSize.height = srcbox->bottom - srcbox->top;
			srcSize.depth = srcbox->back - srcbox->front;
		}
		
		if (src_internal->texture.get() != nullptr && dst_internal->texture.get() != nullptr)
		{
			// normal texture -> texture copy
			if (dst->desc.format != src->desc.format)
			{
				// Hack:
				//	Metal cannot do format reinterpret texture->texture copy, so instead of rewriting my block compression shaders,
				//	I implement texture->buffer->texture copy as a workaround
				const size_t buffer_size = ComputeTextureMipMemorySizeInBytes(src->desc, srcMip);
				const size_t row_pitch = ComputeTextureMipRowPitch(src->desc, srcMip);
				static NS::SharedPtr<MTL::Buffer> reinterpret_buffer[QUEUE_COUNT]; // one per queue for correct gpu multi-queue hazard safety
				static std::mutex locker;
				std::scoped_lock lck(locker);
				if (reinterpret_buffer[commandlist.queue].get() == nullptr || reinterpret_buffer[commandlist.queue]->length() < buffer_size)
				{
					if (reinterpret_buffer[commandlist.queue].get() != nullptr)
					{
						std::scoped_lock lck2(allocationhandler->destroylocker);
						allocationhandler->destroyer_resources.push_back(std::make_pair(reinterpret_buffer[commandlist.queue], allocationhandler->framecount));
					}
					reinterpret_buffer[commandlist.queue] = NS::TransferPtr(device->newBuffer(buffer_size, MTL::ResourceStorageModePrivate));
					reinterpret_buffer[commandlist.queue]->setLabel(NS::TransferPtr(NS::String::alloc()->init("reinterpret_buffer", NS::UTF8StringEncoding)).get());
					allocationhandler->make_resident(reinterpret_buffer[commandlist.queue].get());
				}
				commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), srcSlice, srcMip, srcOrigin, srcSize, reinterpret_buffer[commandlist.queue].get(), 0, row_pitch, buffer_size);
				commandlist.compute_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, MTL4::VisibilityOptionNone);
				if (!IsFormatBlockCompressed(src->desc.format) && IsFormatBlockCompressed(dst->desc.format))
				{
					// raw -> block compressed copy
					const uint32_t block_size = GetFormatBlockSize(dst->desc.format);
					srcSize.width = std::min((uint32_t)srcSize.width * block_size, dstWidth);
					srcSize.height = std::min((uint32_t)srcSize.height * block_size, dstHeight);
				}
				if (IsFormatBlockCompressed(src->desc.format) && !IsFormatBlockCompressed(dst->desc.format))
				{
					// block compressed -> raw copy
					const uint32_t block_size = GetFormatBlockSize(dst->desc.format);
					srcSize.width = std::max(1u, (uint32_t)srcSize.width / block_size);
					srcSize.height = std::max(1u, (uint32_t)srcSize.height / block_size);
				}
				commandlist.compute_encoder->copyFromBuffer(reinterpret_buffer[commandlist.queue].get(), 0, row_pitch, buffer_size, srcSize, dst_internal->texture.get(), dstSlice, dstMip, dstOrigin);
				commandlist.compute_encoder->barrierAfterStages(MTL::StageAll, MTL::StageAll, MTL4::VisibilityOptionNone);
			}
			else
			{
				commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), srcSlice, srcMip, srcOrigin, srcSize, dst_internal->texture.get(), dstSlice, dstMip, dstOrigin);
			}
		}
		else if (src_internal->texture.get() != nullptr && dst_internal->buffer.get() != nullptr)
		{
			// texture -> linear copy
			const uint32_t subresource = ComputeSubresource(dstMip, dstSlice, dst_aspect, dst->desc.mip_levels, dst->desc.array_size);
			assert(dst->mapped_subresource_count > subresource);
			const SubresourceData& data0 = dst->mapped_subresources[0];
			const SubresourceData& data = dst->mapped_subresources[subresource];
			const uint64_t dst_offset = (uint64_t)data.data_ptr - (uint64_t)data0.data_ptr;
			commandlist.compute_encoder->copyFromTexture(src_internal->texture.get(), srcSlice, srcMip, srcOrigin, srcSize, dst_internal->buffer.get(), dst_offset, data.row_pitch, data.row_pitch * dstHeight * dstDepth);
		}
		else if (src_internal->buffer.get() != nullptr && dst_internal->texture.get() != nullptr)
		{
			// linear -> texture copy
			const uint32_t subresource = ComputeSubresource(srcMip, srcSlice, src_aspect, src->desc.mip_levels, src->desc.array_size);
			assert(src->mapped_subresource_count > subresource);
			const SubresourceData& data0 = src->mapped_subresources[0];
			const SubresourceData& data = src->mapped_subresources[subresource];
			uint64_t src_offset = (uint64_t)data.data_ptr - (uint64_t)data0.data_ptr;
			src_offset += srcbox->top * data.row_pitch + srcbox->left * GetFormatStride(src->desc.format) / GetFormatBlockSize(src->desc.format);
			commandlist.compute_encoder->copyFromBuffer(src_internal->buffer.get(), src_offset, data.row_pitch, data.row_pitch * srcHeight * srcDepth, srcSize, dst_internal->texture.get(), dstSlice, dstMip, dstOrigin);
		}
		else
		{
			assert(0); // not implemented
		}
	}

	void MetalDevice::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		switch (heap->desc.type)
		{
			case GpuQueryType_OCCLUSION:
				assert(commandlist.render_encoder.get() != nullptr);
				commandlist.render_encoder->setVisibilityResultMode(MTL::VisibilityResultModeCounting, index * sizeof(uint64_t));
				break;
			case GpuQueryType_OCCLUSION_BINARY:
				assert(commandlist.render_encoder.get() != nullptr);
				commandlist.render_encoder->setVisibilityResultMode(MTL::VisibilityResultModeBoolean, index * sizeof(uint64_t));
				break;
			case GpuQueryType_TIMESTAMP:
				break;
			default:
				break;
		}
	}

	void MetalDevice::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);
		switch (heap->desc.type)
		{
			case GpuQueryType_OCCLUSION:
			case GpuQueryType_OCCLUSION_BINARY:
				assert(commandlist.render_encoder.get() != nullptr);
				commandlist.render_encoder->setVisibilityResultMode(MTL::VisibilityResultModeDisabled, index * sizeof(uint64_t));
				break;
			case GpuQueryType_TIMESTAMP:
				if (commandlist.render_encoder.get() != nullptr)
				{
					// Note: fMTL::RenderStageFragment timestamp is unreliable if no fragment was rendered. But we can't determine here in a non-intrusive way whether a fragment was rendered or not in the current render pass for this timestamp
					commandlist.render_encoder->writeTimestamp(MTL4::TimestampGranularityPrecise, MTL::RenderStageFragment, internal_state->counter_heap.get(), index);
				}
				else if (commandlist.compute_encoder.get() != nullptr)
				{
					commandlist.compute_encoder->writeTimestamp(MTL4::TimestampGranularityPrecise, internal_state->counter_heap.get(), index);
				}
				else
				{
					commandlist.commandbuffer->writeTimestampIntoHeap(internal_state->counter_heap.get(), index);
				}
				break;
			default:
				break;
		}
	}

	void MetalDevice::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		if (count == 0)
			return;
		CommandList_Metal& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);
		
		switch (heap->desc.type)
		{
			case GpuQueryType_OCCLUSION:
			case GpuQueryType_OCCLUSION_BINARY:
				precopy(cmd);
				commandlist.compute_encoder->copyFromBuffer(internal_state->buffer.get(), index * sizeof(uint64_t), dst_internal->buffer.get(), dest_offset, count * sizeof(uint64_t));
				break;
			case GpuQueryType_TIMESTAMP:
				if (commandlist.compute_encoder.get() != nullptr)
				{
					commandlist.compute_encoder->endEncoding();
					commandlist.compute_encoder.reset();
				}
				commandlist.commandbuffer->resolveCounterHeap(internal_state->counter_heap.get(), {index, count}, {dst_internal->gpu_address, count * sizeof(uint64_t)}, nullptr, nullptr);
				break;
			default:
				break;
		}
	}

	void MetalDevice::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];
			commandlist.barriers.push_back(barrier);
		}
	}

	void MetalDevice::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		predispatch(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		auto dst_internal = to_internal(dst);
		
		// descriptor is recreated because buffer references might have changed since creation:
		NS::SharedPtr<MTL4::AccelerationStructureDescriptor> descriptor = mtl_acceleration_structure_descriptor(&dst->desc);
		
		if (src != nullptr && (dst->desc.flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE))
		{
			auto src_internal = to_internal(src);
			commandlist.compute_encoder->refitAccelerationStructure(src_internal->acceleration_structure.get(), descriptor.get(), dst_internal->acceleration_structure.get(), {dst_internal->scratch->gpuAddress(), dst_internal->scratch->length()});
		}
		else
		{
			commandlist.compute_encoder->buildAccelerationStructure(dst_internal->acceleration_structure.get(), descriptor.get(), {dst_internal->scratch->gpuAddress(), dst_internal->scratch->length()});
		}
	}

	void MetalDevice::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		// TODO
	}

	void MetalDevice::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		// TODO
	}

	void MetalDevice::PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset)
	{
		assert(offset + size < sizeof(RootLayout::constants));
		CommandList_Metal& commandlist = GetCommandList(cmd);
		std::memcpy((uint8_t*)commandlist.root.constants + offset, data, size);
		commandlist.dirty_root = true;
	}

	void MetalDevice::PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd)
	{
		// This is not supported in Metal API
	}

	void MetalDevice::PredicationEnd(CommandList cmd)
	{
		// This is not supported in Metal API
	}

	void MetalDevice::ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd)
	{
		// Note: the clears with Metal API don't always match up with the uint32_t clear value that is provided to this function, but in the usual case for clearing to 0 or some common values it will work
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (resource->IsTexture())
		{
			auto internal_state = to_internal<Texture>(resource);
			bool found = false;
			for (auto& x : commandlist.texture_clears)
			{
				if (x.first.get() == internal_state->texture.get())
				{
					// Avoid adding batched clear command twice
					found = true;
					break;
				}
			}
			if (!found)
			{
				commandlist.texture_clears.push_back(std::make_pair(internal_state->texture, value));
			}
		}
		else if (resource->IsBuffer())
		{
			precopy(cmd);
			auto internal_state = to_internal<GPUBuffer>(resource);
			commandlist.compute_encoder->fillBuffer(internal_state->buffer.get(), {0, internal_state->buffer->length()}, (uint8_t)value);
		}
	}

	void MetalDevice::VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd)
	{
		if (video_decoder == nullptr || op == nullptr || !video_decoder->IsValid())
			return;
		auto decoder = to_internal(video_decoder);
		if (decoder == nullptr)
			return;
		if (op->stream == nullptr || op->stream->mapped_data == nullptr)
		{
			wilog_error("Metal video decoder requires a CPU-visible bitstream buffer");
			return;
		}
		if (op->stream_offset + op->stream_size > op->stream->mapped_size)
		{
			wilog_error("Metal video decoder bitstream range is out of bounds");
			return;
		}
		if (decoder->format_desc == nullptr)
		{
			wilog_error("Metal video decoder is missing SPS and PPS");
			return;
		}
		if ((op->flags & VideoDecodeOperation::FLAG_SESSION_RESET) && decoder->frames_decoded > 0)
		{
			decoder->destroy_session();
		}
		if (decoder->session == nullptr)
		{
			if (decoder->output_attrs == nullptr)
			{
				int32_t pixel_format = kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange;
				CFNumberRef pixel_format_num = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pixel_format);
				CFDictionaryRef io_surface = CFDictionaryCreate(kCFAllocatorDefault, nullptr, nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				const void* keys[] = {
					kCVPixelBufferPixelFormatTypeKey,
					kCVPixelBufferMetalCompatibilityKey,
					kCVPixelBufferIOSurfacePropertiesKey,
				};
				const void* values[] = {
					pixel_format_num,
					kCFBooleanTrue,
					io_surface,
				};
				decoder->output_attrs = CFDictionaryCreate(kCFAllocatorDefault, keys, values, arraysize(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
				CFRelease(io_surface);
				CFRelease(pixel_format_num);
			}
			if (decoder->output_attrs == nullptr)
			{
				wilog_error("Metal video decoder session was not created");
				return;
			}
			decoder->destroy_session();
			VTDecompressionOutputCallbackRecord callback = {};
			callback.decompressionOutputCallback = video_output_callback;
			callback.decompressionOutputRefCon = decoder;
			const OSStatus session_status = VTDecompressionSessionCreate(kCFAllocatorDefault, decoder->format_desc, nullptr, decoder->output_attrs, &callback, &decoder->session);
			if (session_status != noErr || decoder->session == nullptr)
			{
				wilog_error("VTDecompressionSessionCreate failed (%d)", (int)session_status);
				decoder->session = nullptr;
				wilog_error("Metal video decoder session was not created");
				return;
			}
		}

		const Texture* target = op->output != nullptr ? op->output : op->DPB;
		if (target == nullptr || target->desc.format != Format_NV12)
		{
			wilog_error("Metal video decoder output must be an NV12 texture");
			return;
		}
		auto target_internal = to_internal(target);
		if (target_internal->texture.get() == nullptr || target_internal->texture_chroma.get() == nullptr)
		{
			wilog_error("Metal video decoder NV12 planes are missing");
			return;
		}
		const uint32_t slice = op->output != nullptr ? 0u : op->current_dpb;
		if (slice >= target->desc.array_size)
		{
			wilog_error("Metal video decoder DPB slot is out of range");
			return;
		}

		if (op->stream_size < 4)
		{
			wilog_error("Metal video decoder bitstream frame is empty");
			return;
		}
		const uint8_t* src = (const uint8_t*)op->stream->mapped_data + op->stream_offset;
		const size_t src_size = (size_t)op->stream_size;
		size_t start = 0;
		if (src_size >= 4 && src[0] == 0 && src[1] == 0 && src[2] == 0 && src[3] == 1)
			start = 4;
		else if (src_size >= 3 && src[0] == 0 && src[1] == 0 && src[2] == 1)
			start = 3;
		else
		{
			wilog_error("Metal video decoder bitstream frame is not Annex-B");
			return;
		}
		const size_t nal_size = src_size - start;
		if (nal_size == 0 || nal_size > 0xffffffffu)
		{
			wilog_error("Metal video decoder bitstream frame is empty");
			return;
		}
		// The shared bitstream is Annex-B. VideoToolbox samples are 4-byte length prefixed.
		const size_t sample_size = nal_size + 4;
		if (decoder->sample_bytes.size() < sample_size)
			decoder->sample_bytes.resize(sample_size);
		uint8_t* dst = decoder->sample_bytes.data();
		dst[0] = (uint8_t)(nal_size >> 24);
		dst[1] = (uint8_t)(nal_size >> 16);
		dst[2] = (uint8_t)(nal_size >> 8);
		dst[3] = (uint8_t)nal_size;
		std::memcpy(dst + 4, src + start, nal_size);

		CMBlockBufferRef block = nullptr;
		OSStatus status = CMBlockBufferCreateWithMemoryBlock(
			kCFAllocatorDefault,
			dst,
			decoder->sample_bytes.size(),
			kCFAllocatorNull,
			nullptr,
			0,
			sample_size,
			0,
			&block
		);
		if (status != noErr || block == nullptr)
		{
			wilog_error("CMBlockBufferCreateWithMemoryBlock failed");
			return;
		}
		CMSampleBufferRef sample = nullptr;
		status = CMSampleBufferCreateReady(
			kCFAllocatorDefault,
			block,
			decoder->format_desc,
			1,
			0,
			nullptr,
			1,
			&sample_size,
			&sample
		);
		if (status != noErr || sample == nullptr)
		{
			CFRelease(block);
			wilog_error("CMSampleBufferCreateReady failed");
			return;
		}

		{
			std::scoped_lock lock(decoder->locker);
			decoder->decode_status = noErr;
			if (decoder->decoded != nullptr)
			{
				CFRelease(decoder->decoded);
				decoder->decoded = nullptr;
			}
		}
		VTDecodeInfoFlags info_flags = 0;
		status = VTDecompressionSessionDecodeFrame(decoder->session, sample, 0, nullptr, &info_flags);
		if (status == noErr)
		{
			status = VTDecompressionSessionWaitForAsynchronousFrames(decoder->session);
		}
		CFRelease(sample);
		CFRelease(block);
		if (status != noErr)
		{
			wilog_error("VTDecompressionSessionDecodeFrame failed");
			return;
		}

		CVPixelBufferRef frame = nullptr;
		OSStatus decode_status = noErr;
		{
			std::scoped_lock lock(decoder->locker);
			frame = decoder->decoded;
			decoder->decoded = nullptr;
			decode_status = decoder->decode_status;
		}
		if (frame == nullptr || decode_status != noErr)
		{
			if (frame != nullptr)
				CFRelease(frame);
			wilog_error("Metal video decoder produced no output frame");
			return;
		}
		if (!CVPixelBufferIsPlanar(frame) || CVPixelBufferGetPlaneCount(frame) < 2)
		{
			CFRelease(frame);
			wilog_error("Metal video decoder output is not bi-planar NV12");
			return;
		}
		if (decoder->texture_cache == nullptr)
		{
			CFRelease(frame);
			wilog_error("Metal video decoder texture cache is missing");
			return;
		}

		CVMetalTextureRef cv_planes[2] = {};
		MTL::Texture* src_planes[2] = {};
		MTL::Texture* dst_planes[2] = {
			target_internal->texture.get(),
			target_internal->texture_chroma.get(),
		};
		const MTL::PixelFormat plane_formats[2] = {
			MTL::PixelFormatR8Unorm,
			MTL::PixelFormatRG8Unorm,
		};
		NS::UInteger copy_width[2] = {};
		NS::UInteger copy_height[2] = {};
		bool planes_ok = true;
		for (uint32_t i = 0; i < 2; ++i)
		{
			const size_t plane_width = CVPixelBufferGetWidthOfPlane(frame, i);
			const size_t plane_height = CVPixelBufferGetHeightOfPlane(frame, i);
			if (plane_width == 0 || plane_height == 0)
			{
				planes_ok = false;
				break;
			}
			const CVReturn cv_status = CVMetalTextureCacheCreateTextureFromImage(
				kCFAllocatorDefault,
				decoder->texture_cache,
				frame,
				nullptr,
				(NS::UInteger)plane_formats[i],
				plane_width,
				plane_height,
				i,
				&cv_planes[i]
			);
			src_planes[i] = cv_planes[i] != nullptr ? (MTL::Texture*)CVMetalTextureGetTexture(cv_planes[i]) : nullptr;
			if (cv_status != kCVReturnSuccess || src_planes[i] == nullptr)
			{
				planes_ok = false;
				break;
			}
			copy_width[i] = std::min(dst_planes[i]->width(), (NS::UInteger)plane_width);
			copy_height[i] = std::min(dst_planes[i]->height(), (NS::UInteger)plane_height);
		}
		if (!planes_ok)
		{
			for (uint32_t i = 0; i < 2; ++i)
			{
				if (cv_planes[i] != nullptr)
					CFRelease(cv_planes[i]);
			}
			CFRelease(frame);
			wilog_error("CVMetalTextureCacheCreateTextureFromImage failed");
			return;
		}

		for (uint32_t i = 0; i < 2; ++i)
			allocationhandler->make_resident(src_planes[i]);

		const uint64_t frame_index = GetFrameCount();
		decoder->release_pending(frame_index, false);
		VideoDecoder_Metal::PendingCopy pending = {};
		pending.image = frame;
		pending.planes[0] = cv_planes[0];
		pending.planes[1] = cv_planes[1];
		pending.textures[0] = src_planes[0];
		pending.textures[1] = src_planes[1];
		pending.frame = frame_index;
		decoder->pending_copies.push_back(pending);

		precopy(cmd);
		CommandList_Metal& commandlist = GetCommandList(cmd);
		for (uint32_t i = 0; i < 2; ++i)
		{
			if (copy_width[i] == 0 || copy_height[i] == 0)
				continue;
			commandlist.compute_encoder->copyFromTexture(
				src_planes[i],
				0,
				0,
				{ 0, 0, 0 },
				{ copy_width[i], copy_height[i], 1u },
				dst_planes[i],
				slice,
				0,
				{ 0, 0, 0 }
			);
		}
		decoder->frames_decoded++;
	}

	void MetalDevice::EventBegin(const char* name, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		NS::SharedPtr<NS::String> str = NS::TransferPtr(NS::String::alloc()->init(name, NS::UTF8StringEncoding));
		if (commandlist.render_encoder.get() != nullptr)
		{
			commandlist.render_encoder->pushDebugGroup(str.get());
		}
		else if (commandlist.compute_encoder.get() != nullptr)
		{
			commandlist.compute_encoder->pushDebugGroup(str.get());
		}
		else
		{
			commandlist.commandbuffer->pushDebugGroup(str.get());
		}
	}

	void MetalDevice::EventEnd(CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		if (commandlist.render_encoder.get() != nullptr)
		{
			commandlist.render_encoder->popDebugGroup();
		}
		else if (commandlist.compute_encoder.get() != nullptr)
		{
			commandlist.compute_encoder->popDebugGroup();
		}
		else
		{
			commandlist.commandbuffer->popDebugGroup();
		}
	}

	void MetalDevice::SetMarker(const char* name, CommandList cmd)
	{
		CommandList_Metal& commandlist = GetCommandList(cmd);
		NS::SharedPtr<NS::String> str = NS::TransferPtr(NS::String::alloc()->init(name, NS::UTF8StringEncoding));
		if (commandlist.render_encoder.get() != nullptr)
		{
			commandlist.render_encoder->setLabel(str.get());
		}
		else if (commandlist.compute_encoder.get() != nullptr)
		{
			commandlist.compute_encoder->setLabel(str.get());
		}
		else
		{
			commandlist.commandbuffer->setLabel(str.get());
		}
	}
}

#endif // __APPLE__
