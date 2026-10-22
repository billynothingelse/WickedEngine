#include "render/GraphicsAPI/Vulkan/VulkanInternal.h"
// Vulkan backend: Swap chain creation and presentation surfaces

#ifdef WICKEDENGINE_BUILD_VULKAN

namespace Protean::Graphics
{
	using namespace VulkanInternal;

	bool VulkanDevice::CreateSwapChain(const SwapChainDesc* desc, Protean::Platform::window_type window, SwapChain* swapchain) const
	{
		auto internal_state = swapchain->IsValid() ? Protean::Allocator::shared_ptr<SwapChain_Vulkan>(swapchain->internal_state) : Protean::Allocator::make_shared<SwapChain_Vulkan>();
		internal_state->allocationhandler = allocationhandler;
		internal_state->desc = *desc;
		swapchain->internal_state = internal_state;
		swapchain->desc = *desc;

		// Surface creation:
		if(internal_state->surface == VK_NULL_HANDLE)
		{
#ifdef _WIN32
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hwnd = window;
			createInfo.hinstance = GetModuleHandle(nullptr);

			vulkan_check(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &internal_state->surface));
#elif defined(SDL2)
			if (!SDL_Vulkan_CreateSurface(window, instance, &internal_state->surface))
			{
				wilog_messagebox("Error creating a vulkan surface with SDL_Vulkan_CreateSurface!");
				Protean::Platform::Exit();
			}
#elif defined(SDL3)
			if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &internal_state->surface))
			{
				wilog_messagebox("Error creating a vulkan surface with SDL_Vulkan_CreateSurface!");
				Protean::Platform::Exit();
			}
#elif defined(__APPLE__)
			wilog("Vulkan surface on Apple platform is not yet implemented");
#else
#error WICKEDENGINE VULKAN DEVICE ERROR: PLATFORM NOT SUPPORTED
#endif // _WIN32
		}

		uint32_t presentFamily = VK_QUEUE_FAMILY_IGNORED;
		uint32_t familyIndex = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			VkBool32 presentSupport = false;
			vulkan_check(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, (uint32_t)familyIndex, internal_state->surface, &presentSupport));

			if (presentFamily == VK_QUEUE_FAMILY_IGNORED && queueFamily.queueFamilyProperties.queueCount > 0 && presentSupport)
			{
				presentFamily = familyIndex;
				break;
			}

			familyIndex++;
		}

		// Present family not found, we cannot create SwapChain
		if (presentFamily == VK_QUEUE_FAMILY_IGNORED)
		{
			return false;
		}

		bool success = CreateSwapChainInternal(internal_state.get(), physicalDevice, device, allocationhandler);
		assert(success);

		// The requested swapchain format is overridden if it wasn't supported:
		swapchain->desc.format = internal_state->desc.format;

		return success;
	}

	Texture VulkanDevice::GetBackBuffer(const SwapChain* swapchain) const
	{
		auto swapchain_internal = to_internal(swapchain);

		auto internal_state = swapchain_internal->textures[swapchain_internal->swapChainImageIndex];

		Texture result;
		result.type = GPUResource::Type_TEXTURE;
		result.internal_state = internal_state;
		result.desc.type = TextureDesc::Type_TEXTURE_2D;
		result.desc.width = swapchain_internal->swapChainExtent.width;
		result.desc.height = swapchain_internal->swapChainExtent.height;
		result.desc.format = swapchain->desc.format;
		result.desc.layout = ResourceState_SWAPCHAIN;
		result.desc.bind_flags = BindFlag_SHADER_RESOURCE | BindFlag_RENDER_TARGET;
		return result;
	}

	ColorSpace VulkanDevice::GetSwapChainColorSpace(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);
		return internal_state->colorSpace;
	}

	bool VulkanDevice::IsSwapChainSupportsHDR(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);

		uint32_t formatCount;
		VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, nullptr);
		if (res == VK_SUCCESS)
		{
			Protean::vector<VkSurfaceFormatKHR> swapchain_formats(formatCount);
			res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, swapchain_formats.data());
			if (res == VK_SUCCESS)
			{
				for (const auto& format : swapchain_formats)
				{
					if (format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						return true;
					}
				}
			}
		}
		return false;
	}
}

#endif // WICKEDENGINE_BUILD_VULKAN
