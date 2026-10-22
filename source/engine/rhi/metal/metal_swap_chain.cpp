#include "rhi/metal/metal_internal.h"
// Metal backend: Swap chain creation and presentation surfaces

#ifdef __APPLE__

namespace Protean::Graphics
{
	using namespace MetalInternal;

	bool MetalDevice::CreateSwapChain(const SwapChainDesc* desc, Protean::Platform::window_type window, SwapChain* swapchain) const
	{
		auto internal_state = swapchain->IsValid() ? Protean::Allocator::shared_ptr<SwapChain_Metal>(swapchain->internal_state) : Protean::Allocator::make_shared<SwapChain_Metal>();
		internal_state->allocationhandler = allocationhandler;
		swapchain->internal_state = internal_state;
		swapchain->desc = *desc;
		
		swapchain->desc.buffer_count = clamp(desc->buffer_count, 2u, 3u);
		
		if (internal_state->layer == nullptr)
		{
			internal_state->layer = CA::MetalLayer::layer();
			internal_state->layer->setDevice(device.get());
			internal_state->layer->setMaximumDrawableCount(swapchain->desc.buffer_count);
			internal_state->layer->setFramebufferOnly(false); // GetBackBuffer() srv
			Protean::Apple::SetMetalLayerToWindow(window, internal_state->layer);
		}
		
		CGSize size = {(CGFloat)desc->width, (CGFloat)desc->height};
		internal_state->layer->setDrawableSize(size);
		internal_state->layer->setDisplaySyncEnabled(desc->vsync);
		internal_state->layer->setPixelFormat(_ConvertPixelFormat(desc->format));

		return internal_state->layer != nullptr;
	}

	Texture MetalDevice::GetBackBuffer(const SwapChain* swapchain) const
	{
		auto swapchain_internal = to_internal(swapchain);
		Texture result;
		result.type = GPUResource::Type_TEXTURE;
		auto texture_internal = Protean::Allocator::make_shared<Texture_Metal>();
		texture_internal->texture = swapchain_internal->current_texture;
		texture_internal->srv.index = allocationhandler->allocate_resource_index();
		texture_internal->srv.entry = create_entry(texture_internal->texture.get());
		std::memcpy(descriptor_heap_res_data + texture_internal->srv.index, &texture_internal->srv.entry, sizeof(texture_internal->srv.entry));
		result.internal_state = texture_internal;
		result.desc.width = swapchain->desc.width;
		result.desc.height = swapchain->desc.height;
		result.desc.bind_flags = BindFlag_SHADER_RESOURCE | BindFlag_RENDER_TARGET;
		result.desc.format = swapchain->desc.format;
		return result;
	}

	ColorSpace MetalDevice::GetSwapChainColorSpace(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);
		return internal_state->colorSpace;
	}

	bool MetalDevice::IsSwapChainSupportsHDR(const SwapChain* swapchain) const
	{
		// TODO
		return false;
	}
}

#endif // __APPLE__
