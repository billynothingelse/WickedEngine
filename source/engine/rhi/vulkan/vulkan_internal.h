#pragma once
// Internal header shared by the Vulkan backend translation units.
// Not part of the public graphics API, include rhi/rhi.h instead.
#include "rhi/vulkan/vulkan_device.h"

// pipeline state cache can be enabled here for Vulkan, but now it seems slower than not using it, so it's disabled by default:
//#define VULKAN_PIPELINE_CACHE_ENABLED

#ifdef WICKEDENGINE_BUILD_VULKAN
#include "core/version.h"
#include "core/timer.h"
#include "core/unordered_set.h"

#include "spirv_reflect/spirv_reflect.h"
#include "vulkan/vk_video/vulkan_video_codec_h264std_decode.h"
#include "codecs/h264.h"

#include "volk/volk.h"

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "vma/vk_mem_alloc.h"

#ifdef SDL2
#include <SDL2/SDL_vulkan.h>
#include "platform/sdl2.h"
#elif defined(SDL3)
#include <SDL3/SDL_vulkan.h>
#include "platform/sdl3.h"
#endif

#include <string>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace Protean::Graphics
{

namespace VulkanInternal
{
	// VmaAllocation_T is only complete in the translation unit that compiles the VMA implementation
	// (VulkanDevice.cpp), other backend files access allocations through these:
	VkDeviceMemory GetAllocationMemory(VmaAllocation allocation);
	VkDeviceSize GetAllocationOffset(VmaAllocation allocation);
	void* GetAllocationMappedData(VmaAllocation allocation);
	VkDeviceSize GetAllocationSize(VmaAllocation allocation);

	static constexpr uint64_t timeout_value = 3'000'000'000ull; // 3 seconds
	static constexpr uint64_t dynamic_cbv_maxsize = 64 * 1024;

	// Converters:
	constexpr VkFormat _ConvertFormat(Format value)
	{
		switch (value)
		{
		case Format_UNKNOWN:
			return VK_FORMAT_UNDEFINED;
		case Format_R32G32B32A32_FLOAT:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case Format_R32G32B32A32_UINT:
			return VK_FORMAT_R32G32B32A32_UINT;
		case Format_R32G32B32A32_SINT:
			return VK_FORMAT_R32G32B32A32_SINT;
		case Format_R32G32B32_FLOAT:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case Format_R32G32B32_UINT:
			return VK_FORMAT_R32G32B32_UINT;
		case Format_R32G32B32_SINT:
			return VK_FORMAT_R32G32B32_SINT;
		case Format_R16G16B16A16_FLOAT:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Format_R16G16B16A16_UNORM:
			return VK_FORMAT_R16G16B16A16_UNORM;
		case Format_R16G16B16A16_UINT:
			return VK_FORMAT_R16G16B16A16_UINT;
		case Format_R16G16B16A16_SNORM:
			return VK_FORMAT_R16G16B16A16_SNORM;
		case Format_R16G16B16A16_SINT:
			return VK_FORMAT_R16G16B16A16_SINT;
		case Format_R32G32_FLOAT:
			return VK_FORMAT_R32G32_SFLOAT;
		case Format_R32G32_UINT:
			return VK_FORMAT_R32G32_UINT;
		case Format_R32G32_SINT:
			return VK_FORMAT_R32G32_SINT;
		case Format_D32_FLOAT_S8X24_UINT:
			return VK_FORMAT_D32_SFLOAT_S8_UINT;
		case Format_R10G10B10A2_UNORM:
			return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		case Format_R10G10B10A2_UINT:
			return VK_FORMAT_A2B10G10R10_UINT_PACK32;
		case Format_R11G11B10_FLOAT:
			return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
		case Format_R8G8B8A8_UNORM:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case Format_R8G8B8A8_UNORM_SRGB:
			return VK_FORMAT_R8G8B8A8_SRGB;
		case Format_R8G8B8A8_UINT:
			return VK_FORMAT_R8G8B8A8_UINT;
		case Format_R8G8B8A8_SNORM:
			return VK_FORMAT_R8G8B8A8_SNORM;
		case Format_R8G8B8A8_SINT:
			return VK_FORMAT_R8G8B8A8_SINT;
		case Format_R16G16_FLOAT:
			return VK_FORMAT_R16G16_SFLOAT;
		case Format_R16G16_UNORM:
			return VK_FORMAT_R16G16_UNORM;
		case Format_R16G16_UINT:
			return VK_FORMAT_R16G16_UINT;
		case Format_R16G16_SNORM:
			return VK_FORMAT_R16G16_SNORM;
		case Format_R16G16_SINT:
			return VK_FORMAT_R16G16_SINT;
		case Format_D32_FLOAT:
			return VK_FORMAT_D32_SFLOAT;
		case Format_R32_FLOAT:
			return VK_FORMAT_R32_SFLOAT;
		case Format_R32_UINT:
			return VK_FORMAT_R32_UINT;
		case Format_R32_SINT:
			return VK_FORMAT_R32_SINT;
		case Format_D24_UNORM_S8_UINT:
			return VK_FORMAT_D24_UNORM_S8_UINT;
		case Format_R9G9B9E5_SHAREDEXP:
			return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
		case Format_R8G8_UNORM:
			return VK_FORMAT_R8G8_UNORM;
		case Format_R8G8_UINT:
			return VK_FORMAT_R8G8_UINT;
		case Format_R8G8_SNORM:
			return VK_FORMAT_R8G8_SNORM;
		case Format_R8G8_SINT:
			return VK_FORMAT_R8G8_SINT;
		case Format_R16_FLOAT:
			return VK_FORMAT_R16_SFLOAT;
		case Format_D16_UNORM:
			return VK_FORMAT_D16_UNORM;
		case Format_R16_UNORM:
			return VK_FORMAT_R16_UNORM;
		case Format_R16_UINT:
			return VK_FORMAT_R16_UINT;
		case Format_R16_SNORM:
			return VK_FORMAT_R16_SNORM;
		case Format_R16_SINT:
			return VK_FORMAT_R16_SINT;
		case Format_R8_UNORM:
			return VK_FORMAT_R8_UNORM;
		case Format_R8_UINT:
			return VK_FORMAT_R8_UINT;
		case Format_R8_SNORM:
			return VK_FORMAT_R8_SNORM;
		case Format_R8_SINT:
			return VK_FORMAT_R8_SINT;
		case Format_BC1_UNORM:
			return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
		case Format_BC1_UNORM_SRGB:
			return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
		case Format_BC2_UNORM:
			return VK_FORMAT_BC2_UNORM_BLOCK;
		case Format_BC2_UNORM_SRGB:
			return VK_FORMAT_BC2_SRGB_BLOCK;
		case Format_BC3_UNORM:
			return VK_FORMAT_BC3_UNORM_BLOCK;
		case Format_BC3_UNORM_SRGB:
			return VK_FORMAT_BC3_SRGB_BLOCK;
		case Format_BC4_UNORM:
			return VK_FORMAT_BC4_UNORM_BLOCK;
		case Format_BC4_SNORM:
			return VK_FORMAT_BC4_SNORM_BLOCK;
		case Format_BC5_UNORM:
			return VK_FORMAT_BC5_UNORM_BLOCK;
		case Format_BC5_SNORM:
			return VK_FORMAT_BC5_SNORM_BLOCK;
		case Format_B8G8R8A8_UNORM:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case Format_B8G8R8A8_UNORM_SRGB:
			return VK_FORMAT_B8G8R8A8_SRGB;
		case Format_BC6H_UF16:
			return VK_FORMAT_BC6H_UFLOAT_BLOCK;
		case Format_BC6H_SF16:
			return VK_FORMAT_BC6H_SFLOAT_BLOCK;
		case Format_BC7_UNORM:
			return VK_FORMAT_BC7_UNORM_BLOCK;
		case Format_BC7_UNORM_SRGB:
			return VK_FORMAT_BC7_SRGB_BLOCK;
		case Format_NV12:
			return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
		}
		return VK_FORMAT_UNDEFINED;
	}
	constexpr VkCompareOp _ConvertComparisonFunc(ComparisonFunc value)
	{
		switch (value)
		{
		case ComparisonFunc_NEVER:
			return VK_COMPARE_OP_NEVER;
		case ComparisonFunc_LESS:
			return VK_COMPARE_OP_LESS;
		case ComparisonFunc_EQUAL:
			return VK_COMPARE_OP_EQUAL;
		case ComparisonFunc_LESS_EQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case ComparisonFunc_GREATER:
			return VK_COMPARE_OP_GREATER;
		case ComparisonFunc_NOT_EQUAL:
			return VK_COMPARE_OP_NOT_EQUAL;
		case ComparisonFunc_GREATER_EQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case ComparisonFunc_ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
		default:
			return VK_COMPARE_OP_NEVER;
		}
	}
	constexpr VkBlendFactor _ConvertBlend(Blend value)
	{
		switch (value)
		{
		case Blend_ZERO:
			return VK_BLEND_FACTOR_ZERO;
		case Blend_ONE:
			return VK_BLEND_FACTOR_ONE;
		case Blend_SRC_COLOR:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case Blend_INV_SRC_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case Blend_SRC_ALPHA:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case Blend_INV_SRC_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case Blend_DEST_ALPHA:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case Blend_INV_DEST_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case Blend_DEST_COLOR:
			return VK_BLEND_FACTOR_DST_COLOR;
		case Blend_INV_DEST_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case Blend_SRC_ALPHA_SAT:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case Blend_BLEND_FACTOR:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case Blend_INV_BLEND_FACTOR:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
			break;
		case Blend_SRC1_COLOR:
			return VK_BLEND_FACTOR_SRC1_COLOR;
		case Blend_INV_SRC1_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case Blend_SRC1_ALPHA:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
		case Blend_INV_SRC1_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		default:
			return VK_BLEND_FACTOR_ZERO;
		}
	}
	constexpr VkBlendOp _ConvertBlendOp(BlendOp value)
	{
		switch (value)
		{
		case BlendOp_ADD:
			return VK_BLEND_OP_ADD;
		case BlendOp_SUBTRACT:
			return VK_BLEND_OP_SUBTRACT;
		case BlendOp_REV_SUBTRACT:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case BlendOp_MIN:
			return VK_BLEND_OP_MIN;
		case BlendOp_MAX:
			return VK_BLEND_OP_MAX;
		default:
			return VK_BLEND_OP_ADD;
		}
	}
	constexpr VkSamplerAddressMode _ConvertTextureAddressMode(TextureAddressMode value, const VkPhysicalDeviceVulkan12Features& features_1_2)
	{
		switch (value)
		{
		case TextureAddressMode_WRAP:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case TextureAddressMode_MIRROR:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case TextureAddressMode_CLAMP:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case TextureAddressMode_BORDER:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case TextureAddressMode_MIRROR_ONCE:
			if (features_1_2.samplerMirrorClampToEdge == VK_TRUE)
			{
				return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			}
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		default:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		}
	}
	constexpr VkBorderColor _ConvertSamplerBorderColor(SamplerBorderColor value)
	{
		switch (value)
		{
		case SamplerBorderColor_TRANSPARENT_BLACK:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		case SamplerBorderColor_OPAQUE_BLACK:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case SamplerBorderColor_OPAQUE_WHITE:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		default:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		}
	}
	constexpr VkStencilOp _ConvertStencilOp(StencilOp value)
	{
		switch (value)
		{
		case Protean::Graphics::StencilOp_KEEP:
			return VK_STENCIL_OP_KEEP;
		case Protean::Graphics::StencilOp_ZERO:
			return VK_STENCIL_OP_ZERO;
		case Protean::Graphics::StencilOp_REPLACE:
			return VK_STENCIL_OP_REPLACE;
		case Protean::Graphics::StencilOp_INCR_SAT:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case Protean::Graphics::StencilOp_DECR_SAT:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case Protean::Graphics::StencilOp_INVERT:
			return VK_STENCIL_OP_INVERT;
		case Protean::Graphics::StencilOp_INCR:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case Protean::Graphics::StencilOp_DECR:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			return VK_STENCIL_OP_KEEP;
		}
	}
	constexpr VkImageLayout _ConvertImageLayout(ResourceState value)
	{
		switch (value)
		{
		case ResourceState_UNDEFINED:
			return VK_IMAGE_LAYOUT_UNDEFINED;
		case ResourceState_RENDERTARGET:
			return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		case ResourceState_DEPTHSTENCIL:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		case ResourceState_DEPTHSTENCIL_READONLY:
			return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
		case ResourceState_SHADER_RESOURCE:
		case ResourceState_SHADER_RESOURCE_COMPUTE:
			return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		case ResourceState_UNORDERED_ACCESS:
			return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceState_COPY_SRC:
		case ResourceState_COPY_DST:
			// we can't assume transfer layout because it's allowed for resource to be used by multiple queues like DX12 (decay to common state), so this is a workaround
			//	the problem is that image copy commands will require specifying the current layout, but different queues can often use textures in different layouts
			return VK_IMAGE_LAYOUT_GENERAL;
		case ResourceState_SHADING_RATE_SOURCE:
			return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
		case ResourceState_VIDEO_DECODE_DPB:
			return VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR;
		case ResourceState_VIDEO_DECODE_SRC:
			return VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR;
		case ResourceState_VIDEO_DECODE_DST:
			return VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR;
		case ResourceState_SWAPCHAIN:
			return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		default:
			// combination of state flags will default to general
			//	whether the combination of states is valid needs to be validated by the user
			//	combining read-only states should be fine
			return VK_IMAGE_LAYOUT_GENERAL;
		}
	}
	constexpr VkShaderStageFlags _ConvertStageFlags(ShaderStage value)
	{
		switch (value)
		{
		case ShaderStage_MS:
			return VK_SHADER_STAGE_MESH_BIT_EXT;
		case ShaderStage_AS:
			return VK_SHADER_STAGE_TASK_BIT_EXT;
		case ShaderStage_VS:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case ShaderStage_HS:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case ShaderStage_DS:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case ShaderStage_GS:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		case ShaderStage_PS:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case ShaderStage_CS:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		default:
			return VK_SHADER_STAGE_ALL;
		}
	}
	constexpr VkImageAspectFlags _ConvertImageAspect(ImageAspect value)
	{
		switch (value)
		{
		default:
		case Protean::Graphics::ImageAspect_COLOR:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		case Protean::Graphics::ImageAspect_DEPTH:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case Protean::Graphics::ImageAspect_STENCIL:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		case Protean::Graphics::ImageAspect_LUMINANCE:
			return VK_IMAGE_ASPECT_PLANE_0_BIT;
		case Protean::Graphics::ImageAspect_CHROMINANCE:
			return VK_IMAGE_ASPECT_PLANE_1_BIT;
		}
	}
	constexpr VkPipelineStageFlags2 _ConvertPipelineStage(ResourceState value)
	{
		VkPipelineStageFlags2 flags = VK_PIPELINE_STAGE_2_NONE;

		if (has_flag(value, ResourceState_SHADER_RESOURCE) ||
			has_flag(value, ResourceState_SHADER_RESOURCE_COMPUTE) ||
			has_flag(value, ResourceState_UNORDERED_ACCESS) ||
			has_flag(value, ResourceState_CONSTANT_BUFFER))
		{
			flags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		}
		if (has_flag(value, ResourceState_COPY_SRC) ||
			has_flag(value, ResourceState_COPY_DST))
		{
			flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
		}
		if (has_flag(value, ResourceState_RENDERTARGET))
		{
			flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
		}
		if (has_flag(value, ResourceState_DEPTHSTENCIL) ||
			has_flag(value, ResourceState_DEPTHSTENCIL_READONLY))
		{
			flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
		}
		if (has_flag(value, ResourceState_SHADING_RATE_SOURCE))
		{
			flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		}
		if (has_flag(value, ResourceState_VERTEX_BUFFER))
		{
			flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
		}
		if (has_flag(value, ResourceState_INDEX_BUFFER))
		{
			flags |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
		}
		if (has_flag(value, ResourceState_INDIRECT_ARGUMENT))
		{
			flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
		}
		if (has_flag(value, ResourceState_RAYTRACING_ACCELERATION_STRUCTURE))
		{
			flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
		}
		if (has_flag(value, ResourceState_PREDICATION))
		{
			flags |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
		}
		if (has_flag(value, ResourceState_VIDEO_DECODE_DST) ||
			has_flag(value, ResourceState_VIDEO_DECODE_SRC) ||
			has_flag(value, ResourceState_VIDEO_DECODE_DPB))
		{
			flags |= VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
		}

		return flags;
	}
	constexpr VkAccessFlags2 _ParseResourceState(ResourceState value)
	{
		VkAccessFlags2 flags = 0;

		if (has_flag(value, ResourceState_SHADER_RESOURCE))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (has_flag(value, ResourceState_SHADER_RESOURCE_COMPUTE))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
		}
		if (has_flag(value, ResourceState_UNORDERED_ACCESS))
		{
			flags |= VK_ACCESS_2_SHADER_READ_BIT;
			flags |= VK_ACCESS_2_SHADER_WRITE_BIT;
		}
		if (has_flag(value, ResourceState_COPY_SRC))
		{
			flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
		}
		if (has_flag(value, ResourceState_COPY_DST))
		{
			flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
		}
		if (has_flag(value, ResourceState_RENDERTARGET))
		{
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		}
		if (has_flag(value, ResourceState_DEPTHSTENCIL))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}
		if (has_flag(value, ResourceState_DEPTHSTENCIL_READONLY))
		{
			flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}
		if (has_flag(value, ResourceState_VERTEX_BUFFER))
		{
			flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
		}
		if (has_flag(value, ResourceState_INDEX_BUFFER))
		{
			flags |= VK_ACCESS_2_INDEX_READ_BIT;
		}
		if (has_flag(value, ResourceState_CONSTANT_BUFFER))
		{
			flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
		}
		if (has_flag(value, ResourceState_INDIRECT_ARGUMENT))
		{
			flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
		}
		if (has_flag(value, ResourceState_PREDICATION))
		{
			flags |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
		}
		if (has_flag(value, ResourceState_SHADING_RATE_SOURCE))
		{
			flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
		}
		if (has_flag(value, ResourceState_VIDEO_DECODE_DST))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
		}
		if (has_flag(value, ResourceState_VIDEO_DECODE_SRC))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;
		}
		if (has_flag(value, ResourceState_VIDEO_DECODE_DPB))
		{
			flags |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
		}

		return flags;
	}
	constexpr VkComponentSwizzle _ConvertComponentSwizzle(ComponentSwizzle value)
	{
		switch (value)
		{
		default:
			return VK_COMPONENT_SWIZZLE_IDENTITY;
		case Protean::Graphics::ComponentSwizzle_R:
			return VK_COMPONENT_SWIZZLE_R;
		case Protean::Graphics::ComponentSwizzle_G:
			return VK_COMPONENT_SWIZZLE_G;
		case Protean::Graphics::ComponentSwizzle_B:
			return VK_COMPONENT_SWIZZLE_B;
		case Protean::Graphics::ComponentSwizzle_A:
			return VK_COMPONENT_SWIZZLE_A;
		case Protean::Graphics::ComponentSwizzle_ZERO:
			return VK_COMPONENT_SWIZZLE_ZERO;
		case Protean::Graphics::ComponentSwizzle_ONE:
			return VK_COMPONENT_SWIZZLE_ONE;
		}
	}
	constexpr VkComponentMapping _ConvertSwizzle(Swizzle value)
	{
		VkComponentMapping mapping = {};
		mapping.r = _ConvertComponentSwizzle(value.r);
		mapping.g = _ConvertComponentSwizzle(value.g);
		mapping.b = _ConvertComponentSwizzle(value.b);
		mapping.a = _ConvertComponentSwizzle(value.a);
		return mapping;
	}
	constexpr VkImageCreateInfo _ConvertTextureDesc(const TextureDesc& desc, const uint32_t* families, uint32_t family_count,
		VkVideoProfileListInfoKHR& profile_list_info, const VkVideoProfileInfoKHR& profile_h264, const VkVideoProfileInfoKHR& profile_h265)
	{
		VkImageCreateInfo imageInfo = {};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.extent.width = desc.width;
		imageInfo.extent.height = desc.height;
		imageInfo.extent.depth = desc.depth;
		imageInfo.format = _ConvertFormat(desc.format);
		imageInfo.arrayLayers = desc.array_size;
		imageInfo.mipLevels = GetMipCount(desc);
		imageInfo.samples = (VkSampleCountFlagBits)desc.sample_count;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = 0;
		if (has_flag(desc.bind_flags, BindFlag_SHADER_RESOURCE))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
		}
		if (has_flag(desc.bind_flags, BindFlag_UNORDERED_ACCESS))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

			if (IsFormatSRGB(desc.format))
			{
				imageInfo.flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
			}
		}
		if (has_flag(desc.bind_flags, BindFlag_RENDER_TARGET))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		}
		if (has_flag(desc.bind_flags, BindFlag_DEPTH_STENCIL))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		}
		if (has_flag(desc.bind_flags, BindFlag_SHADING_RATE))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		}
		if (has_flag(desc.misc_flags, ResourceMiscFlag_TRANSIENT_ATTACHMENT))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		}
		else
		{
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		}

		if (has_flag(desc.misc_flags, ResourceMiscFlag_TEXTURECUBE))
		{
			imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}
		if (has_flag(desc.misc_flags, ResourceMiscFlag_TYPED_FORMAT_CASTING) || has_flag(desc.misc_flags, ResourceMiscFlag_TYPELESS_FORMAT_CASTING))
		{
			imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		}

		if (has_flag(desc.misc_flags, ResourceMiscFlag_SPARSE))
		{
			imageInfo.flags |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
			imageInfo.flags |= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
			imageInfo.flags |= VK_IMAGE_CREATE_SPARSE_ALIASED_BIT;
		}

		if (has_flag(desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR;
			imageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR;
			imageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
		}
		if (has_flag(desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE_OUTPUT_ONLY))
		{
			imageInfo.usage |= VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR;
		}
		if (has_flag(desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE_DPB_ONLY))
		{
			imageInfo.usage = VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR; // Note: this is not a combination of flags, but complete assignment!
		}

		if (desc.format == Format_NV12 && has_flag(desc.bind_flags, BindFlag_SHADER_RESOURCE))
		{
			imageInfo.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
		}

		if (family_count > 1)
		{
			imageInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			imageInfo.queueFamilyIndexCount = family_count;
			imageInfo.pQueueFamilyIndices = families;
		}
		else
		{
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		switch (desc.type)
		{
		case TextureDesc::Type_TEXTURE_1D:
			imageInfo.imageType = VK_IMAGE_TYPE_1D;
			break;
		case TextureDesc::Type_TEXTURE_2D:
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			break;
		case TextureDesc::Type_TEXTURE_3D:
			imageInfo.imageType = VK_IMAGE_TYPE_3D;
			break;
		default:
			assert(0);
			break;
		}

		if (has_flag(desc.misc_flags, ResourceMiscFlag_VIDEO_COMPATIBILITY_H264))
		{
			profile_list_info.pProfiles = &profile_h264;
			profile_list_info.profileCount = 1;
			imageInfo.pNext = &profile_list_info;
		}
		else if (has_flag(desc.misc_flags, ResourceMiscFlag_VIDEO_COMPATIBILITY_H265))
		{
			profile_list_info.pProfiles = &profile_h265;
			profile_list_info.profileCount = 1;
			imageInfo.pNext = &profile_list_info;
		}

		return imageInfo;
	}


	inline bool checkExtensionSupport(const char* checkExtension, const Protean::vector<VkExtensionProperties>& available_extensions)
	{
		for (const auto& x : available_extensions)
		{
			if (strcmp(x.extensionName, checkExtension) == 0)
			{
				return true;
			}
		}
		return false;
	}

	inline bool ValidateLayers(const Protean::vector<const char*>& required,
		const Protean::vector<VkLayerProperties>& available)
	{
		for (auto layer : required)
		{
			bool found = false;
			for (auto& available_layer : available)
			{
				if (strcmp(available_layer.layerName, layer) == 0)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				return false;
			}
		}

		return true;
	}

	inline VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
		VkDebugUtilsMessageTypeFlagsEXT message_type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data)
	{
		// Log debug message
		std::string ss;

		if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			ss += "[Vulkan Warning]: ";
			ss += callback_data->pMessage;
			ss += "\n";
			Protean::Helper::DebugOut(ss, Protean::Helper::DebugLevel_Warning);
		}
		else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			ss += "[Vulkan Error]: ";
			ss += callback_data->pMessage;
			ss += "\n";
			Protean::Helper::DebugOut(ss, Protean::Helper::DebugLevel_Error);
		}

		return VK_FALSE;
	}

	inline std::string get_shader_cache_path()
	{
		return Protean::Helper::GetCurrentPath() + "/pso_cache_vulkan";
	}

	struct BindingUsage
	{
		bool used = false;
		VkDescriptorSetLayoutBinding binding = {};
	};
	struct Buffer_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkBuffer resource = VK_NULL_HANDLE;
		struct BufferSubresource
		{
			VkBufferView buffer_view = VK_NULL_HANDLE;
			VkDescriptorBufferInfo buffer_info = {};
			int index = -1; // bindless

			constexpr bool IsValid() const
			{
				return index >= 0;
			}
		};
		BufferSubresource srv;
		BufferSubresource uav;
		Protean::vector<BufferSubresource> subresources_srv;
		Protean::vector<BufferSubresource> subresources_uav;
		VkDeviceAddress address = 0;

		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			if (srv.IsValid())
			{
				if (srv.buffer_view != VK_NULL_HANDLE)
				{
					allocationhandler->destroyer_bufferviews.push_back(std::make_pair(srv.buffer_view, framecount));
					allocationhandler->destroyer_bindlessUniformTexelBuffers.push_back(std::make_pair(srv.index, framecount));
				}
				else
				{
					allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(srv.index, framecount));
				}
				srv = {};
			}
			if (uav.IsValid())
			{
				if (uav.buffer_view != VK_NULL_HANDLE)
				{
					allocationhandler->destroyer_bufferviews.push_back(std::make_pair(uav.buffer_view, framecount));
					allocationhandler->destroyer_bindlessStorageTexelBuffers.push_back(std::make_pair(uav.index, framecount));
				}
				else
				{
					allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(uav.index, framecount));
				}
				uav = {};
			}
			for (auto& x : subresources_srv)
			{
				if (x.buffer_view != VK_NULL_HANDLE)
				{
					allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x.buffer_view, framecount));
					allocationhandler->destroyer_bindlessUniformTexelBuffers.push_back(std::make_pair(x.index, framecount));
				}
				else
				{
					allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(x.index, framecount));
				}
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				if (x.buffer_view != VK_NULL_HANDLE)
				{
					allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x.buffer_view, framecount));
					allocationhandler->destroyer_bindlessStorageTexelBuffers.push_back(std::make_pair(x.index, framecount));
				}
				else
				{
					allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(x.index, framecount));
				}
			}
			subresources_uav.clear();
		}

		~Buffer_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource)
			{
				allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
			}
			else if(allocation)
			{
				allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
			}
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Texture_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkImage resource = VK_NULL_HANDLE;
		VkImageLayout defaultLayout = VK_IMAGE_LAYOUT_GENERAL;
		VkBuffer staging_resource = VK_NULL_HANDLE;
		struct TextureSubresource
		{
			VkImageView image_view = VK_NULL_HANDLE;
			int index = -1; // bindless
			uint32_t firstMip = 0;
			uint32_t mipCount = 0;
			uint32_t firstSlice = 0;
			uint32_t sliceCount = 0;

			constexpr bool IsValid() const
			{
				return image_view != VK_NULL_HANDLE;
			}
		};
		TextureSubresource srv;
		TextureSubresource uav;
		TextureSubresource rtv;
		TextureSubresource dsv;
		uint32_t framebuffer_layercount = 0;
		Protean::vector<TextureSubresource> subresources_srv;
		Protean::vector<TextureSubresource> subresources_uav;
		Protean::vector<TextureSubresource> subresources_rtv;
		Protean::vector<TextureSubresource> subresources_dsv;

		Protean::vector<SubresourceData> mapped_subresources;
		SparseTextureProperties sparse_texture_properties;

		VkImageView video_decode_view = VK_NULL_HANDLE;

		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			if (srv.IsValid())
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(srv.image_view, framecount));
				allocationhandler->destroyer_bindlessSampledImages.push_back(std::make_pair(srv.index, framecount));
				srv = {};
			}
			if (uav.IsValid())
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(uav.image_view, framecount));
				allocationhandler->destroyer_bindlessStorageImages.push_back(std::make_pair(uav.index, framecount));
				uav = {};
			}
			if (rtv.IsValid())
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(rtv.image_view, framecount));
				rtv = {};
			}
			if (dsv.IsValid())
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(dsv.image_view, framecount));
				dsv = {};
			}
			for (auto x : subresources_srv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
				allocationhandler->destroyer_bindlessSampledImages.push_back(std::make_pair(x.index, framecount));
			}
			subresources_srv.clear();
			for (auto x : subresources_uav)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
				allocationhandler->destroyer_bindlessStorageImages.push_back(std::make_pair(x.index, framecount));
			}
			subresources_uav.clear();
			for (auto x : subresources_rtv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
			}
			subresources_rtv.clear();
			for (auto x : subresources_dsv)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
			}
			subresources_dsv.clear();
		}

		~Texture_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource)
			{
				allocationhandler->destroyer_images.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
			}
			else if (staging_resource)
			{
				allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(staging_resource, allocation), framecount));
			}
			else if (allocation)
			{
				allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
			}
			if (video_decode_view != VK_NULL_HANDLE)
			{
				allocationhandler->destroyer_imageviews.push_back(std::make_pair(video_decode_view, framecount));
			}
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Sampler_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VkSampler resource = VK_NULL_HANDLE;
		int index = -1;

		~Sampler_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_samplers.push_back(std::make_pair(resource, framecount));
			if (index >= 0) allocationhandler->destroyer_bindlessSamplers.push_back(std::make_pair(index, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct QueryHeap_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VkQueryPool pool = VK_NULL_HANDLE;

		~QueryHeap_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (pool) allocationhandler->destroyer_querypools.push_back(std::make_pair(pool, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Shader_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VkShaderModule shaderModule = VK_NULL_HANDLE;
		VkPipeline pipeline_cs = VK_NULL_HANDLE;
		VkPipelineShaderStageCreateInfo stageInfo = {};
		VulkanDevice::PSOLayout layout;
		std::string entrypoint;

		~Shader_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (shaderModule) allocationhandler->destroyer_shadermodules.push_back(std::make_pair(shaderModule, framecount));
			if (pipeline_cs) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline_cs, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct PipelineState_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VulkanDevice::PSOLayout layout;

		~PipelineState_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (pipeline) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct BVH_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VmaAllocation allocation = nullptr;
		VkBuffer buffer = VK_NULL_HANDLE;
		VkAccelerationStructureKHR resource = VK_NULL_HANDLE;
		int index = -1;

		VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
		VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
		VkAccelerationStructureCreateInfoKHR createInfo = {};
		Protean::vector<VkAccelerationStructureGeometryKHR> geometries;
		Protean::vector<uint32_t> primitiveCounts;
		VkDeviceAddress scratch_address = 0;
		VkDeviceAddress as_address = 0;

		~BVH_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (buffer) allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(buffer, allocation), framecount));
			if (resource) allocationhandler->destroyer_bvhs.push_back(std::make_pair(resource, framecount));
			if (index >= 0) allocationhandler->destroyer_bindlessAccelerationStructures.push_back(std::make_pair(index, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct RTPipelineState_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VkPipeline pipeline = VK_NULL_HANDLE;

		~RTPipelineState_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (pipeline) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct SwapChain_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		Protean::vector<Protean::Allocator::shared_ptr<Texture_Vulkan>> textures; // shared_ptr is used because they can be given out by GetBackBuffer()

		Texture dummyTexture;

		VkSurfaceKHR surface = VK_NULL_HANDLE;

		uint32_t swapChainImageIndex = 0;
		uint32_t swapChainAcquireSemaphoreIndex = 0;
		Protean::vector<VkSemaphore> swapchainAcquireSemaphores;
		Protean::vector<VkSemaphore> swapchainReleaseSemaphores;

		ColorSpace colorSpace = ColorSpace_SRGB;
		SwapChainDesc desc;
		std::mutex locker;

		~SwapChain_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;

			for (size_t i = 0; i < swapchainAcquireSemaphores.size(); ++i)
			{
				allocationhandler->destroyer_semaphores.push_back(std::make_pair(swapchainAcquireSemaphores[i], framecount));
				allocationhandler->destroyer_semaphores.push_back(std::make_pair(swapchainReleaseSemaphores[i], framecount));
			}

#if defined(SDL2) || defined(SDL3)
			// Checks if the SDL VIDEO System was already destroyed.
			// If so we would delete the swapchain twice, causing a crash on wayland.
			if (SDL_WasInit(SDL_INIT_VIDEO))
#endif
			{
				allocationhandler->destroyer_swapchains.push_back(std::make_pair(swapChain, framecount));
				allocationhandler->destroyer_surfaces.push_back(std::make_pair(surface, framecount));
			}

			allocationhandler->destroylocker.unlock();

		}
	};
	struct VideoDecoder_Vulkan
	{
		Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler> allocationhandler;
		VkVideoSessionKHR video_session = VK_NULL_HANDLE;
		VkVideoSessionParametersKHR session_parameters = VK_NULL_HANDLE;
		Protean::vector<VmaAllocation> allocations;

		~VideoDecoder_Vulkan()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_video_sessions.push_back(std::make_pair(video_session, framecount));
			allocationhandler->destroyer_video_session_parameters.push_back(std::make_pair(session_parameters, framecount));
			for (auto& x : allocations)
			{
				allocationhandler->destroyer_allocations.push_back(std::make_pair(x, framecount));
			}
			allocationhandler->destroylocker.unlock();
		}
	};

	template<typename T> struct VulkanType;
	template<> struct VulkanType<GPUBuffer> { using type = Buffer_Vulkan; };
	template<> struct VulkanType<Texture> { using type = Texture_Vulkan; };
	template<> struct VulkanType<Sampler> { using type = Sampler_Vulkan; };
	template<> struct VulkanType<GPUQueryHeap> { using type = QueryHeap_Vulkan; };
	template<> struct VulkanType<Shader> { using type = Shader_Vulkan; };
	template<> struct VulkanType<RaytracingAccelerationStructure> { using type = BVH_Vulkan; };
	template<> struct VulkanType<PipelineState> { using type = PipelineState_Vulkan; };
	template<> struct VulkanType<RaytracingPipelineState> { using type = RTPipelineState_Vulkan; };
	template<> struct VulkanType<SwapChain> { using type = SwapChain_Vulkan; };
	template<> struct VulkanType<VideoDecoder> { using type = VideoDecoder_Vulkan; };


	template<typename T>
	typename VulkanType<T>::type* to_internal(const T* param)
	{
		return static_cast<typename VulkanType<T>::type*>(param->internal_state.get());
	}

	template<typename T>
	typename VulkanType<T>::type* to_internal(const GPUResource* res)
	{
		return static_cast<typename VulkanType<T>::type*>(res->internal_state.get());
	}

	inline bool CreateSwapChainInternal(
		SwapChain_Vulkan* internal_state,
		VkPhysicalDevice physicalDevice,
		VkDevice device,
		const Protean::Allocator::shared_ptr<VulkanDevice::AllocationHandler>& allocationhandler
	)
	{
		// In vulkan, the swapchain recreate can happen whenever it gets outdated, it's not in application's control
		//	so we have to be extra careful
		std::scoped_lock lock(internal_state->locker);

		VkSurfaceCapabilitiesKHR swapchain_capabilities;
		vulkan_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, internal_state->surface, &swapchain_capabilities));

		uint32_t formatCount;
		vulkan_check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, nullptr));

		Protean::vector<VkSurfaceFormatKHR> swapchain_formats(formatCount);
		vulkan_check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, swapchain_formats.data()));

		uint32_t presentModeCount;
		vulkan_check(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, internal_state->surface, &presentModeCount, nullptr));

		Protean::vector<VkPresentModeKHR> swapchain_presentModes(presentModeCount);
		swapchain_presentModes.resize(presentModeCount);
		vulkan_check(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, internal_state->surface, &presentModeCount, swapchain_presentModes.data()));

		VkSurfaceFormatKHR surfaceFormat = {};
		surfaceFormat.format = _ConvertFormat(internal_state->desc.format);
		surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		bool valid = false;

		for (const auto& format : swapchain_formats)
		{
			if (!internal_state->desc.allow_hdr && format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				continue;
			if (format.format == surfaceFormat.format)
			{
				surfaceFormat = format;
				valid = true;
				break;
			}
		}
		if (!valid)
		{
			internal_state->desc.format = Format_B8G8R8A8_UNORM;
			surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
			surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		}

		// For now, we only include the color spaces that were tested successfully:
		ColorSpace prev_colorspace = internal_state->colorSpace;
		switch (surfaceFormat.colorSpace)
		{
		default:
		case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
			internal_state->colorSpace = ColorSpace_SRGB;
			break;
		case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
			internal_state->colorSpace = ColorSpace_HDR_LINEAR;
			break;
		case VK_COLOR_SPACE_HDR10_ST2084_EXT:
			internal_state->colorSpace = ColorSpace_HDR10_ST2084;
			break;
		}

		if (prev_colorspace != internal_state->colorSpace)
		{
			if (internal_state->swapChain != VK_NULL_HANDLE)
			{
				// For some reason, if the swapchain gets recreated (via oldSwapChain) with different color space but same image format,
				//	the color space change will not be applied
				vulkan_check(vkDeviceWaitIdle(device));
				vkDestroySwapchainKHR(device, internal_state->swapChain, nullptr);
				internal_state->swapChain = nullptr;
			}
		}

		if (swapchain_capabilities.currentExtent.width != 0xFFFFFFFF && swapchain_capabilities.currentExtent.height != 0xFFFFFFFF)
		{
			internal_state->swapChainExtent = swapchain_capabilities.currentExtent;
		}
		else
		{
			internal_state->swapChainExtent = { internal_state->desc.width, internal_state->desc.height };
			internal_state->swapChainExtent.width = std::max(swapchain_capabilities.minImageExtent.width, std::min(swapchain_capabilities.maxImageExtent.width, internal_state->swapChainExtent.width));
			internal_state->swapChainExtent.height = std::max(swapchain_capabilities.minImageExtent.height, std::min(swapchain_capabilities.maxImageExtent.height, internal_state->swapChainExtent.height));
		}

		uint32_t imageCount = std::max(internal_state->desc.buffer_count, swapchain_capabilities.minImageCount);
		if ((swapchain_capabilities.maxImageCount > 0) && (imageCount > swapchain_capabilities.maxImageCount))
		{
			imageCount = swapchain_capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = internal_state->surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = internal_state->swapChainExtent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.preTransform = swapchain_capabilities.currentTransform;

		createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // The only one that is always supported
		if (!internal_state->desc.vsync)
		{
			// The mailbox/immediate present mode is not necessarily supported:
			for (auto& presentMode : swapchain_presentModes)
			{
				if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
					break;
				}
				if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				}
			}
		}
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = internal_state->swapChain;

		vulkan_check(vkCreateSwapchainKHR(device, &createInfo, nullptr, &internal_state->swapChain));

		if (createInfo.oldSwapchain != VK_NULL_HANDLE)
		{
			std::scoped_lock lock(allocationhandler->destroylocker);
			allocationhandler->destroyer_swapchains.emplace_back(createInfo.oldSwapchain, allocationhandler->framecount);
		}

		vulkan_check(vkGetSwapchainImagesKHR(device, internal_state->swapChain, &imageCount, nullptr));
		VkImage swapChainImages[32] = {};
		assert(arraysize(swapChainImages) >= imageCount);
		vulkan_check(vkGetSwapchainImagesKHR(device, internal_state->swapChain, &imageCount, swapChainImages));
		internal_state->swapChainImageFormat = surfaceFormat.format;

		// Create swap chain render targets:
		internal_state->textures.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = internal_state->swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (internal_state->textures[i].IsValid())
			{
				allocationhandler->destroylocker.lock();
				internal_state->textures[i]->rtv = {};
				internal_state->textures[i]->srv = {};
				allocationhandler->destroylocker.unlock();
			}
			else
			{
				internal_state->textures[i] = Protean::Allocator::make_shared<Texture_Vulkan>();
			}
			internal_state->textures[i]->resource = swapChainImages[i];
			internal_state->textures[i]->defaultLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			vulkan_check(vkCreateImageView(device, &createInfo, nullptr, &internal_state->textures[i]->rtv.image_view));
			vulkan_check(vkCreateImageView(device, &createInfo, nullptr, &internal_state->textures[i]->srv.image_view));
		}

		VkSemaphoreCreateInfo semaphoreInfo = {};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// safety release of current swapchain semaphores that might still be working, since this could have been called mid-frame:
		allocationhandler->destroylocker.lock();
		for (auto& x : internal_state->swapchainAcquireSemaphores)
		{
			allocationhandler->destroyer_semaphores.push_back(std::make_pair(x, allocationhandler->framecount));
		}
		internal_state->swapchainAcquireSemaphores.clear();
		for (auto& x : internal_state->swapchainReleaseSemaphores)
		{
			allocationhandler->destroyer_semaphores.push_back(std::make_pair(x, allocationhandler->framecount));
		}
		internal_state->swapchainReleaseSemaphores.clear();
		allocationhandler->destroylocker.unlock();

		internal_state->swapChainAcquireSemaphoreIndex = 0;
		internal_state->swapChainImageIndex = 0;

		if (internal_state->swapchainAcquireSemaphores.empty())
		{
			for (size_t i = 0; i < internal_state->textures.size(); ++i)
			{
				vulkan_check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &internal_state->swapchainAcquireSemaphores.emplace_back()));
			}
		}

		if (internal_state->swapchainReleaseSemaphores.empty())
		{
			for (size_t i = 0; i < internal_state->textures.size(); ++i)
			{
				vulkan_check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &internal_state->swapchainReleaseSemaphores.emplace_back()));
			}
		}

		return true;
	}
}
}

#endif // WICKEDENGINE_BUILD_VULKAN
