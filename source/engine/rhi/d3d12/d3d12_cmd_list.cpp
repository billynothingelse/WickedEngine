#include "rhi/d3d12/d3d12_internal.h"
// D3D12 backend: Command list recording

#ifdef WICKEDENGINE_BUILD_DX12

namespace Protean::Graphics
{
	using namespace D3D12Internal;

	void D3D12Device::pso_validate(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (!commandlist.dirty_pso)
			return;

		const PipelineState* pso = commandlist.active_pso;
		PipelineHash pipeline_hash = commandlist.prev_pipeline_hash;

		auto internal_state = to_internal(pso);

		ID3D12PipelineState* pipeline = nullptr;
		auto it = pipelines_global.find(pipeline_hash);
		if (it == pipelines_global.end())
		{
			for (auto& x : commandlist.pipelines_worker)
			{
				if (pipeline_hash == x.first)
				{
					auto pipeline_internal = to_internal(&x.second);
					pipeline = pipeline_internal->resource.Get();
					break;
				}
			}

			if (pipeline == nullptr)
			{
				PipelineState just_in_time_pso;
				bool success = CreatePipelineState(&pso->desc, &just_in_time_pso, &commandlist.renderpass_info);
				assert(success);

				commandlist.pipelines_worker.push_back(std::make_pair(pipeline_hash, just_in_time_pso));

				auto pipeline_internal = to_internal(&just_in_time_pso);
				pipeline = pipeline_internal->resource.Get();
			}
		}
		else
		{
			auto pipeline_internal = to_internal(&it->second);
			pipeline = pipeline_internal->resource.Get();
		}
		assert(pipeline != nullptr);

		commandlist.GetGraphicsCommandList()->SetPipelineState(pipeline);
		commandlist.dirty_pso = false;

		if (commandlist.prev_pt != internal_state->primitiveTopology)
		{
			commandlist.prev_pt = internal_state->primitiveTopology;

			commandlist.GetGraphicsCommandList()->IASetPrimitiveTopology(internal_state->primitiveTopology);
		}
	}

	void D3D12Device::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(true, cmd);
	}

	void D3D12Device::predispatch(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(false, cmd);
	}

	void D3D12Device::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		CommandList_DX12& commandlist_wait_for = GetCommandList(wait_for);
		assert(commandlist_wait_for.id < commandlist.id); // can't wait for future command list!
		Semaphore semaphore = new_semaphore();
		commandlist.waits.push_back(semaphore);
		commandlist_wait_for.signals.push_back(semaphore);
	}

	void D3D12Device::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();
		commandlist.swapchains.push_back(swapchain);
		auto internal_state = to_internal(swapchain);

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = internal_state->textures[internal_state->GetBufferIndex()]->resource.Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		commandlist.GetGraphicsCommandList()->ResourceBarrier(1, &barrier);

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		commandlist.renderpass_barriers_end.push_back(barrier);

#ifdef DISABLE_RENDERPASS
		commandlist.GetGraphicsCommandList()->OMSetRenderTargets(
			1,
			&internal_state->textures[internal_state->GetBufferIndex()]->rtv.handle,
			TRUE,
			nullptr
		);
		commandlist.GetGraphicsCommandList()->ClearRenderTargetView(
			internal_state->textures[internal_state->GetBufferIndex()]->rtv.handle,
			swapchain->desc.clear_color,
			0,
			nullptr
		);
#else
		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTV = {};
		RTV.cpuDescriptor = internal_state->textures[internal_state->GetBufferIndex()]->rtv.handle;
		RTV.BeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
		RTV.BeginningAccess.Clear.ClearValue.Color[0] = swapchain->desc.clear_color[0];
		RTV.BeginningAccess.Clear.ClearValue.Color[1] = swapchain->desc.clear_color[1];
		RTV.BeginningAccess.Clear.ClearValue.Color[2] = swapchain->desc.clear_color[2];
		RTV.BeginningAccess.Clear.ClearValue.Color[3] = swapchain->desc.clear_color[3];
		RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
		commandlist.GetGraphicsCommandListLatest()->BeginRenderPass(1, &RTV, nullptr, D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES);
#endif // DISABLE_RENDERPASS

		commandlist.renderpass_info = RenderPassInfo::from(swapchain->desc);
	}

	void D3D12Device::RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();

		for (uint32_t rt = 0; rt < commandlist.renderpass_info.rt_count; ++rt)
		{
			commandlist.resolve_subresources[rt].clear();
			commandlist.resolve_dst[rt] = nullptr;
			commandlist.resolve_src[rt] = nullptr;
		}
		commandlist.resolve_subresources_dsv.clear();
		commandlist.resolve_dst_ds = nullptr;
		commandlist.resolve_src_ds = nullptr;

		uint32_t rt_count = 0;
		D3D12_RENDER_PASS_RENDER_TARGET_DESC RTVs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC DSV = {};

		// Some bookkeeping for resolves:
		uint32_t rt_resolve_count = 0;
		struct ResolveSourceInfo
		{
			ID3D12Resource* resource = nullptr;
			BOOL preserve = FALSE;
			uint32_t firstMip = 0;
			uint32_t mipCount = 0;
			uint32_t firstSlice = 0;
			uint32_t sliceCount = 0;
			uint32_t total_mipCount = 0;
			uint32_t total_sliceCount = 0;
		};
		ResolveSourceInfo RT_resolve_src_infos[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT];
		ResolveSourceInfo DS_resolve_src_info;

		for (uint32_t i = 0; i < image_count; ++i)
		{
			const RenderPassImage& image = images[i];
			const Texture* texture = image.texture;
			const TextureDesc& desc = texture->GetDesc();
			int subresource = image.subresource;
			auto internal_state = to_internal(texture);

			D3D12_CLEAR_VALUE clear_value;
			clear_value.Format = _ConvertFormat(desc.format);

			D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE beginning_access_type;
			switch (image.loadop)
			{
			default:
			case RenderPassImage::LoadOp_LOAD:
				beginning_access_type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_PRESERVE;
				break;
			case RenderPassImage::LoadOp_CLEAR:
				beginning_access_type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR;
				break;
			case RenderPassImage::LoadOp_DONTCARE:
				beginning_access_type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_DISCARD;
				break;
			}

			D3D12_RENDER_PASS_ENDING_ACCESS_TYPE ending_access_type;
			switch (image.storeop)
			{
			default:
			case RenderPassImage::StoreOp_STORE:
				ending_access_type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				break;
			case RenderPassImage::StoreOp_DONTCARE:
				ending_access_type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD;
				break;
			}

			SingleDescriptor descriptor;

			switch (image.type)
			{
			case RenderPassImage::Type_RENDERTARGET:
			{
				descriptor = subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[subresource];
				ResolveSourceInfo& resolve_src_info = RT_resolve_src_infos[rt_count];
				D3D12_RENDER_PASS_RENDER_TARGET_DESC& RTV = RTVs[rt_count++];
				RTV.cpuDescriptor = descriptor.handle;
				RTV.BeginningAccess.Type = beginning_access_type;
				RTV.EndingAccess.Type = ending_access_type;
				clear_value.Color[0] = desc.clear.color[0];
				clear_value.Color[1] = desc.clear.color[1];
				clear_value.Color[2] = desc.clear.color[2];
				clear_value.Color[3] = desc.clear.color[3];
				RTV.BeginningAccess.Clear.ClearValue = clear_value;
				resolve_src_info.resource = internal_state->resource.Get();
				resolve_src_info.preserve = ending_access_type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				resolve_src_info.firstMip = descriptor.firstMip;
				resolve_src_info.mipCount = descriptor.mipCount;
				resolve_src_info.firstSlice = descriptor.firstSlice;
				resolve_src_info.sliceCount = descriptor.sliceCount;
				resolve_src_info.total_mipCount = desc.mip_levels;
				resolve_src_info.total_sliceCount = desc.array_size;
			}
			break;

			case RenderPassImage::Type_RESOLVE:
			{
				descriptor = subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[subresource];
				ResolveSourceInfo& resolve_src_info = RT_resolve_src_infos[rt_resolve_count];
				D3D12_RENDER_PASS_RENDER_TARGET_DESC& RTV = RTVs[rt_resolve_count];
				RTV.EndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
				RTV.EndingAccess.Resolve.Format = clear_value.Format;
				RTV.EndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_AVERAGE;
				RTV.EndingAccess.Resolve.pDstResource = internal_state->resource.Get();
				RTV.EndingAccess.Resolve.pSrcResource = resolve_src_info.resource;
				RTV.EndingAccess.Resolve.PreserveResolveSource = resolve_src_info.preserve;
				for (uint32_t mip = 0; mip < std::min(desc.mip_levels, descriptor.mipCount); ++mip)
				{
					for (uint32_t slice = 0; slice < std::min(desc.array_size, descriptor.sliceCount); ++slice)
					{
						D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS& params = commandlist.resolve_subresources[rt_resolve_count].emplace_back();
						params.SrcSubresource = D3D12CalcSubresource(resolve_src_info.firstMip + mip, resolve_src_info.firstSlice + slice, 0, resolve_src_info.total_mipCount, resolve_src_info.total_sliceCount);
						params.DstSubresource = D3D12CalcSubresource(descriptor.firstMip + mip, descriptor.firstSlice + slice, 0, desc.mip_levels, desc.array_size);
						params.SrcRect.left = 0;
						params.SrcRect.top = 0;
						params.SrcRect.right = (LONG)desc.width;
						params.SrcRect.bottom = (LONG)desc.height;
					}
				}
				RTV.EndingAccess.Resolve.pSubresourceParameters = commandlist.resolve_subresources[rt_resolve_count].data();
				RTV.EndingAccess.Resolve.SubresourceCount = (UINT)commandlist.resolve_subresources[rt_resolve_count].size();
				commandlist.resolve_src[rt_resolve_count] = RTV.EndingAccess.Resolve.pSrcResource;
				commandlist.resolve_dst[rt_resolve_count] = RTV.EndingAccess.Resolve.pDstResource;
				commandlist.resolve_formats[rt_resolve_count] = RTV.EndingAccess.Resolve.Format;
				rt_resolve_count++;
			}
			break;

			case RenderPassImage::Type_DEPTH_STENCIL:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				DSV.cpuDescriptor = descriptor.handle;
				DSV.DepthBeginningAccess.Type = beginning_access_type;
				DSV.DepthEndingAccess.Type = ending_access_type;
				clear_value.DepthStencil.Depth = desc.clear.depth_stencil.depth;
				clear_value.DepthStencil.Stencil = desc.clear.depth_stencil.stencil;
				DSV.DepthBeginningAccess.Clear.ClearValue = clear_value;
				DSV.DepthEndingAccess.Resolve.pSrcResource = internal_state->resource.Get();
				DSV.DepthEndingAccess.Resolve.PreserveResolveSource = ending_access_type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				DS_resolve_src_info.resource = internal_state->resource.Get();
				DS_resolve_src_info.preserve = ending_access_type == D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE;
				DS_resolve_src_info.firstMip = descriptor.firstMip;
				DS_resolve_src_info.mipCount = descriptor.mipCount;
				DS_resolve_src_info.firstSlice = descriptor.firstSlice;
				DS_resolve_src_info.sliceCount = descriptor.sliceCount;
				DS_resolve_src_info.total_mipCount = desc.mip_levels;
				DS_resolve_src_info.total_sliceCount = desc.array_size;
				if (IsFormatStencilSupport(desc.format))
				{
					DSV.StencilBeginningAccess = DSV.DepthBeginningAccess;
					DSV.StencilEndingAccess = DSV.DepthEndingAccess;
				}
				else
				{
					DSV.StencilBeginningAccess.Type = D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS;
					DSV.StencilEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS;
				}
			}
			break;

			case RenderPassImage::Type_RESOLVE_DEPTH:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				DSV.DepthEndingAccess.Type = D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_RESOLVE;
				DSV.DepthEndingAccess.Resolve.Format = clear_value.Format;
				switch (image.depth_resolve_mode)
				{
				default:
				case RenderPassImage::DepthResolveMode_Min:
					DSV.DepthEndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_MIN;
					break;
				case RenderPassImage::DepthResolveMode_Max:
					DSV.DepthEndingAccess.Resolve.ResolveMode = D3D12_RESOLVE_MODE_MAX;
					break;
				}
				DSV.DepthEndingAccess.Resolve.pDstResource = internal_state->resource.Get();
				DSV.DepthEndingAccess.Resolve.pSrcResource = DS_resolve_src_info.resource;
				DSV.DepthEndingAccess.Resolve.PreserveResolveSource = DS_resolve_src_info.preserve;
				for (uint32_t mip = 0; mip < std::min(desc.mip_levels, descriptor.mipCount); ++mip)
				{
					for (uint32_t slice = 0; slice < std::min(desc.array_size, descriptor.sliceCount); ++slice)
					{
						D3D12_RENDER_PASS_ENDING_ACCESS_RESOLVE_SUBRESOURCE_PARAMETERS& params = commandlist.resolve_subresources_dsv.emplace_back();
						params.SrcSubresource = D3D12CalcSubresource(DS_resolve_src_info.firstMip + mip, DS_resolve_src_info.firstSlice + slice, 0, DS_resolve_src_info.total_mipCount, DS_resolve_src_info.total_sliceCount);
						params.DstSubresource = D3D12CalcSubresource(descriptor.firstMip + mip, descriptor.firstSlice + slice, 0, desc.mip_levels, desc.array_size);
						params.SrcRect.left = 0;
						params.SrcRect.top = 0;
						params.SrcRect.right = (LONG)desc.width;
						params.SrcRect.bottom = (LONG)desc.height;
					}
				}
				DSV.DepthEndingAccess.Resolve.pSubresourceParameters = commandlist.resolve_subresources_dsv.data();
				DSV.DepthEndingAccess.Resolve.SubresourceCount = (UINT)commandlist.resolve_subresources_dsv.size();
				if (IsFormatStencilSupport(desc.format))
				{
					DSV.StencilEndingAccess = DSV.DepthEndingAccess;
				}
				commandlist.resolve_dst_ds = DSV.DepthEndingAccess.Resolve.pDstResource;
				commandlist.resolve_src_ds = DSV.DepthEndingAccess.Resolve.pSrcResource;
				commandlist.resolve_ds_format = DSV.DepthEndingAccess.Resolve.Format;
			}
			break;

			case RenderPassImage::Type_SHADING_RATE_SOURCE:
				commandlist.shading_rate_image = internal_state->resource.Get(); // will be set after barriers
				break;

			default:
				break;
			}


			// Beginning barriers:
			{
				D3D12_RESOURCE_STATES before = _ParseResourceState(image.layout_before);
				D3D12_RESOURCE_STATES after = _ParseResourceState(image.layout);
				if (image.type == RenderPassImage::Type_RESOLVE || image.type == RenderPassImage::Type_RESOLVE_DEPTH)
				{
					after = D3D12_RESOURCE_STATE_RESOLVE_DEST;
				}
				if (before != after)
				{
					D3D12_RESOURCE_BARRIER barrierdesc = {};
					barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barrierdesc.Transition.pResource = internal_state->resource.Get();
					barrierdesc.Transition.StateBefore = before;
					barrierdesc.Transition.StateAfter = after;

					if (subresource >= 0)
					{
						// Need to unroll descriptor into multiple subresource barriers:
						for (uint32_t mip = descriptor.firstMip; mip < std::min(desc.mip_levels, descriptor.firstMip + descriptor.mipCount); ++mip)
						{
							for (uint32_t slice = descriptor.firstSlice; slice < std::min(desc.array_size, descriptor.firstSlice + descriptor.sliceCount); ++slice)
							{
								barrierdesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, desc.mip_levels, desc.array_size);
								commandlist.renderpass_barriers_begin.push_back(barrierdesc);
							}
						}
					}
					else
					{
						// Single barrier for whole resource:
						barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						commandlist.renderpass_barriers_begin.push_back(barrierdesc);
					}
				}
			}

			// Ending barriers:
			{
				D3D12_RESOURCE_STATES before = _ParseResourceState(image.layout);
				D3D12_RESOURCE_STATES after = _ParseResourceState(image.layout_after);
				if (image.type == RenderPassImage::Type_RESOLVE || image.type == RenderPassImage::Type_RESOLVE_DEPTH)
				{
					before = D3D12_RESOURCE_STATE_RESOLVE_DEST;
				}
				if (before != after)
				{
					D3D12_RESOURCE_BARRIER barrierdesc = {};
					barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					barrierdesc.Transition.pResource = internal_state->resource.Get();
					barrierdesc.Transition.StateBefore = before;
					barrierdesc.Transition.StateAfter = after;

					if (subresource >= 0)
					{
						// Need to unroll descriptor into multiple subresource barriers:
						for (uint32_t mip = descriptor.firstMip; mip < std::min(desc.mip_levels, descriptor.firstMip + descriptor.mipCount); ++mip)
						{
							for (uint32_t slice = descriptor.firstSlice; slice < std::min(desc.array_size, descriptor.firstSlice + descriptor.sliceCount); ++slice)
							{
								barrierdesc.Transition.Subresource = D3D12CalcSubresource(mip, slice, 0, desc.mip_levels, desc.array_size);
								commandlist.renderpass_barriers_end.push_back(barrierdesc);
							}
						}
					}
					else
					{
						// Single barrier for whole resource:
						barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						commandlist.renderpass_barriers_end.push_back(barrierdesc);
					}
				}
			}
		}

		if (!commandlist.renderpass_barriers_begin.empty())
		{
			commandlist.GetGraphicsCommandList()->ResourceBarrier((UINT)commandlist.renderpass_barriers_begin.size(), commandlist.renderpass_barriers_begin.data());
		}

		if (commandlist.shading_rate_image != nullptr)
		{
			commandlist.GetGraphicsCommandListLatest()->RSSetShadingRateImage(commandlist.shading_rate_image);
		}

#ifdef DISABLE_RENDERPASS

		D3D12_CPU_DESCRIPTOR_HANDLE rt_descriptors[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
		for (uint32_t i = 0; i < rt_count; ++i)
		{
			rt_descriptors[i] = RTVs[i].cpuDescriptor;
		}

		commandlist.GetGraphicsCommandList()->OMSetRenderTargets(
			rt_count,
			rt_descriptors,
			FALSE,
			DSV.cpuDescriptor.ptr == 0 ? nullptr : &DSV.cpuDescriptor
		);

		for (uint32_t i = 0; i < rt_count; ++i)
		{
			if (RTVs[i].BeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
			{
				commandlist.GetGraphicsCommandList()->ClearRenderTargetView(
					RTVs[i].cpuDescriptor,
					RTVs[i].BeginningAccess.Clear.ClearValue.Color,
					0,
					nullptr
				);
			}
		}
		if (DSV.DepthBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR || DSV.StencilBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
		{
			D3D12_CLEAR_FLAGS clear_flags = {};
			if (DSV.DepthBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
			{
				clear_flags |= D3D12_CLEAR_FLAG_DEPTH;
			}
			if (DSV.StencilBeginningAccess.Type == D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR)
			{
				clear_flags |= D3D12_CLEAR_FLAG_STENCIL;
			}
			commandlist.GetGraphicsCommandList()->ClearDepthStencilView(
				DSV.cpuDescriptor,
				clear_flags,
				DSV.DepthBeginningAccess.Clear.ClearValue.DepthStencil.Depth,
				DSV.StencilBeginningAccess.Clear.ClearValue.DepthStencil.Stencil,
				0,
				nullptr
			);
		}

#else
		D3D12_RENDER_PASS_FLAGS FLAGS = D3D12_RENDER_PASS_FLAG_NONE;
		if (has_flag(flags, RenderPassFlags_ALLOW_UAV_WRITES))
		{
			FLAGS |= D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
		}
		if (has_flag(flags, RenderPassFlags_SUSPENDING))
		{
			FLAGS |= D3D12_RENDER_PASS_FLAG_SUSPENDING_PASS;
		}
		if (has_flag(flags, RenderPassFlags_RESUMING))
		{
			FLAGS |= D3D12_RENDER_PASS_FLAG_RESUMING_PASS;
		}

		commandlist.GetGraphicsCommandListLatest()->BeginRenderPass(
			rt_count,
			RTVs,
			DSV.cpuDescriptor.ptr == 0 ? nullptr : &DSV,
			FLAGS
		);
#endif // DISABLE_RENDERPASS

		commandlist.renderpass_info = RenderPassInfo::from(images, image_count);
	}

	void D3D12Device::RenderPassEnd(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);

#ifdef DISABLE_RENDERPASS
		// Batch up resolve SRC barriers since XBOX cannot do it with RenderPass:
		commandlist.resolve_src_barriers.clear();
		for (uint32_t rt = 0; rt < commandlist.renderpass_info.rt_count; ++rt)
		{
			for (auto& resolve : commandlist.resolve_subresources[rt])
			{
				D3D12_RESOURCE_BARRIER& barrier = commandlist.resolve_src_barriers.emplace_back();
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = commandlist.resolve_src[rt];
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
				barrier.Transition.Subresource = resolve.SrcSubresource;
			}
		}
		if (commandlist.resolve_dst_ds != nullptr)
		{
			for (auto& resolve : commandlist.resolve_subresources_dsv)
			{
				D3D12_RESOURCE_BARRIER& barrier = commandlist.resolve_src_barriers.emplace_back();
				barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrier.Transition.pResource = commandlist.resolve_src_ds;
				barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
				barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
				barrier.Transition.Subresource = resolve.SrcSubresource;
			}
		}
		if (!commandlist.resolve_src_barriers.empty())
		{
			commandlist.GetGraphicsCommandList()->ResourceBarrier((UINT)commandlist.resolve_src_barriers.size(), commandlist.resolve_src_barriers.data());
		}

		// Perform all resolves:
		for (uint32_t rt = 0; rt < commandlist.renderpass_info.rt_count; ++rt)
		{
			for (auto& resolve : commandlist.resolve_subresources[rt])
			{
				commandlist.GetGraphicsCommandList()->ResolveSubresource(
					commandlist.resolve_dst[rt],
					resolve.DstSubresource,
					commandlist.resolve_src[rt],
					resolve.SrcSubresource,
					commandlist.resolve_formats[rt]
				);
			}
		}
		if (commandlist.resolve_dst_ds != nullptr)
		{
			for (auto& resolve : commandlist.resolve_subresources_dsv)
			{
				commandlist.GetGraphicsCommandList()->ResolveSubresource(
					commandlist.resolve_dst_ds,
					resolve.DstSubresource,
					commandlist.resolve_src_ds,
					resolve.SrcSubresource,
					commandlist.resolve_ds_format
				);
			}
		}

		if (!commandlist.resolve_src_barriers.empty())
		{
			for (auto& barrier : commandlist.resolve_src_barriers)
			{
				std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
			}
			commandlist.GetGraphicsCommandList()->ResourceBarrier((UINT)commandlist.resolve_src_barriers.size(), commandlist.resolve_src_barriers.data());
		}

#else
		commandlist.GetGraphicsCommandListLatest()->EndRenderPass();
#endif // DISABLE_RENDERPASS

		if (commandlist.shading_rate_image != nullptr)
		{
			commandlist.GetGraphicsCommandListLatest()->RSSetShadingRateImage(nullptr);
			commandlist.shading_rate_image = nullptr;
		}

		if (!commandlist.renderpass_barriers_end.empty())
		{
			commandlist.GetGraphicsCommandList()->ResourceBarrier(
				(UINT)commandlist.renderpass_barriers_end.size(),
				commandlist.renderpass_barriers_end.data()
			);
		}

		commandlist.renderpass_info = {};
	}

	void D3D12Device::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd)
	{
		// Ensure that engine side Rect structure matches the D3D12_RECT structure
		static_assert(sizeof(Rect) == sizeof(D3D12_RECT));
		static_assert(offsetof(Rect, left) == offsetof(D3D12_RECT, left));
		static_assert(offsetof(Rect, right) == offsetof(D3D12_RECT, right));
		static_assert(offsetof(Rect, top) == offsetof(D3D12_RECT, top));
		static_assert(offsetof(Rect, bottom) == offsetof(D3D12_RECT, bottom));

		assert(rects != nullptr);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->RSSetScissorRects(numRects, (const D3D12_RECT*)rects);
	}

	void D3D12Device::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		// Ensure that engine side Viewport structure matches the D3D12_VIEWPORT structure
		static_assert(sizeof(Viewport) == sizeof(D3D12_VIEWPORT));
		static_assert(offsetof(Viewport, top_left_x) == offsetof(D3D12_VIEWPORT, TopLeftX));
		static_assert(offsetof(Viewport, top_left_y) == offsetof(D3D12_VIEWPORT, TopLeftY));
		static_assert(offsetof(Viewport, width) == offsetof(D3D12_VIEWPORT, Width));
		static_assert(offsetof(Viewport, height) == offsetof(D3D12_VIEWPORT, Height));
		static_assert(offsetof(Viewport, min_depth) == offsetof(D3D12_VIEWPORT, MinDepth));
		static_assert(offsetof(Viewport, max_depth) == offsetof(D3D12_VIEWPORT, MaxDepth));

		assert(pViewports != nullptr);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->RSSetViewports(NumViewports, (const D3D12_VIEWPORT*)pViewports);
	}

	void D3D12Device::BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < DESCRIPTORBINDER_SRV_COUNT);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (binder.table.SRV[slot].internal_state != resource->internal_state || binder.table.SRV_index[slot] != subresource)
		{
			binder.table.SRV[slot] = *resource;
			binder.table.SRV_index[slot] = subresource;

			if (binder.optimizer_graphics != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_graphics;
				if (optimizer.SRV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_graphics |= 1ull << optimizer.SRV[slot];
				}
			}
			if (binder.optimizer_compute != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_compute;
				if (optimizer.SRV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_compute |= 1ull << optimizer.SRV[slot];
				}
			}
		}
	}

	void D3D12Device::BindResources(const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(resources[i], slot + i, cmd, -1);
			}
		}
	}

	void D3D12Device::BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		assert(slot < DESCRIPTORBINDER_UAV_COUNT);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (binder.table.UAV[slot].internal_state != resource->internal_state || binder.table.UAV_index[slot] != subresource)
		{
			binder.table.UAV[slot] = *resource;
			binder.table.UAV_index[slot] = subresource;

			if (binder.optimizer_graphics != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_graphics;
				if (optimizer.UAV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_graphics |= 1ull << optimizer.UAV[slot];
				}
			}
			if (binder.optimizer_compute != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_compute;
				if (optimizer.UAV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_compute |= 1ull << optimizer.UAV[slot];
				}
			}
		}
	}

	void D3D12Device::BindUAVs(const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(resources[i], slot + i, cmd, -1);
			}
		}
	}

	void D3D12Device::BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		assert(slot < DESCRIPTORBINDER_SAMPLER_COUNT);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (binder.table.SAM[slot].internal_state != sampler->internal_state)
		{
			binder.table.SAM[slot] = *sampler;

			if (binder.optimizer_graphics != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_graphics;
				if (optimizer.SAM[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_graphics |= 1ull << optimizer.SAM[slot];
				}
			}
			if (binder.optimizer_compute != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_compute;
				if (optimizer.SAM[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_compute |= 1ull << optimizer.SAM[slot];
				}
			}
		}
	}

	void D3D12Device::BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset)
	{
		assert(slot < DESCRIPTORBINDER_CBV_COUNT);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (binder.table.CBV[slot].internal_state != buffer->internal_state || binder.table.CBV_offset[slot] != offset)
		{
			binder.table.CBV[slot] = *buffer;
			binder.table.CBV_offset[slot] = offset;

			if (binder.optimizer_graphics != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_graphics;
				if (optimizer.CBV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_graphics |= 1ull << optimizer.CBV[slot];
				}
			}
			if (binder.optimizer_compute != nullptr)
			{
				const RootSignatureOptimizer& optimizer = *(RootSignatureOptimizer*)binder.optimizer_compute;
				if (optimizer.CBV[slot] != RootSignatureOptimizer::INVALID_ROOT_PARAMETER)
				{
					binder.dirty_compute |= 1ull << optimizer.CBV[slot];
				}
			}
		}
	}

	void D3D12Device::BindVertexBuffers(const GPUBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd)
	{
		assert(count <= 8);
		D3D12_VERTEX_BUFFER_VIEW res[8] = {};
		for (uint32_t i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] != nullptr)
			{
				res[i].BufferLocation = vertexBuffers[i]->IsValid() ? to_internal(vertexBuffers[i])->gpu_address : 0;
				res[i].SizeInBytes = (UINT)vertexBuffers[i]->desc.size;
				if (offsets != nullptr)
				{
					res[i].BufferLocation += offsets[i];
					res[i].SizeInBytes -= (UINT)offsets[i];
				}
				res[i].StrideInBytes = strides[i];
			}
		}
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->IASetVertexBuffers(slot, count, res);
	}

	void D3D12Device::BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd)
	{
		D3D12_INDEX_BUFFER_VIEW res = {};
		if (indexBuffer != nullptr)
		{
			auto internal_state = to_internal(indexBuffer);

			res.BufferLocation = internal_state->gpu_address + (D3D12_GPU_VIRTUAL_ADDRESS)offset;
			res.Format = (format == IndexBufferFormat_UINT16 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT);
			res.SizeInBytes = (UINT)(indexBuffer->desc.size - offset);
		}
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->IASetIndexBuffer(&res);
	}

	void D3D12Device::BindStencilRef(uint32_t value, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.prev_stencilref != value)
		{
			commandlist.prev_stencilref = value;
			commandlist.GetGraphicsCommandList()->OMSetStencilRef(value);
		}
	}

	void D3D12Device::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		const float blendFactor[4] = { r, g, b, a };
		commandlist.GetGraphicsCommandList()->OMSetBlendFactor(blendFactor);
	}

	void D3D12Device::BindShadingRate(ShadingRate rate, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (CheckCapability(GraphicsDeviceCapability_VARIABLE_RATE_SHADING) && commandlist.prev_shadingrate != rate)
		{
			commandlist.prev_shadingrate = rate;

			D3D12_SHADING_RATE _rate = D3D12_SHADING_RATE_1X1;
			WriteShadingRateValue(rate, &_rate);

			D3D12_SHADING_RATE_COMBINER combiners[] =
			{
				D3D12_SHADING_RATE_COMBINER_MAX,
				D3D12_SHADING_RATE_COMBINER_MAX,
			};
			commandlist.GetGraphicsCommandListLatest()->RSSetShadingRate(_rate, combiners);
		}
	}

	void D3D12Device::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.active_cs = nullptr;
		commandlist.active_rt = nullptr;

		auto internal_state = to_internal(pso);
		if (internal_state->resource != nullptr)
		{
			if (commandlist.active_pso != pso)
			{
				commandlist.GetGraphicsCommandList()->SetPipelineState(internal_state->resource.Get());

				if (commandlist.prev_pt != internal_state->primitiveTopology)
				{
					commandlist.prev_pt = internal_state->primitiveTopology;

					commandlist.GetGraphicsCommandList()->IASetPrimitiveTopology(internal_state->primitiveTopology);
				}
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

		if (commandlist.active_rootsig_graphics != internal_state->rootSignature.Get())
		{
			commandlist.active_rootsig_graphics = internal_state->rootSignature.Get();
			commandlist.GetGraphicsCommandList()->SetGraphicsRootSignature(internal_state->rootSignature.Get());

			auto& binder = commandlist.binder;
			binder.optimizer_graphics = &internal_state->rootsig_optimizer;
			binder.dirty_graphics = internal_state->rootsig_optimizer.root_mask; // invalidates all root bindings
		}

		commandlist.active_pso = pso;
	}

	void D3D12Device::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.active_cs == cs)
			return;
		commandlist.active_pso = nullptr;
		commandlist.active_rt = nullptr;

		assert(cs->stage == ShaderStage_CS || cs->stage == ShaderStage_LIB);

		commandlist.prev_pipeline_hash = {};

		commandlist.active_cs = cs;

		auto internal_state = to_internal(cs);

		if (cs->stage == ShaderStage_CS)
		{
			commandlist.GetGraphicsCommandList()->SetPipelineState(internal_state->resource.Get());
		}

		if (commandlist.active_rootsig_compute != internal_state->rootSignature.Get())
		{
			commandlist.active_rootsig_compute = internal_state->rootSignature.Get();
			commandlist.GetGraphicsCommandList()->SetComputeRootSignature(internal_state->rootSignature.Get());

			auto& binder = commandlist.binder;
			binder.optimizer_compute = &internal_state->rootsig_optimizer;
			binder.dirty_compute = internal_state->rootsig_optimizer.root_mask; // invalidates all root bindings
		}
	}

	void D3D12Device::BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability_DEPTH_BOUNDS_TEST))
		{
			CommandList_DX12& commandlist = GetCommandList(cmd);
			commandlist.GetGraphicsCommandListLatest()->OMSetDepthBounds(min_bounds, max_bounds);
		}
	}

	void D3D12Device::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->DrawInstanced(vertexCount, 1, startVertexLocation, 0);
	}

	void D3D12Device::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}

	void D3D12Device::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->DrawInstanced(vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}

	void D3D12Device::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->DrawIndexedInstanced(indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}

	void D3D12Device::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(drawInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}

	void D3D12Device::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(drawIndexedInstancedIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}

	void D3D12Device::DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto pso_internal = to_internal(commandlist.active_pso);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(pso_internal->drawInstancedIndirectCountCommandSignature.Get(), max_count, args_internal->resource.Get(), args_offset, count_internal->resource.Get(), count_offset);
	}

	void D3D12Device::DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto pso_internal = to_internal(commandlist.active_pso);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(pso_internal->drawIndexedInstancedIndirectCountCommandSignature.Get(), max_count, args_internal->resource.Get(), args_offset, count_internal->resource.Get(), count_offset);
	}

	void D3D12Device::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->Dispatch(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void D3D12Device::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(dispatchIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}

	void D3D12Device::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandListLatest()->DispatchMesh(threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}

	void D3D12Device::DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(dispatchMeshIndirectCommandSignature.Get(), 1, internal_state->resource.Get(), args_offset, nullptr, 0);
	}

	void D3D12Device::DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto pso_internal = to_internal(commandlist.active_pso);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		commandlist.GetGraphicsCommandList()->ExecuteIndirect(pso_internal->dispatchMeshIndirectCountCommandSignature.Get(), max_count, args_internal->resource.Get(), args_offset, count_internal->resource.Get(), count_offset);
	}

	void D3D12Device::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);

		if (pDst->IsTexture() && pSrc->IsTexture())
		{
			const TextureDesc& src_desc = ((const Texture*)pSrc)->GetDesc();
			const TextureDesc& dst_desc = ((const Texture*)pDst)->GetDesc();

			if (src_desc.usage == Usage_UPLOAD && dst_desc.usage == Usage_DEFAULT)
			{
				// CPU (buffer) -> GPU (texture)
				const uint32_t planes = GetFormatPlaneCount(dst_desc.format); // dst plane count!
				for (uint32_t plane = 0; plane < planes; ++plane)
				{
					for (uint32_t layer = 0; layer < dst_desc.array_size; ++layer)
					{
						for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
						{
							const UINT subresource = D3D12CalcSubresource(mip, layer, plane, dst_desc.mip_levels, dst_desc.array_size);
							CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), internal_state_src->footprints[layer * dst_desc.mip_levels + mip]);
							CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), subresource);
							commandlist.GetGraphicsCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
						}
					}
				}
			}
			else if (src_desc.usage == Usage_DEFAULT && dst_desc.usage == Usage_READBACK)
			{
				// GPU (texture) -> CPU (buffer)
				const uint32_t planes = GetFormatPlaneCount(src_desc.format); // src plane count!
				for (uint32_t plane = 0; plane < planes; ++plane)
				{
					for (uint32_t layer = 0; layer < dst_desc.array_size; ++layer)
					{
						for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
						{
							const UINT subresource = D3D12CalcSubresource(mip, layer, 0, dst_desc.mip_levels, dst_desc.array_size);
							CD3DX12_TEXTURE_COPY_LOCATION Src(internal_state_src->resource.Get(), subresource);
							CD3DX12_TEXTURE_COPY_LOCATION Dst(internal_state_dst->resource.Get(), internal_state_dst->footprints[layer * dst_desc.mip_levels + mip]);
							commandlist.GetGraphicsCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
						}
					}
				}
			}
			else
			{
				commandlist.GetGraphicsCommandList()->CopyResource(internal_state_dst->resource.Get(), internal_state_src->resource.Get());
			}
		}
		else
		{
			commandlist.GetGraphicsCommandList()->CopyResource(internal_state_dst->resource.Get(), internal_state_src->resource.Get());
		}
	}

	void D3D12Device::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto src_internal = to_internal(pSrc);
		auto dst_internal = to_internal(pDst);

		commandlist.GetGraphicsCommandList()->CopyBufferRegion(dst_internal->resource.Get(), dst_offset, src_internal->resource.Get(), src_offset, size);
	}

	void D3D12Device::CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto src_internal = to_internal(src);
		auto dst_internal = to_internal(dst);

		const TextureDesc& src_desc = src->GetDesc();
		const TextureDesc& dst_desc = dst->GetDesc();

		const UINT srcPlane = GetPlaneSlice(src_aspect);
		const UINT dstPlane = GetPlaneSlice(dst_aspect);
		const UINT src_subresource = D3D12CalcSubresource(srcMip, srcSlice, srcPlane, src->desc.mip_levels, src->desc.array_size);
		const UINT dst_subresource = D3D12CalcSubresource(dstMip, dstSlice, dstPlane, dst->desc.mip_levels, dst->desc.array_size);

		CD3DX12_TEXTURE_COPY_LOCATION src_location;
		CD3DX12_TEXTURE_COPY_LOCATION dst_location;
		if (src_desc.usage == Usage_UPLOAD && dst_desc.usage == Usage_DEFAULT)
		{
			// CPU (buffer) -> GPU (texture)
			src_location = CD3DX12_TEXTURE_COPY_LOCATION(src_internal->resource.Get(), src_internal->footprints[src_subresource]);
			dst_location = CD3DX12_TEXTURE_COPY_LOCATION(dst_internal->resource.Get(), dst_subresource);
		}
		else if (src_desc.usage == Usage_DEFAULT && dst_desc.usage == Usage_READBACK)
		{
			// GPU (texture) -> CPU (buffer)
			src_location = CD3DX12_TEXTURE_COPY_LOCATION(src_internal->resource.Get(), src_subresource);
			dst_location = CD3DX12_TEXTURE_COPY_LOCATION(dst_internal->resource.Get(), dst_internal->footprints[dst_subresource]);
		}
		else if (src_desc.usage == Usage_DEFAULT && dst_desc.usage == Usage_DEFAULT)
		{
			// GPU (texture) -> GPU (texture)
			src_location = CD3DX12_TEXTURE_COPY_LOCATION(src_internal->resource.Get(), src_subresource);
			dst_location = CD3DX12_TEXTURE_COPY_LOCATION(dst_internal->resource.Get(), dst_subresource);
		}
		else
		{
			// CPU (buffer) -> CPU (buffer)
			src_location = CD3DX12_TEXTURE_COPY_LOCATION(src_internal->resource.Get(), src_internal->footprints[src_subresource]);
			dst_location = CD3DX12_TEXTURE_COPY_LOCATION(dst_internal->resource.Get(), dst_internal->footprints[dst_subresource]);
		}
		if (srcbox == nullptr)
		{
			commandlist.GetGraphicsCommandList()->CopyTextureRegion(
				&dst_location,
				dstX,
				dstY,
				dstZ,
				&src_location,
				nullptr
			);
		}
		else
		{
			D3D12_BOX d3dbox = {};
			d3dbox.left = srcbox->left;
			d3dbox.top = srcbox->top;
			d3dbox.front = srcbox->front;
			d3dbox.right = srcbox->right;
			d3dbox.bottom = srcbox->bottom;
			d3dbox.back = srcbox->back;
			commandlist.GetGraphicsCommandList()->CopyTextureRegion(
				&dst_location,
				dstX,
				dstY,
				dstZ,
				&src_location,
				&d3dbox
			);
		}

	}

	void D3D12Device::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType_TIMESTAMP:
			commandlist.GetGraphicsCommandList()->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index
			);
			break;
		case GpuQueryType_OCCLUSION_BINARY:
			commandlist.GetGraphicsCommandList()->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index
			);
			break;
		case GpuQueryType_OCCLUSION:
			commandlist.GetGraphicsCommandList()->BeginQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				index
			);
			break;
		}
	}

	void D3D12Device::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType_TIMESTAMP:
			commandlist.GetGraphicsCommandList()->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index
			);
			break;
		case GpuQueryType_OCCLUSION_BINARY:
			commandlist.GetGraphicsCommandList()->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index
			);
			break;
		case GpuQueryType_OCCLUSION:
			commandlist.GetGraphicsCommandList()->EndQuery(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				index
			);
			break;
		}
	}

	void D3D12Device::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);

		switch (heap->desc.type)
		{
		case GpuQueryType_TIMESTAMP:
			commandlist.GetGraphicsCommandList()->ResolveQueryData(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_TIMESTAMP,
				index,
				count,
				dst_internal->resource.Get(),
				dest_offset
			);
			break;
		case GpuQueryType_OCCLUSION_BINARY:
			commandlist.GetGraphicsCommandList()->ResolveQueryData(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_BINARY_OCCLUSION,
				index,
				count,
				dst_internal->resource.Get(),
				dest_offset
			);
			break;
		case GpuQueryType_OCCLUSION:
			commandlist.GetGraphicsCommandList()->ResolveQueryData(
				internal_state->heap.Get(),
				D3D12_QUERY_TYPE_OCCLUSION,
				index,
				count,
				dst_internal->resource.Get(),
				dest_offset
			);
			break;
		}
	}

	void D3D12Device::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);

		auto& barrierdescs = commandlist.frame_barriers;

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];

			if (barrier.type == GPUBarrier::Type_IMAGE && (barrier.image.texture == nullptr || !barrier.image.texture->IsValid()))
				continue;
			if (barrier.type == GPUBarrier::Type_BUFFER && (barrier.buffer.buffer == nullptr || !barrier.buffer.buffer->IsValid()))
				continue;

			D3D12_RESOURCE_BARRIER barrierdesc = {};

			switch (barrier.type)
			{
			default:
			case GPUBarrier::Type_MEMORY:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.UAV.pResource = barrier.memory.resource == nullptr ? nullptr : to_internal(barrier.memory.resource)->resource.Get();
			}
			break;
			case GPUBarrier::Type_IMAGE:
			{
				auto internal_state = to_internal(barrier.image.texture);
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = internal_state->resource.Get();
				barrierdesc.Transition.StateBefore = _ParseResourceState(barrier.image.layout_before);
				barrierdesc.Transition.StateAfter = _ParseResourceState(barrier.image.layout_after);
				if (barrier.image.mip >= 0 || barrier.image.slice >= 0 || barrier.image.aspect != nullptr)
				{
					const UINT PlaneSlice = barrier.image.aspect != nullptr ? _ImageAspectToPlane(*barrier.image.aspect) : 0;
					barrierdesc.Transition.Subresource = D3D12CalcSubresource(
						(UINT)std::max(0, barrier.image.mip),
						(UINT)std::max(0, barrier.image.slice),
						PlaneSlice,
						barrier.image.texture->desc.mip_levels,
						barrier.image.texture->desc.array_size
					);
				}
				else
				{
					barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				}

				if (barrier.image.layout_before == ResourceState_UNDEFINED)
				{
					CommandList_DX12::Discard& discard = commandlist.discards.emplace_back();
					discard.resource = internal_state->resource.Get();

					if (barrier.image.mip >= 0 || barrier.image.slice >= 0)
					{
						discard.region.FirstSubresource = D3D12CalcSubresource(
							(UINT)std::max(0, barrier.image.mip),
							(UINT)std::max(0, barrier.image.slice),
							0,
							barrier.image.texture->desc.mip_levels,
							barrier.image.texture->desc.array_size
						);
						discard.region.NumSubresources = 1;
						discard.region.NumRects = 0;
						discard.region.pRects = nullptr;
					}

					barrierdesc.Transition.StateBefore = _ParseResourceState(barrier.image.texture->desc.layout);
				}
			}
			break;
			case GPUBarrier::Type_BUFFER:
			{
				auto internal_state = to_internal(barrier.buffer.buffer);
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Transition.pResource = internal_state->resource.Get();
				barrierdesc.Transition.StateBefore = _ParseResourceState(barrier.buffer.state_before);
				barrierdesc.Transition.StateAfter = _ParseResourceState(barrier.buffer.state_after);
				barrierdesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

				if (barrier.buffer.state_before == ResourceState_UNDEFINED)
				{
					CommandList_DX12::Discard& discard = commandlist.discards.emplace_back();
					discard.resource = internal_state->resource.Get();
				}
			}
			break;
			case GPUBarrier::Type_ALIASING:
			{
				barrierdesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
				barrierdesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				barrierdesc.Aliasing.pResourceBefore = to_internal(barrier.aliasing.resource_before)->resource.Get();
				barrierdesc.Aliasing.pResourceAfter = to_internal(barrier.aliasing.resource_after)->resource.Get();
			}
			break;
			}

			if (commandlist.queue == QUEUE_COMPUTE && barrierdesc.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
			{
				// Compute queue can't transition pixel shader state:
				barrierdesc.Transition.StateBefore &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				barrierdesc.Transition.StateAfter &= ~D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
			else if (commandlist.queue == QUEUE_COPY && barrierdesc.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
			{
				// Copy queue can only transition from/to COPY_ and COMMON states, so we will overwrite anything else to COMMON,
				//	but the video textures will be using D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS so that implicit transition can happen from COMMON in a different queue
				if (barrierdesc.Transition.StateBefore != D3D12_RESOURCE_STATE_COPY_SOURCE && barrierdesc.Transition.StateBefore != D3D12_RESOURCE_STATE_COPY_DEST)
				{
					barrierdesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
				}
				if (barrierdesc.Transition.StateAfter != D3D12_RESOURCE_STATE_COPY_SOURCE && barrierdesc.Transition.StateAfter != D3D12_RESOURCE_STATE_COPY_DEST)
				{
					barrierdesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
				}
			}
			else if (commandlist.queue == QUEUE_VIDEO_DECODE && barrierdesc.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
			{
				// Video queue can only transition from/to VIDEO_ and COMMON states, so we will overwrite anything else to COMMON,
				//	but the video textures will be using D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS so that implicit transition can happen from COMMON in a different queue
				if (barrierdesc.Transition.StateBefore != D3D12_RESOURCE_STATE_VIDEO_DECODE_READ && barrierdesc.Transition.StateBefore != D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE)
				{
					barrierdesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
				}
				if (barrierdesc.Transition.StateAfter != D3D12_RESOURCE_STATE_VIDEO_DECODE_READ && barrierdesc.Transition.StateAfter != D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE)
				{
					barrierdesc.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
				}
			}

			if (barrierdesc.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION && barrierdesc.Transition.StateBefore == barrierdesc.Transition.StateAfter)
			{
				// skip nop barrier
				continue;
			}

			barrierdescs.push_back(barrierdesc);
		}

		if (!barrierdescs.empty())
		{
			if (commandlist.queue == QUEUE_VIDEO_DECODE)
			{
				commandlist.GetVideoDecodeCommandList()->ResourceBarrier(
					(UINT)barrierdescs.size(),
					barrierdescs.data()
				);
			}
			else
			{
				commandlist.GetGraphicsCommandList()->ResourceBarrier(
					(UINT)barrierdescs.size(),
					barrierdescs.data()
				);
			}
			barrierdescs.clear();
		}

		if (!commandlist.discards.empty() && commandlist.queue != QUEUE_VIDEO_DECODE)
		{
			for (auto& discard : commandlist.discards)
			{
				commandlist.GetGraphicsCommandList()->DiscardResource(discard.resource, discard.region.NumSubresources > 0 ? &discard.region : nullptr);
			}
			commandlist.discards.clear();
		}
	}

	void D3D12Device::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto dst_internal = to_internal(dst);

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
		desc.Inputs = dst_internal->desc;

		// Make a copy of geometries, don't overwrite internal_state (thread safety)
		commandlist.accelerationstructure_build_geometries = dst_internal->geometries;
		desc.Inputs.pGeometryDescs = commandlist.accelerationstructure_build_geometries.data();

		// The real GPU addresses get filled here:
		switch (dst->desc.type)
		{
		case RaytracingAccelerationStructureDesc::Type_BOTTOMLEVEL:
		{
			size_t i = 0;
			for (auto& x : dst->desc.bottom_level.geometries)
			{
				auto& geometry = commandlist.accelerationstructure_build_geometries[i++];
				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE)
				{
					geometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
				}
				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
				{
					geometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
				}

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_TRIANGLES)
				{
					geometry.Triangles.VertexBuffer.StartAddress = to_internal(&x.triangles.vertex_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.vertex_byte_offset;
					geometry.Triangles.IndexBuffer = to_internal(&x.triangles.index_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.index_offset * (x.triangles.index_format == IndexBufferFormat_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.Triangles.Transform3x4 = to_internal(&x.triangles.transform_3x4_buffer)->gpu_address +
							(D3D12_GPU_VIRTUAL_ADDRESS)x.triangles.transform_3x4_buffer_offset;
					}
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type_PROCEDURAL_AABBS)
				{
					geometry.AABBs.AABBs.StartAddress = to_internal(&x.aabbs.aabb_buffer)->gpu_address +
						(D3D12_GPU_VIRTUAL_ADDRESS)x.aabbs.offset;
				}
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::Type_TOPLEVEL:
		{
			desc.Inputs.InstanceDescs = to_internal(&dst->desc.top_level.instance_buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)dst->desc.top_level.offset;
		}
		break;
		}

		if (src != nullptr && (desc.Inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE))
		{
			desc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;

			auto src_internal = to_internal(src);
			desc.SourceAccelerationStructureData = src_internal->gpu_address;
		}
		desc.DestAccelerationStructureData = dst_internal->gpu_address;
		desc.ScratchAccelerationStructureData = to_internal(&dst_internal->scratch)->gpu_address;
		commandlist.GetGraphicsCommandListLatest()->BuildRaytracingAccelerationStructure(&desc, 0, nullptr);
	}

	void D3D12Device::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.active_cs = nullptr;
		commandlist.active_pso = nullptr;
		commandlist.prev_pipeline_hash = {};
		commandlist.active_rt = rtpso;

		BindComputeShader(rtpso->desc.shader_libraries.front().shader, cmd);

		auto internal_state = to_internal(rtpso);
		commandlist.GetGraphicsCommandListLatest()->SetPipelineState1(internal_state->resource.Get());
	}

	void D3D12Device::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		predispatch(cmd);

		D3D12_DISPATCH_RAYS_DESC dispatchrays_desc = {};

		dispatchrays_desc.Width = desc->width;
		dispatchrays_desc.Height = desc->height;
		dispatchrays_desc.Depth = desc->depth;

		if (desc->ray_generation.buffer != nullptr)
		{
			dispatchrays_desc.RayGenerationShaderRecord.StartAddress =
				to_internal(desc->ray_generation.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->ray_generation.offset;
			dispatchrays_desc.RayGenerationShaderRecord.SizeInBytes =
				desc->ray_generation.size;
		}

		if (desc->miss.buffer != nullptr)
		{
			dispatchrays_desc.MissShaderTable.StartAddress =
				to_internal(desc->miss.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->miss.offset;
			dispatchrays_desc.MissShaderTable.SizeInBytes =
				desc->miss.size;
			dispatchrays_desc.MissShaderTable.StrideInBytes =
				desc->miss.stride;
		}

		if (desc->hit_group.buffer != nullptr)
		{
			dispatchrays_desc.HitGroupTable.StartAddress =
				to_internal(desc->hit_group.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->hit_group.offset;
			dispatchrays_desc.HitGroupTable.SizeInBytes =
				desc->hit_group.size;
			dispatchrays_desc.HitGroupTable.StrideInBytes =
				desc->hit_group.stride;
		}

		if (desc->callable.buffer != nullptr)
		{
			dispatchrays_desc.CallableShaderTable.StartAddress =
				to_internal(desc->callable.buffer)->gpu_address +
				(D3D12_GPU_VIRTUAL_ADDRESS)desc->callable.offset;
			dispatchrays_desc.CallableShaderTable.SizeInBytes =
				desc->callable.size;
			dispatchrays_desc.CallableShaderTable.StrideInBytes =
				desc->callable.stride;
		}

		commandlist.GetGraphicsCommandListLatest()->DispatchRays(&dispatchrays_desc);
	}

	void D3D12Device::PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset)
	{
		assert(size % sizeof(uint32_t) == 0);
		assert(offset % sizeof(uint32_t) == 0);

		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto& binder = commandlist.binder;
		if (commandlist.active_pso != nullptr)
		{
			const RootSignatureOptimizer* optimizer = (const RootSignatureOptimizer*)binder.optimizer_graphics;
			const D3D12_ROOT_PARAMETER1& param = optimizer->rootsig_desc->Desc_1_1.pParameters[optimizer->PUSH];
			assert(param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
			assert(size <= param.Constants.Num32BitValues * sizeof(uint32_t)); // if this fires, not enough root constants were declared in root signature!
			commandlist.GetGraphicsCommandList()->SetGraphicsRoot32BitConstants(
				optimizer->PUSH,
				size / sizeof(uint32_t),
				data,
				offset / sizeof(uint32_t)
			);
			return;
		}
		if (commandlist.active_cs != nullptr)
		{
			const RootSignatureOptimizer* optimizer = (const RootSignatureOptimizer*)binder.optimizer_compute;
			const D3D12_ROOT_PARAMETER1& param = optimizer->rootsig_desc->Desc_1_1.pParameters[optimizer->PUSH];
			assert(param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS);
			assert(size <= param.Constants.Num32BitValues * sizeof(uint32_t)); // if this fires, not enough root constants were declared in root signature!
			commandlist.GetGraphicsCommandList()->SetComputeRoot32BitConstants(
				optimizer->PUSH,
				size / sizeof(uint32_t),
				data,
				offset / sizeof(uint32_t)
			);
			return;
		}
		assert(0); // there was no active pipeline!
	}

	void D3D12Device::PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(buffer);
		D3D12_PREDICATION_OP operation;
		switch (op)
		{
		default:
		case PredicationOp_EQUAL_ZERO:
			operation = D3D12_PREDICATION_OP_EQUAL_ZERO;
			break;
		case PredicationOp_NOT_EQUAL_ZERO:
			operation = D3D12_PREDICATION_OP_NOT_EQUAL_ZERO;
			break;
		}
		commandlist.GetGraphicsCommandList()->SetPredication(internal_state->resource.Get(), offset, operation);
	}

	void D3D12Device::PredicationEnd(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		commandlist.GetGraphicsCommandList()->SetPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
	}

	void D3D12Device::ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd)
	{
		const UINT values[4] = { value,value,value,value };

		auto internal_state = to_internal(resource);
		if (internal_state->uav_raw.IsValid())
		{
			// We cannot clear eg. a StructuredBuffer, so in those cases we must clear the RAW view with uav_raw
			const SingleDescriptor& descriptor = internal_state->uav_raw;
			D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = descriptorheap_res.start_gpu;
			gpu_handle.ptr += descriptor.index * resource_descriptor_size;
			D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = descriptor.handle;

			CommandList_DX12& commandlist = GetCommandList(cmd);
			commandlist.GetGraphicsCommandList()->ClearUnorderedAccessViewUint(
				gpu_handle,
				cpu_handle,
				internal_state->resource.Get(),
				values,
				0,
				nullptr
			);
		}
		else
		{
			if (internal_state->subresources_uav.empty())
			{
				const SingleDescriptor& descriptor = internal_state->uav;
				D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = descriptorheap_res.start_gpu;
				gpu_handle.ptr += descriptor.index * resource_descriptor_size;
				D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = descriptor.handle;

				CommandList_DX12& commandlist = GetCommandList(cmd);
				commandlist.GetGraphicsCommandList()->ClearUnorderedAccessViewUint(
					gpu_handle,
					cpu_handle,
					internal_state->resource.Get(),
					values,
					0,
					nullptr
				);
			}
			else
			{
				// This is clearing every subresource (for example every mip since they can't be referenced by single UAV)
				for (auto& uav : internal_state->subresources_uav)
				{
					const SingleDescriptor& descriptor = uav;
					D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = descriptorheap_res.start_gpu;
					gpu_handle.ptr += descriptor.index * resource_descriptor_size;
					D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = descriptor.handle;

					CommandList_DX12& commandlist = GetCommandList(cmd);
					commandlist.GetGraphicsCommandList()->ClearUnorderedAccessViewUint(
						gpu_handle,
						cpu_handle,
						internal_state->resource.Get(),
						values,
						0,
						nullptr
					);
				}
			}
		}
	}

	void D3D12Device::VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd)
	{
		auto decoder_internal = to_internal(video_decoder);
		auto stream_internal = to_internal(op->stream);
		auto dpb_internal = to_internal(op->DPB);
		D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS output = {};
		D3D12_VIDEO_DECODE_INPUT_STREAM_ARGUMENTS input = {};

		if (op->output == nullptr)
		{
			output.pOutputTexture2D = dpb_internal->resource.Get();
			output.OutputSubresource = D3D12CalcSubresource(0, op->current_dpb, 0, op->DPB->desc.mip_levels, op->DPB->desc.array_size);
		}
		else
		{
			auto output_internal = to_internal(op->output);
			output.pOutputTexture2D = output_internal->resource.Get();
			output.OutputSubresource = 0;
			output.ConversionArguments.Enable = TRUE;
			output.ConversionArguments.pReferenceTexture2D = dpb_internal->resource.Get();
			output.ConversionArguments.ReferenceSubresource = D3D12CalcSubresource(0, op->current_dpb, 0, op->DPB->desc.mip_levels, op->DPB->desc.array_size);
		}

		ID3D12Resource* reference_frames[17] = {};
		UINT reference_subresources[17] = {};
		const UINT reference_count = std::min<UINT>((UINT)op->DPB->desc.array_size, 17u);
		for (UINT i = 0; i < reference_count; ++i)
		{
			reference_frames[i] = dpb_internal->resource.Get();
			reference_subresources[i] = D3D12CalcSubresource(0, i, 0, op->DPB->desc.mip_levels, op->DPB->desc.array_size);
		}
		input.ReferenceFrames.NumTexture2Ds = reference_count;
		input.ReferenceFrames.ppTexture2Ds = reference_frames;
		input.ReferenceFrames.pSubresources = reference_subresources;

		input.CompressedBitstream.pBuffer = stream_internal->resource.Get();
		input.CompressedBitstream.Offset = op->stream_offset;
		input.CompressedBitstream.Size = op->stream_size;
		input.pHeap = decoder_internal->decoder_heap.Get();

		if (video_decoder->desc.profile == VideoProfile_H264)
		{
			const h264::SliceHeader* slice_header = (const h264::SliceHeader*)op->slice_header;
			const h264::PPS* pps = (const h264::PPS*)op->pps;
			const h264::SPS* sps = (const h264::SPS*)op->sps;

			DXVA_PicParams_H264 pic_params_h264 = {};
			DXVA_Qmatrix_H264 qmatrix_h264 = {};
			DXVA_Slice_H264_Short sliceinfo_h264 = {};

			// DirectX Video Acceleration for H.264/MPEG-4 AVC Decoding, Microsoft, Updated 2010, Page 21
			//	https://www.microsoft.com/en-us/download/details.aspx?id=11323
			//	Also: https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gallium/drivers/d3d12/d3d12_video_dec_h264.cpp
			//	Also: https://github.com/mofo7777/H264Dxva2Decoder
			pic_params_h264.wFrameWidthInMbsMinus1 = sps->pic_width_in_mbs_minus1;
			pic_params_h264.wFrameHeightInMbsMinus1 = sps->pic_height_in_map_units_minus1;
			pic_params_h264.IntraPicFlag = op->frame_type == VideoFrameType_Intra ? 1 : 0;
			pic_params_h264.MbaffFrameFlag = 0 /*sps->mb_adaptive_frame_field_flag && !slice_header->field_pic_flag*/;
			pic_params_h264.field_pic_flag = 0 /*slice_header->field_pic_flag*/; // 0 = full frame (top and bottom field)
			//pic_params_h264.bottom_field_flag = 0; // missing??
			pic_params_h264.chroma_format_idc = 1; // sps->chroma_format_idc; // only 1 is supported (YUV420)
			pic_params_h264.bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
			assert(pic_params_h264.bit_depth_chroma_minus8 == 0);   // Only support for NV12 now
			pic_params_h264.bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
			assert(pic_params_h264.bit_depth_luma_minus8 == 0);   // Only support for NV12 now
			pic_params_h264.residual_colour_transform_flag = sps->separate_colour_plane_flag; // https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gallium/drivers/d3d12/d3d12_video_dec_h264.cpp#L328
			if (pic_params_h264.field_pic_flag)
			{
				pic_params_h264.CurrPic.AssociatedFlag = slice_header->bottom_field_flag ? 1 : 0; // if pic_params_h264.field_pic_flag == 1, then this is 0 = top field, 1 = bottom_field
			}
			else
			{
				pic_params_h264.CurrPic.AssociatedFlag = 0;
			}
			pic_params_h264.CurrPic.Index7Bits = (UCHAR)op->current_dpb;
			pic_params_h264.CurrFieldOrderCnt[0] = op->poc[0];
			pic_params_h264.CurrFieldOrderCnt[1] = op->poc[1];
			for (uint32_t i = 0; i < 16; ++i)
			{
				pic_params_h264.RefFrameList[i].bPicEntry = 0xFF;
				pic_params_h264.FieldOrderCntList[i][0] = 0;
				pic_params_h264.FieldOrderCntList[i][1] = 0;
				pic_params_h264.FrameNumList[i] = 0;
			}
			for (size_t i = 0; i < op->dpb_reference_count && i < 16; ++i)
			{
				uint32_t ref_slot = op->dpb_reference_slots[i];
				assert(ref_slot != op->current_dpb);
				pic_params_h264.RefFrameList[i].AssociatedFlag = 0; // 0 = short term, 1 = long term reference
				pic_params_h264.RefFrameList[i].Index7Bits = (UCHAR)ref_slot;
				pic_params_h264.FieldOrderCntList[i][0] = op->dpb_poc[ref_slot];
				pic_params_h264.FieldOrderCntList[i][1] = op->dpb_poc[ref_slot];
				pic_params_h264.UsedForReferenceFlags |= 1 << (i * 2 + 0);
				pic_params_h264.UsedForReferenceFlags |= 1 << (i * 2 + 1);
				pic_params_h264.FrameNumList[i] = op->dpb_framenum[ref_slot];
			}
			pic_params_h264.weighted_pred_flag = pps->weighted_pred_flag;
			pic_params_h264.weighted_bipred_idc = pps->weighted_bipred_idc;
			pic_params_h264.sp_for_switch_flag = 0;

			pic_params_h264.transform_8x8_mode_flag = pps->transform_8x8_mode_flag;
			pic_params_h264.constrained_intra_pred_flag = pps->constrained_intra_pred_flag;
			pic_params_h264.num_ref_frames = sps->num_ref_frames;
			pic_params_h264.MbsConsecutiveFlag = 1; // The value shall be 1 unless the restricted-mode profile in use explicitly supports the value 0.
			pic_params_h264.frame_mbs_only_flag = sps->frame_mbs_only_flag;
			pic_params_h264.MinLumaBipredSize8x8Flag = sps->level_idc >= 31;
			pic_params_h264.RefPicFlag = op->reference_priority > 0 ? 1 : 0;
			pic_params_h264.frame_num = slice_header->frame_num;
			pic_params_h264.pic_init_qp_minus26 = pps->pic_init_qp_minus26;
			pic_params_h264.pic_init_qs_minus26 = pps->pic_init_qs_minus26;
			pic_params_h264.chroma_qp_index_offset = pps->chroma_qp_index_offset;
			pic_params_h264.second_chroma_qp_index_offset = pps->second_chroma_qp_index_offset;
			pic_params_h264.log2_max_frame_num_minus4 = sps->log2_max_frame_num_minus4;
			pic_params_h264.pic_order_cnt_type = sps->pic_order_cnt_type;
			pic_params_h264.log2_max_pic_order_cnt_lsb_minus4 = sps->log2_max_pic_order_cnt_lsb_minus4;
			pic_params_h264.delta_pic_order_always_zero_flag = sps->delta_pic_order_always_zero_flag;
			pic_params_h264.direct_8x8_inference_flag = sps->direct_8x8_inference_flag;
			pic_params_h264.entropy_coding_mode_flag = pps->entropy_coding_mode_flag;
			pic_params_h264.pic_order_present_flag = pps->pic_order_present_flag;
			pic_params_h264.num_slice_groups_minus1 = pps->num_slice_groups_minus1;
			assert(pic_params_h264.num_slice_groups_minus1 == 0);   // FMO Not supported by VA
			pic_params_h264.slice_group_map_type = pps->slice_group_map_type;
			pic_params_h264.deblocking_filter_control_present_flag = pps->deblocking_filter_control_present_flag;
			pic_params_h264.redundant_pic_cnt_present_flag = pps->redundant_pic_cnt_present_flag;
			pic_params_h264.slice_group_change_rate_minus1 = pps->slice_group_change_rate_minus1;
			pic_params_h264.Reserved16Bits = 3; // DXVA spec
			pic_params_h264.StatusReportFeedbackNumber = (UINT)op->decoded_frame_index + 1; // shall not be 0
			assert(pic_params_h264.StatusReportFeedbackNumber > 0);
			pic_params_h264.ContinuationFlag = 1;
			int ref_idx_l0 = pps->num_ref_idx_l0_active_minus1;
			int ref_idx_l1 = pps->num_ref_idx_l1_active_minus1;
			if (slice_header->num_ref_idx_active_override_flag)
			{
				ref_idx_l0 = slice_header->num_ref_idx_l0_active_minus1;
				if (h264::is_slice_type(slice_header->slice_type, h264::SH_SLICE_TYPE_B))
					ref_idx_l1 = slice_header->num_ref_idx_l1_active_minus1;
			}
			pic_params_h264.num_ref_idx_l0_active_minus1 = ref_idx_l0;
			pic_params_h264.num_ref_idx_l1_active_minus1 = ref_idx_l1;
			input.FrameArguments[input.NumFrameArguments].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS;
			input.FrameArguments[input.NumFrameArguments].Size = sizeof(pic_params_h264);
			input.FrameArguments[input.NumFrameArguments].pData = &pic_params_h264;
			input.NumFrameArguments++;

			// DirectX Video Acceleration for H.264/MPEG-4 AVC Decoding, Microsoft, Updated 2010, Page 29
			//	Also: https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gallium/drivers/d3d12/d3d12_video_dec_h264.cpp#L548
			if (sps->seq_scaling_matrix_present_flag)
			{
				static constexpr int vl_zscan_normal_16[] =
				{
					/* Zig-Zag scan pattern */
					0, 1, 4, 8, 5, 2, 3, 6,
					9, 12, 13, 10, 7, 11, 14, 15
				};
				static constexpr int vl_zscan_normal[] =
				{
					/* Zig-Zag scan pattern */
					 0, 1, 8,16, 9, 2, 3,10,
					17,24,32,25,18,11, 4, 5,
					12,19,26,33,40,48,41,34,
					27,20,13, 6, 7,14,21,28,
					35,42,49,56,57,50,43,36,
					29,22,15,23,30,37,44,51,
					58,59,52,45,38,31,39,46,
					53,60,61,54,47,55,62,63
				};
				for (int i = 0; i < 6; ++i)
				{
					for (int j = 0; j < 16; ++j)
					{
						qmatrix_h264.bScalingLists4x4[i][j] = pps->ScalingList4x4[i][vl_zscan_normal_16[j]];
					}
				}
				for (int i = 0; i < 64; ++i)
				{
					qmatrix_h264.bScalingLists8x8[0][i] = pps->ScalingList8x8[0][vl_zscan_normal[i]];
					qmatrix_h264.bScalingLists8x8[1][i] = pps->ScalingList8x8[1][vl_zscan_normal[i]];
				}
			}
			else
			{
				// I don't know why it needs to be filled with 16, but otherwise it gets corrupted output
				//	Source: https://github.com/mofo7777/H264Dxva2Decoder
				std::memset(&qmatrix_h264, 16, sizeof(DXVA_Qmatrix_H264));
			}
			input.FrameArguments[input.NumFrameArguments].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_INVERSE_QUANTIZATION_MATRIX;
			input.FrameArguments[input.NumFrameArguments].Size = sizeof(qmatrix_h264);
			input.FrameArguments[input.NumFrameArguments].pData = &qmatrix_h264;
			input.NumFrameArguments++;

			// DirectX Video Acceleration for H.264/MPEG-4 AVC Decoding, Microsoft, Updated 2010, Page 31
			sliceinfo_h264.BSNALunitDataLocation = 0;
			sliceinfo_h264.SliceBytesInBuffer = (UINT)op->stream_size;
			sliceinfo_h264.wBadSliceChopping = 0; // whole slice is in the buffer
			input.FrameArguments[input.NumFrameArguments].Type = D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL;
			input.FrameArguments[input.NumFrameArguments].Size = sizeof(sliceinfo_h264);
			input.FrameArguments[input.NumFrameArguments].pData = &sliceinfo_h264;
			input.NumFrameArguments++;

			CommandList_DX12& commandlist = GetCommandList(cmd);
			commandlist.GetVideoDecodeCommandList()->DecodeFrame(
				decoder_internal->decoder.Get(),
				&output,
				&input
			);
		}
		else if (video_decoder->desc.profile == VideoProfile_H265)
		{
			assert(0); // TODO
		}
	}

	void D3D12Device::EventBegin(const char* name, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.queue == QUEUE_VIDEO_DECODE)
			return;
		wchar_t text[128];
		if (Protean::Helper::StringConvert(name, text, arraysize(text)) > 0)
		{
			PIXBeginEvent(commandlist.GetGraphicsCommandList(), 0xFF000000, text);
		}
	}

	void D3D12Device::EventEnd(CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.queue == QUEUE_VIDEO_DECODE)
			return;
		PIXEndEvent(commandlist.GetGraphicsCommandList());
	}

	void D3D12Device::SetMarker(const char* name, CommandList cmd)
	{
		CommandList_DX12& commandlist = GetCommandList(cmd);
		if (commandlist.queue == QUEUE_VIDEO_DECODE)
			return;
		wchar_t text[128];
		if (Protean::Helper::StringConvert(name, text, arraysize(text)) > 0)
		{
			PIXSetMarker(commandlist.GetGraphicsCommandList(), 0xFFFF0000, text);
		}
	}
}

#endif // WICKEDENGINE_BUILD_DX12
