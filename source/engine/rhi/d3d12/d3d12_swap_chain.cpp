#include "rhi/d3d12/d3d12_internal.h"
// D3D12 backend: Swap chain creation and presentation surfaces

#ifdef WICKEDENGINE_BUILD_DX12

namespace Protean::Graphics
{
	using namespace D3D12Internal;

	bool D3D12Device::CreateSwapChain(const SwapChainDesc* desc, Protean::Platform::window_type window, SwapChain* swapchain) const
	{
		auto internal_state = swapchain->IsValid() ? Protean::Allocator::shared_ptr<SwapChain_DX12>(swapchain->internal_state) : Protean::Allocator::make_shared<SwapChain_DX12>();
		internal_state->allocationhandler = allocationhandler;
		swapchain->internal_state = internal_state;
		swapchain->desc = *desc;
		HRESULT hr = E_FAIL;

		if (!internal_state->textures.empty())
		{
			// Delete back buffer resources if they exist before resizing swap chain:
			WaitForGPU();
			for (auto& x : internal_state->textures)
			{
				x->resource.Reset(); // forced immediate reset
			}
			internal_state->textures.clear();
		}

		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = _ConvertFormat(desc->format);
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Format = rtv_desc.Format;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;

#ifdef PLATFORM_XBOX

		D3D12_HEAP_PROPERTIES heap_properties = {};
		heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

		D3D12_RESOURCE_DESC resource_desc = {};
		resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resource_desc.Format = rtv_desc.Format;
		resource_desc.Width = swapchain->desc.width;
		resource_desc.Height = swapchain->desc.height;
		resource_desc.DepthOrArraySize = 1;
		resource_desc.MipLevels = 1;
		resource_desc.SampleDesc.Count = 1;
		resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

		D3D12_HEAP_FLAGS heap_flags = D3D12_HEAP_FLAG_ALLOW_DISPLAY;
		Protean::Graphics::Xbox::ApplyTextureCreationFlags({}, resource_desc.Flags, heap_flags);

		D3D12_CLEAR_VALUE clear_value = {};
		clear_value.Format = resource_desc.Format;
		clear_value.Color[0] = swapchain->desc.clear_color[0];
		clear_value.Color[1] = swapchain->desc.clear_color[1];
		clear_value.Color[2] = swapchain->desc.clear_color[2];
		clear_value.Color[3] = swapchain->desc.clear_color[3];

		internal_state->textures.resize(swapchain->desc.buffer_count);
		for (uint32_t i = 0; i < swapchain->desc.buffer_count; ++i)
		{
			internal_state->textures[i] = Protean::Allocator::make_shared<Resource_DX12>();
			dx12_check(device->CreateCommittedResource(
				&heap_properties,
				heap_flags,
				&resource_desc,
				D3D12_RESOURCE_STATE_PRESENT,
				&clear_value,
				PPV_ARGS(internal_state->textures[i]->resource)
			));

			dx12_check(internal_state->textures[i]->resource->SetName(L"BackBufferXBOX"));

			internal_state->textures[i]->rtv.init(this, rtv_desc, internal_state->textures[i]->resource.Get());
			internal_state->textures[i]->srv.init(this, srv_desc, internal_state->textures[i]->resource.Get());
		}

#else
		UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		if (tearingSupported)
		{
			swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
		}

		if (internal_state->swapChain == nullptr)
		{
			// Create swapchain:
			ComPtr<IDXGISwapChain1> tempSwapChain;

			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
			swapChainDesc.Width = desc->width;
			swapChainDesc.Height = desc->height;
			swapChainDesc.Format = rtv_desc.Format;
			swapChainDesc.Stereo = false;
			swapChainDesc.SampleDesc.Count = 1;
			swapChainDesc.SampleDesc.Quality = 0;
			swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
			swapChainDesc.BufferCount = desc->buffer_count;
			swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			swapChainDesc.Flags = swapChainFlags;

#ifdef PLATFORM_WINDOWS_DESKTOP
			swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

			DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
			fullscreenDesc.Windowed = !desc->fullscreen;

			hr = dxgiFactory->CreateSwapChainForHwnd(
				queues[QUEUE_GRAPHICS].queue.Get(),
				window,
				&swapChainDesc,
				&fullscreenDesc,
				nullptr,
				tempSwapChain.GetAddressOf()
			);
#else
			swapChainDesc.Scaling = DXGI_SCALING_ASPECT_RATIO_STRETCH;

			hr = dxgiFactory->CreateSwapChainForCoreWindow(
				queues[QUEUE_GRAPHICS].queue.Get(),
				static_cast<IUnknown*>(winrt::get_abi(*window)),
				&swapChainDesc,
				nullptr,
				tempSwapChain.GetAddressOf()
			);
#endif // PLATFORM_WINDOWS_DESKTOP

			if (FAILED(hr))
			{
				return false;
			}

			hr = tempSwapChain.As(&internal_state->swapChain);
			if (FAILED(hr))
			{
				return false;
			}
		}
		else
		{
			// Resize swapchain:
			dx12_check(internal_state->swapChain->ResizeBuffers(
				desc->buffer_count,
				desc->width,
				desc->height,
				rtv_desc.Format,
				swapChainFlags
			));

		}

		const bool hdr = desc->allow_hdr && IsSwapChainSupportsHDR(swapchain);

		// Ensure correct color space:
		//	https://github.com/microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12HDR/src/D3D12HDR.cpp
		{
			internal_state->colorSpace = ColorSpace_SRGB; // reset to SDR, in case anything below fails to set HDR state
			DXGI_COLOR_SPACE_TYPE colorSpace = {};

			switch (desc->format)
			{
			case Format_R10G10B10A2_UNORM:
				// This format is either HDR10 (ST.2084), or SDR (SRGB)
				colorSpace = hdr ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
				break;
			case Format_R16G16B16A16_FLOAT:
				// This format is HDR (Linear):
				colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
				break;
			default:
				// Anything else will be SDR (SRGB):
				colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
				break;
			}

			UINT colorSpaceSupport = 0;
			if (SUCCEEDED(internal_state->swapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)))
			{
				if (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
				{
					hr = dx12_check(internal_state->swapChain->SetColorSpace1(colorSpace));
					if (SUCCEEDED(hr))
					{
						switch (colorSpace)
						{
						default:
						case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
							internal_state->colorSpace = ColorSpace_SRGB;
							break;
						case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
							internal_state->colorSpace = ColorSpace_HDR_LINEAR;
							break;
						case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
							internal_state->colorSpace = ColorSpace_HDR10_ST2084;
							break;
						}
					}
				}
			}
		}

		internal_state->textures.resize(desc->buffer_count);

		for (uint32_t i = 0; i < desc->buffer_count; ++i)
		{
			internal_state->textures[i] = Protean::Allocator::make_shared<Resource_DX12>();
			internal_state->textures[i]->allocationhandler = allocationhandler;
			dx12_check(internal_state->swapChain->GetBuffer(i, PPV_ARGS(internal_state->textures[i]->resource)));
			internal_state->textures[i]->rtv.init(this, rtv_desc, internal_state->textures[i]->resource.Get());
			internal_state->textures[i]->srv.init(this, srv_desc, internal_state->textures[i]->resource.Get());
		}
#endif // PLATFORM_XBOX

		internal_state->dummyTexture.desc.format = desc->format;
		internal_state->dummyTexture.desc.width = desc->width;
		internal_state->dummyTexture.desc.height = desc->height;
		return true;
	}

	Texture D3D12Device::GetBackBuffer(const SwapChain* swapchain) const
	{
		auto swapchain_internal = to_internal(swapchain);

		auto internal_state = swapchain_internal->textures[swapchain_internal->GetBufferIndex()];

		D3D12_RESOURCE_DESC resourcedesc = internal_state->resource->GetDesc();
		internal_state->total_size = 0;
		internal_state->footprints.resize(resourcedesc.DepthOrArraySize * resourcedesc.MipLevels);
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

		Texture result;
		result.type = GPUResource::Type_TEXTURE;
		result.internal_state = internal_state;
		result.desc = _ConvertTextureDesc_Inv(resourcedesc);
		result.desc.bind_flags = BindFlag_SHADER_RESOURCE | BindFlag_RENDER_TARGET;
		result.desc.layout = ResourceState_SWAPCHAIN;
		return result;
	}

	ColorSpace D3D12Device::GetSwapChainColorSpace(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);
		return internal_state->colorSpace;
	}

	bool D3D12Device::IsSwapChainSupportsHDR(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);

#ifdef PLATFORM_XBOX
		// TODO
#else
		// HDR display query: https://docs.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range
		ComPtr<IDXGIOutput> dxgiOutput;
		if (SUCCEEDED(internal_state->swapChain->GetContainingOutput(&dxgiOutput)))
		{
			ComPtr<IDXGIOutput6> output6;
			if (SUCCEEDED(dxgiOutput.As(&output6)))
			{
				DXGI_OUTPUT_DESC1 desc1;
				if (SUCCEEDED(output6->GetDesc1(&desc1)))
				{
					if (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
					{
						return true;
					}
				}
			}
		}
#endif // PLATFORM_XBOX
		return false;
	}
}

#endif // WICKEDENGINE_BUILD_DX12
