#pragma once
// Internal header shared by the D3D12 backend translation units.
// Not part of the public graphics API, include rhi/rhi.h instead.
#include "rhi/d3d12/d3d12_device.h"

#ifdef WICKEDENGINE_BUILD_DX12
#include "core/helper.h"
#include "core/timer.h"
#include "core/unordered_set.h"

#ifdef PLATFORM_XBOX
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_H264, 0x1b81be68, 0xa0c7, 0x11d3, 0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5);
DEFINE_GUID(D3D12_VIDEO_DECODE_PROFILE_HEVC_MAIN, 0x5b11d51b, 0x2f4c, 0x4452, 0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0);
#else
#include "dx12/dxgiformat.h"
#include "dx12/d3dx12_default.h"
#include "dx12/d3dx12_resource_helpers.h"
#include "dx12/d3dx12_pipeline_state_stream.h"
#include "dx12/d3dx12_check_feature_support.h"
#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#endif // PLATFORM_XBOX

#pragma comment(lib,"dxguid.lib")

#include "h264.h"
#include "dxva.h"

#include <map>
#include <string>
#include <pix.h>

#include <sstream>
#include <algorithm>
#include <intrin.h> // _BitScanReverse64

using namespace Microsoft::WRL;

namespace Protean::Graphics
{
namespace D3D12Internal
{
#ifdef PLATFORM_XBOX
// No renderpass API on xbox yet
#define DISABLE_RENDERPASS
#endif // PLATFORM_XBOX

#ifdef PLATFORM_WINDOWS_DESKTOP
	// On Windows PC we load DLLs manually because graphics device can be chosen at runtime:
	using PFN_CREATE_DXGI_FACTORY_2 = decltype(&CreateDXGIFactory2);
	inline PFN_CREATE_DXGI_FACTORY_2 CreateDXGIFactory2 = nullptr;
#ifdef _DEBUG
	using PFN_DXGI_GET_DEBUG_INTERFACE1 = decltype(&DXGIGetDebugInterface1);
	inline PFN_DXGI_GET_DEBUG_INTERFACE1 DXGIGetDebugInterface1 = nullptr;
#endif
	inline PFN_D3D12_CREATE_DEVICE D3D12CreateDevice = nullptr;
	inline PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer = nullptr;
	inline PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12SerializeRootSignature = nullptr;
#endif // PLATFORM_WINDOWS_DESKTOP

	// Engine -> Native converters
	constexpr uint32_t _ParseColorWriteMask(ColorWrite value)
	{
		uint32_t _flag = 0;

		if (value == ColorWrite_ENABLE_ALL)
		{
			return D3D12_COLOR_WRITE_ENABLE_ALL;
		}
		else
		{
			if (has_flag(value, ColorWrite_ENABLE_RED))
				_flag |= D3D12_COLOR_WRITE_ENABLE_RED;
			if (has_flag(value, ColorWrite_ENABLE_GREEN))
				_flag |= D3D12_COLOR_WRITE_ENABLE_GREEN;
			if (has_flag(value, ColorWrite_ENABLE_BLUE))
				_flag |= D3D12_COLOR_WRITE_ENABLE_BLUE;
			if (has_flag(value, ColorWrite_ENABLE_ALPHA))
				_flag |= D3D12_COLOR_WRITE_ENABLE_ALPHA;
		}

		return _flag;
	}
	constexpr D3D12_RESOURCE_STATES _ParseResourceState(ResourceState value)
	{
		D3D12_RESOURCE_STATES ret = {};

		// MISSING STATE: UNDEFINED
		//	UNDEFINED state will be handled by DiscardResource() operation

		if (has_flag(value, ResourceState_SHADER_RESOURCE))
			ret |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (has_flag(value, ResourceState_SHADER_RESOURCE_COMPUTE))
			ret |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		if (has_flag(value, ResourceState_UNORDERED_ACCESS))
			ret |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		if (has_flag(value, ResourceState_COPY_SRC))
			ret |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		if (has_flag(value, ResourceState_COPY_DST))
			ret |= D3D12_RESOURCE_STATE_COPY_DEST;

		if (has_flag(value, ResourceState_RENDERTARGET))
			ret |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		if (has_flag(value, ResourceState_DEPTHSTENCIL))
			ret |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
		if (has_flag(value, ResourceState_DEPTHSTENCIL_READONLY))
			ret |= D3D12_RESOURCE_STATE_DEPTH_READ;
		if (has_flag(value, ResourceState_SHADING_RATE_SOURCE))
			ret |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

		if (has_flag(value, ResourceState_VERTEX_BUFFER))
			ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (has_flag(value, ResourceState_INDEX_BUFFER))
			ret |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		if (has_flag(value, ResourceState_CONSTANT_BUFFER))
			ret |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		if (has_flag(value, ResourceState_INDIRECT_ARGUMENT))
			ret |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
		if (has_flag(value, ResourceState_RAYTRACING_ACCELERATION_STRUCTURE))
			ret |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		if (has_flag(value, ResourceState_PREDICATION))
			ret |= D3D12_RESOURCE_STATE_PREDICATION;

		if (has_flag(value, ResourceState_VIDEO_DECODE_SRC))
			ret |= D3D12_RESOURCE_STATE_VIDEO_DECODE_READ;
		if (has_flag(value, ResourceState_VIDEO_DECODE_DST))
			ret |= D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE;
		if (has_flag(value, ResourceState_VIDEO_DECODE_DPB))
			ret |= D3D12_RESOURCE_STATE_VIDEO_DECODE_WRITE;

		if (has_flag(value, ResourceState_SWAPCHAIN))
			ret |= D3D12_RESOURCE_STATE_PRESENT;

		return ret;
	}
	constexpr D3D12_FILTER _ConvertFilter(Filter value)
	{
		switch (value)
		{
		case Filter_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MIN_MAG_MIP_POINT;
			break;
		case Filter_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case Filter_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case Filter_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case Filter_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case Filter_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case Filter_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case Filter_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MIN_MAG_MIP_LINEAR;
			break;
		case Filter_ANISOTROPIC:
			return D3D12_FILTER_ANISOTROPIC;
			break;
		case Filter_COMPARISON_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
			break;
		case Filter_COMPARISON_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case Filter_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case Filter_COMPARISON_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case Filter_COMPARISON_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case Filter_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case Filter_COMPARISON_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case Filter_COMPARISON_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
			break;
		case Filter_COMPARISON_ANISOTROPIC:
			return D3D12_FILTER_COMPARISON_ANISOTROPIC;
			break;
		case Filter_MINIMUM_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT;
			break;
		case Filter_MINIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case Filter_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case Filter_MINIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case Filter_MINIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case Filter_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case Filter_MINIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case Filter_MINIMUM_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR;
			break;
		case Filter_MINIMUM_ANISOTROPIC:
			return D3D12_FILTER_MINIMUM_ANISOTROPIC;
			break;
		case Filter_MAXIMUM_MIN_MAG_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
			break;
		case Filter_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR;
			break;
		case Filter_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT;
			break;
		case Filter_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR;
			break;
		case Filter_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT;
			break;
		case Filter_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
			break;
		case Filter_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT;
			break;
		case Filter_MAXIMUM_MIN_MAG_MIP_LINEAR:
			return D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR;
			break;
		case Filter_MAXIMUM_ANISOTROPIC:
			return D3D12_FILTER_MAXIMUM_ANISOTROPIC;
			break;
		default:
			break;
		}
		return D3D12_FILTER_MIN_MAG_MIP_POINT;
	}
	constexpr D3D_PRIMITIVE_TOPOLOGY _ConvertPrimitiveTopology(PrimitiveTopology topology, uint32_t controlPoints)
	{
		switch (topology)
		{
			case PrimitiveTopology_POINTLIST:
				return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
			case PrimitiveTopology_LINELIST:
				return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
			case PrimitiveTopology_LINESTRIP:
				return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
			case PrimitiveTopology_TRIANGLELIST:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			case PrimitiveTopology_TRIANGLESTRIP:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			case PrimitiveTopology_PATCHLIST:
				if (controlPoints == 0 || controlPoints > 32)
				{
					assert(false && "Invalid PatchList control points");
					return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
				}
				return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (controlPoints - 1));
			default:
				return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}
	constexpr D3D12_TEXTURE_ADDRESS_MODE _ConvertTextureAddressMode(TextureAddressMode value)
	{
		switch (value)
		{
		case TextureAddressMode_WRAP:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case TextureAddressMode_MIRROR:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case TextureAddressMode_CLAMP:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case TextureAddressMode_BORDER:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		case TextureAddressMode_MIRROR_ONCE:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		default:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		}
	}
	constexpr D3D12_STATIC_BORDER_COLOR _ConvertSamplerBorderColor(SamplerBorderColor value)
	{
		switch (value)
		{
		case SamplerBorderColor_TRANSPARENT_BLACK:
			return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		case SamplerBorderColor_OPAQUE_BLACK:
			return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		case SamplerBorderColor_OPAQUE_WHITE:
			return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		default:
			return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		}
	}
	constexpr D3D12_COMPARISON_FUNC _ConvertComparisonFunc(ComparisonFunc value)
	{
		switch (value)
		{
		case ComparisonFunc_NEVER:
			return D3D12_COMPARISON_FUNC_NEVER;
			break;
		case ComparisonFunc_LESS:
			return D3D12_COMPARISON_FUNC_LESS;
			break;
		case ComparisonFunc_EQUAL:
			return D3D12_COMPARISON_FUNC_EQUAL;
			break;
		case ComparisonFunc_LESS_EQUAL:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
			break;
		case ComparisonFunc_GREATER:
			return D3D12_COMPARISON_FUNC_GREATER;
			break;
		case ComparisonFunc_NOT_EQUAL:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
			break;
		case ComparisonFunc_GREATER_EQUAL:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
			break;
		case ComparisonFunc_ALWAYS:
			return D3D12_COMPARISON_FUNC_ALWAYS;
			break;
		default:
			break;
		}
		return D3D12_COMPARISON_FUNC_NEVER;
	}
	constexpr D3D12_FILL_MODE _ConvertFillMode(FillMode value)
	{
		switch (value)
		{
		case FillMode_WIREFRAME:
			return D3D12_FILL_MODE_WIREFRAME;
			break;
		case FillMode_SOLID:
			return D3D12_FILL_MODE_SOLID;
			break;
		default:
			break;
		}
		return D3D12_FILL_MODE_WIREFRAME;
	}
	constexpr D3D12_CULL_MODE _ConvertCullMode(CullMode value)
	{
		switch (value)
		{
		case CullMode_NONE:
			return D3D12_CULL_MODE_NONE;
			break;
		case CullMode_FRONT:
			return D3D12_CULL_MODE_FRONT;
			break;
		case CullMode_BACK:
			return D3D12_CULL_MODE_BACK;
			break;
		default:
			break;
		}
		return D3D12_CULL_MODE_NONE;
	}
	constexpr D3D12_DEPTH_WRITE_MASK _ConvertDepthWriteMask(DepthWriteMask value)
	{
		switch (value)
		{
		case DepthWriteMask_ZERO:
			return D3D12_DEPTH_WRITE_MASK_ZERO;
			break;
		case DepthWriteMask_ALL:
			return D3D12_DEPTH_WRITE_MASK_ALL;
			break;
		default:
			break;
		}
		return D3D12_DEPTH_WRITE_MASK_ZERO;
	}
	constexpr D3D12_STENCIL_OP _ConvertStencilOp(StencilOp value)
	{
		switch (value)
		{
		case StencilOp_KEEP:
			return D3D12_STENCIL_OP_KEEP;
			break;
		case StencilOp_ZERO:
			return D3D12_STENCIL_OP_ZERO;
			break;
		case StencilOp_REPLACE:
			return D3D12_STENCIL_OP_REPLACE;
			break;
		case StencilOp_INCR_SAT:
			return D3D12_STENCIL_OP_INCR_SAT;
			break;
		case StencilOp_DECR_SAT:
			return D3D12_STENCIL_OP_DECR_SAT;
			break;
		case StencilOp_INVERT:
			return D3D12_STENCIL_OP_INVERT;
			break;
		case StencilOp_INCR:
			return D3D12_STENCIL_OP_INCR;
			break;
		case StencilOp_DECR:
			return D3D12_STENCIL_OP_DECR;
			break;
		default:
			break;
		}
		return D3D12_STENCIL_OP_KEEP;
	}
	constexpr D3D12_BLEND _ConvertBlend(Blend value)
	{
		switch (value)
		{
		case Blend_ZERO:
			return D3D12_BLEND_ZERO;
		case Blend_ONE:
			return D3D12_BLEND_ONE;
		case Blend_SRC_COLOR:
			return D3D12_BLEND_SRC_COLOR;
		case Blend_INV_SRC_COLOR:
			return D3D12_BLEND_INV_SRC_COLOR;
		case Blend_SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
		case Blend_INV_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case Blend_DEST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
		case Blend_INV_DEST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
		case Blend_DEST_COLOR:
			return D3D12_BLEND_DEST_COLOR;
		case Blend_INV_DEST_COLOR:
			return D3D12_BLEND_INV_DEST_COLOR;
		case Blend_SRC_ALPHA_SAT:
			return D3D12_BLEND_SRC_ALPHA_SAT;
		case Blend_BLEND_FACTOR:
			return D3D12_BLEND_BLEND_FACTOR;
		case Blend_INV_BLEND_FACTOR:
			return D3D12_BLEND_INV_BLEND_FACTOR;
		case Blend_SRC1_COLOR:
			return D3D12_BLEND_SRC1_COLOR;
		case Blend_INV_SRC1_COLOR:
			return D3D12_BLEND_INV_SRC1_COLOR;
		case Blend_SRC1_ALPHA:
			return D3D12_BLEND_SRC1_ALPHA;
		case Blend_INV_SRC1_ALPHA:
			return D3D12_BLEND_INV_SRC1_ALPHA;
		default:
			return D3D12_BLEND_ZERO;
		}
	}
	constexpr D3D12_BLEND _ConvertAlphaBlend(Blend value)
	{
		switch (value)
		{
			case Blend_SRC_COLOR:
				return D3D12_BLEND_SRC_ALPHA;
			case Blend_INV_SRC_COLOR:
				return D3D12_BLEND_INV_SRC_ALPHA;
			case Blend_DEST_COLOR:
				return D3D12_BLEND_DEST_ALPHA;
			case Blend_INV_DEST_COLOR:
				return D3D12_BLEND_INV_DEST_ALPHA;
			case Blend_SRC1_COLOR:
				return D3D12_BLEND_SRC1_ALPHA;
			case Blend_INV_SRC1_COLOR:
				return D3D12_BLEND_INV_SRC1_ALPHA;
			default:
				return _ConvertBlend(value);
		}
	}
	constexpr D3D12_BLEND_OP _ConvertBlendOp(BlendOp value)
	{
		switch (value)
		{
		case BlendOp_ADD:
			return D3D12_BLEND_OP_ADD;
		case BlendOp_SUBTRACT:
			return D3D12_BLEND_OP_SUBTRACT;
		case BlendOp_REV_SUBTRACT:
			return D3D12_BLEND_OP_REV_SUBTRACT;
		case BlendOp_MIN:
			return D3D12_BLEND_OP_MIN;
		case BlendOp_MAX:
			return D3D12_BLEND_OP_MAX;
		default:
			return D3D12_BLEND_OP_ADD;
		}
	}
	constexpr D3D12_INPUT_CLASSIFICATION _ConvertInputClassification(InputClassification value)
	{
		switch (value)
		{
		case InputClassification_PER_INSTANCE_DATA:
			return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
			break;
		default:
			return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		}
	}
	constexpr DXGI_FORMAT _ConvertFormat(Format value)
	{
		switch (value)
		{
		case Format_UNKNOWN:
			return DXGI_FORMAT_UNKNOWN;
		case Format_R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Format_R32G32B32A32_UINT:
			return DXGI_FORMAT_R32G32B32A32_UINT;
		case Format_R32G32B32A32_SINT:
			return DXGI_FORMAT_R32G32B32A32_SINT;
		case Format_R32G32B32_FLOAT:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case Format_R32G32B32_UINT:
			return DXGI_FORMAT_R32G32B32_UINT;
		case Format_R32G32B32_SINT:
			return DXGI_FORMAT_R32G32B32_SINT;
		case Format_R16G16B16A16_FLOAT:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Format_R16G16B16A16_UNORM:
			return DXGI_FORMAT_R16G16B16A16_UNORM;
		case Format_R16G16B16A16_UINT:
			return DXGI_FORMAT_R16G16B16A16_UINT;
		case Format_R16G16B16A16_SNORM:
			return DXGI_FORMAT_R16G16B16A16_SNORM;
		case Format_R16G16B16A16_SINT:
			return DXGI_FORMAT_R16G16B16A16_SINT;
		case Format_R32G32_FLOAT:
			return DXGI_FORMAT_R32G32_FLOAT;
		case Format_R32G32_UINT:
			return DXGI_FORMAT_R32G32_UINT;
		case Format_R32G32_SINT:
			return DXGI_FORMAT_R32G32_SINT;
		case Format_D32_FLOAT_S8X24_UINT:
			return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
		case Format_R10G10B10A2_UNORM:
			return DXGI_FORMAT_R10G10B10A2_UNORM;
		case Format_R10G10B10A2_UINT:
			return DXGI_FORMAT_R10G10B10A2_UINT;
		case Format_R11G11B10_FLOAT:
			return DXGI_FORMAT_R11G11B10_FLOAT;
		case Format_R8G8B8A8_UNORM:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Format_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case Format_R8G8B8A8_UINT:
			return DXGI_FORMAT_R8G8B8A8_UINT;
		case Format_R8G8B8A8_SNORM:
			return DXGI_FORMAT_R8G8B8A8_SNORM;
		case Format_R8G8B8A8_SINT:
			return DXGI_FORMAT_R8G8B8A8_SINT;
		case Format_R16G16_FLOAT:
			return DXGI_FORMAT_R16G16_FLOAT;
		case Format_R16G16_UNORM:
			return DXGI_FORMAT_R16G16_UNORM;
		case Format_R16G16_UINT:
			return DXGI_FORMAT_R16G16_UINT;
		case Format_R16G16_SNORM:
			return DXGI_FORMAT_R16G16_SNORM;
		case Format_R16G16_SINT:
			return DXGI_FORMAT_R16G16_SINT;
		case Format_D32_FLOAT:
			return DXGI_FORMAT_D32_FLOAT;
		case Format_R32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		case Format_R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case Format_R32_SINT:
			return DXGI_FORMAT_R32_SINT;
		case Format_D24_UNORM_S8_UINT:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		case Format_R9G9B9E5_SHAREDEXP:
			return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		case Format_R8G8_UNORM:
			return DXGI_FORMAT_R8G8_UNORM;
		case Format_R8G8_UINT:
			return DXGI_FORMAT_R8G8_UINT;
		case Format_R8G8_SNORM:
			return DXGI_FORMAT_R8G8_SNORM;
		case Format_R8G8_SINT:
			return DXGI_FORMAT_R8G8_SINT;
		case Format_R16_FLOAT:
			return DXGI_FORMAT_R16_FLOAT;
		case Format_D16_UNORM:
			return DXGI_FORMAT_D16_UNORM;
		case Format_R16_UNORM:
			return DXGI_FORMAT_R16_UNORM;
		case Format_R16_UINT:
			return DXGI_FORMAT_R16_UINT;
		case Format_R16_SNORM:
			return DXGI_FORMAT_R16_SNORM;
		case Format_R16_SINT:
			return DXGI_FORMAT_R16_SINT;
		case Format_R8_UNORM:
			return DXGI_FORMAT_R8_UNORM;
		case Format_R8_UINT:
			return DXGI_FORMAT_R8_UINT;
		case Format_R8_SNORM:
			return DXGI_FORMAT_R8_SNORM;
		case Format_R8_SINT:
			return DXGI_FORMAT_R8_SINT;
		case Format_BC1_UNORM:
			return DXGI_FORMAT_BC1_UNORM;
		case Format_BC1_UNORM_SRGB:
			return DXGI_FORMAT_BC1_UNORM_SRGB;
		case Format_BC2_UNORM:
			return DXGI_FORMAT_BC2_UNORM;
		case Format_BC2_UNORM_SRGB:
			return DXGI_FORMAT_BC2_UNORM_SRGB;
		case Format_BC3_UNORM:
			return DXGI_FORMAT_BC3_UNORM;
		case Format_BC3_UNORM_SRGB:
			return DXGI_FORMAT_BC3_UNORM_SRGB;
		case Format_BC4_UNORM:
			return DXGI_FORMAT_BC4_UNORM;
		case Format_BC4_SNORM:
			return DXGI_FORMAT_BC4_SNORM;
		case Format_BC5_UNORM:
			return DXGI_FORMAT_BC5_UNORM;
		case Format_BC5_SNORM:
			return DXGI_FORMAT_BC5_SNORM;
		case Format_B8G8R8A8_UNORM:
			return DXGI_FORMAT_B8G8R8A8_UNORM;
		case Format_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		case Format_BC6H_UF16:
			return DXGI_FORMAT_BC6H_UF16;
		case Format_BC6H_SF16:
			return DXGI_FORMAT_BC6H_SF16;
		case Format_BC7_UNORM:
			return DXGI_FORMAT_BC7_UNORM;
		case Format_BC7_UNORM_SRGB:
			return DXGI_FORMAT_BC7_UNORM_SRGB;
		case Format_NV12:
			return DXGI_FORMAT_NV12;
		}
		return DXGI_FORMAT_UNKNOWN;
	}
	constexpr DXGI_FORMAT _ConvertFormatToTypeless(DXGI_FORMAT value)
	{
		switch (value)
		{
		case DXGI_FORMAT_R32G32B32A32_TYPELESS:
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
		case DXGI_FORMAT_R32G32B32A32_UINT:
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return DXGI_FORMAT_R32G32B32A32_TYPELESS;
		case DXGI_FORMAT_R32G32B32_TYPELESS:
		case DXGI_FORMAT_R32G32B32_FLOAT:
		case DXGI_FORMAT_R32G32B32_UINT:
		case DXGI_FORMAT_R32G32B32_SINT:
			return DXGI_FORMAT_R32G32B32_TYPELESS;
		case DXGI_FORMAT_R16G16B16A16_TYPELESS:
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
		case DXGI_FORMAT_R16G16B16A16_UINT:
		case DXGI_FORMAT_R16G16B16A16_SNORM:
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return DXGI_FORMAT_R16G16B16A16_TYPELESS;
		case DXGI_FORMAT_R32G32_TYPELESS:
		case DXGI_FORMAT_R32G32_FLOAT:
		case DXGI_FORMAT_R32G32_UINT:
		case DXGI_FORMAT_R32G32_SINT:
			return DXGI_FORMAT_R32G32_TYPELESS;
		case DXGI_FORMAT_R32G8X24_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
		case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
			return DXGI_FORMAT_R32G8X24_TYPELESS;
		case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
			return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;
		case DXGI_FORMAT_R10G10B10A2_TYPELESS:
		case DXGI_FORMAT_R10G10B10A2_UNORM:
		case DXGI_FORMAT_R10G10B10A2_UINT:
			return DXGI_FORMAT_R10G10B10A2_TYPELESS;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
		case DXGI_FORMAT_R16G16_TYPELESS:
		case DXGI_FORMAT_R16G16_FLOAT:
		case DXGI_FORMAT_R16G16_UNORM:
		case DXGI_FORMAT_R16G16_UINT:
		case DXGI_FORMAT_R16G16_SNORM:
		case DXGI_FORMAT_R16G16_SINT:
			return DXGI_FORMAT_R16G16_TYPELESS;
		case DXGI_FORMAT_R32_TYPELESS:
		case DXGI_FORMAT_D32_FLOAT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_SINT:
			return DXGI_FORMAT_R32_TYPELESS;
		case DXGI_FORMAT_R24G8_TYPELESS:
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
		case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
		case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
			return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
		case DXGI_FORMAT_R8G8_TYPELESS:
		case DXGI_FORMAT_R8G8_UNORM:
		case DXGI_FORMAT_R8G8_UINT:
		case DXGI_FORMAT_R8G8_SNORM:
		case DXGI_FORMAT_R8G8_SINT:
			return DXGI_FORMAT_R8G8_TYPELESS;
		case DXGI_FORMAT_R16_TYPELESS:
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_D16_UNORM:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			return DXGI_FORMAT_R16_TYPELESS;
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
			return DXGI_FORMAT_R8_TYPELESS;
		case DXGI_FORMAT_BC1_TYPELESS:
		case DXGI_FORMAT_BC1_UNORM:
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return DXGI_FORMAT_BC1_TYPELESS;
		case DXGI_FORMAT_BC2_TYPELESS:
		case DXGI_FORMAT_BC2_UNORM:
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return DXGI_FORMAT_BC2_TYPELESS;
		case DXGI_FORMAT_BC3_TYPELESS:
		case DXGI_FORMAT_BC3_UNORM:
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return DXGI_FORMAT_BC3_TYPELESS;
		case DXGI_FORMAT_BC4_TYPELESS:
		case DXGI_FORMAT_BC4_UNORM:
		case DXGI_FORMAT_BC4_SNORM:
			return DXGI_FORMAT_BC4_TYPELESS;
		case DXGI_FORMAT_BC5_TYPELESS:
		case DXGI_FORMAT_BC5_UNORM:
		case DXGI_FORMAT_BC5_SNORM:
			return DXGI_FORMAT_BC5_TYPELESS;
		case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8A8_TYPELESS;
		case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
			return DXGI_FORMAT_B8G8R8X8_TYPELESS;
		case DXGI_FORMAT_BC6H_TYPELESS:
		case DXGI_FORMAT_BC6H_UF16:
		case DXGI_FORMAT_BC6H_SF16:
			return DXGI_FORMAT_BC6H_TYPELESS;
		case DXGI_FORMAT_BC7_TYPELESS:
		case DXGI_FORMAT_BC7_UNORM:
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return DXGI_FORMAT_BC7_TYPELESS;
		default:
			return DXGI_FORMAT_UNKNOWN;
		}
	}
	constexpr D3D12_SUBRESOURCE_DATA _ConvertSubresourceData(const SubresourceData& pInitialData)
	{
		D3D12_SUBRESOURCE_DATA data = {};
		data.pData = pInitialData.data_ptr;
		data.RowPitch = pInitialData.row_pitch;
		data.SlicePitch = pInitialData.slice_pitch;

		return data;
	}
	constexpr D3D12_SHADER_VISIBILITY _ConvertShaderVisibility(ShaderStage value)
	{
		switch (value)
		{
		case ShaderStage_MS:
			return D3D12_SHADER_VISIBILITY_MESH;
		case ShaderStage_AS:
			return D3D12_SHADER_VISIBILITY_AMPLIFICATION;
		case ShaderStage_VS:
			return D3D12_SHADER_VISIBILITY_VERTEX;
		case ShaderStage_HS:
			return D3D12_SHADER_VISIBILITY_HULL;
		case ShaderStage_DS:
			return D3D12_SHADER_VISIBILITY_DOMAIN;
		case ShaderStage_GS:
			return D3D12_SHADER_VISIBILITY_GEOMETRY;
		case ShaderStage_PS:
			return D3D12_SHADER_VISIBILITY_PIXEL;
		default:
			return D3D12_SHADER_VISIBILITY_ALL;
		}
		return D3D12_SHADER_VISIBILITY_ALL;
	}
	constexpr D3D12_SHADING_RATE _ConvertShadingRate(ShadingRate value)
	{
		switch (value)
		{
		case ShadingRate_RATE_1X1:
			return D3D12_SHADING_RATE_1X1;
		case ShadingRate_RATE_1X2:
			return D3D12_SHADING_RATE_1X2;
		case ShadingRate_RATE_2X1:
			return D3D12_SHADING_RATE_2X1;
		case ShadingRate_RATE_2X2:
			return D3D12_SHADING_RATE_2X2;
		case ShadingRate_RATE_2X4:
			return D3D12_SHADING_RATE_2X4;
		case ShadingRate_RATE_4X2:
			return D3D12_SHADING_RATE_4X2;
		case ShadingRate_RATE_4X4:
			return D3D12_SHADING_RATE_4X4;
		default:
			return D3D12_SHADING_RATE_1X1;
		}
		return D3D12_SHADING_RATE_1X1;
	}
	constexpr UINT _ImageAspectToPlane(ImageAspect value)
	{
		switch (value)
		{
		default:
		case Protean::Graphics::ImageAspect_COLOR:
			return 0;
		case Protean::Graphics::ImageAspect_DEPTH:
			return 0;
		case Protean::Graphics::ImageAspect_STENCIL:
			return 1;
		case Protean::Graphics::ImageAspect_LUMINANCE:
			return 0;
		case Protean::Graphics::ImageAspect_CHROMINANCE:
			return 1;
		}
	}
	constexpr D3D12_SHADER_COMPONENT_MAPPING _ConvertComponentSwizzle(ComponentSwizzle value)
	{
		switch (value)
		{
		default:
		case Protean::Graphics::ComponentSwizzle_R:
			return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
		case Protean::Graphics::ComponentSwizzle_G:
			return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
		case Protean::Graphics::ComponentSwizzle_B:
			return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
		case Protean::Graphics::ComponentSwizzle_A:
			return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
		case Protean::Graphics::ComponentSwizzle_ZERO:
			return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
		case Protean::Graphics::ComponentSwizzle_ONE:
			return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
		}
	}
	constexpr UINT _ConvertSwizzle(Swizzle value)
	{
		return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(_ConvertComponentSwizzle(value.r), _ConvertComponentSwizzle(value.g), _ConvertComponentSwizzle(value.b), _ConvertComponentSwizzle(value.a));
	}
	constexpr D3D12_RESOURCE_DESC _ConvertTextureDesc(const TextureDesc& desc, bool casting_fully_typed_formats)
	{
		bool require_format_casting = has_flag(desc.misc_flags, ResourceMiscFlag_TYPED_FORMAT_CASTING);

		D3D12_RESOURCE_DESC resourcedesc = {};
		resourcedesc.Format = _ConvertFormat(desc.format);
		resourcedesc.Width = desc.width;
		resourcedesc.Height = desc.height;
		resourcedesc.MipLevels = GetMipCount(desc);
		resourcedesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourcedesc.DepthOrArraySize = (UINT16)desc.array_size;
		resourcedesc.SampleDesc.Count = desc.sample_count;
		resourcedesc.SampleDesc.Quality = 0;
		resourcedesc.Alignment = 0;
		resourcedesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (has_flag(desc.bind_flags, BindFlag_DEPTH_STENCIL))
		{
			resourcedesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			if (has_flag(desc.bind_flags, BindFlag_SHADER_RESOURCE))
			{
				require_format_casting = true;
			}
			else
			{
				resourcedesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}
		if (has_flag(desc.bind_flags, BindFlag_RENDER_TARGET))
		{
			resourcedesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (has_flag(desc.bind_flags, BindFlag_UNORDERED_ACCESS))
		{
			resourcedesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (!has_flag(desc.bind_flags, BindFlag_DEPTH_STENCIL) && resourcedesc.SampleDesc.Count <= 1)
		{
			// The copy and video queues have much stricter requirements to supported resource states, but they support
			//	implicit promotion from COMMON state. Because user is not allowed to set resource to COMMON state, we use this flag
			//	so textures automatically decay to COMMON state at the queue submit when they are left in a read-only state
			resourcedesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		}

#ifndef PLATFORM_XBOX
		if (has_flag(desc.misc_flags, ResourceMiscFlag_VIDEO_DECODE_DPB_ONLY))
		{
			resourcedesc.Flags = D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
		}
#endif // PLATFORM_XBOX

		switch (desc.type)
		{
		case TextureDesc::Type_TEXTURE_1D:
			resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			break;
		case TextureDesc::Type_TEXTURE_2D:
			resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			break;
		case TextureDesc::Type_TEXTURE_3D:
			resourcedesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			resourcedesc.DepthOrArraySize = (UINT16)desc.depth;
			break;
		default:
			assert(0);
			break;
		}

		if ((!casting_fully_typed_formats && require_format_casting) || has_flag(desc.misc_flags, ResourceMiscFlag_TYPELESS_FORMAT_CASTING))
		{
			resourcedesc.Format = _ConvertFormatToTypeless(resourcedesc.Format);
		}

#ifdef PLATFORM_XBOX
		Protean::Graphics::Xbox::ApplyTextureCreationFlags(desc, resourcedesc.Flags, D3D12_HEAP_FLAG_NONE);
#endif // PLATFORM_XBOX

		return resourcedesc;
	}

	// Native -> Engine converters
	constexpr Format _ConvertFormat_Inv(DXGI_FORMAT value)
	{
		switch (value)
		{
		case DXGI_FORMAT_UNKNOWN:
			return Format_UNKNOWN;
		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return Format_R32G32B32A32_FLOAT;
		case DXGI_FORMAT_R32G32B32A32_UINT:
			return Format_R32G32B32A32_UINT;
		case DXGI_FORMAT_R32G32B32A32_SINT:
			return Format_R32G32B32A32_SINT;
		case DXGI_FORMAT_R32G32B32_FLOAT:
			return Format_R32G32B32_FLOAT;
		case DXGI_FORMAT_R32G32B32_UINT:
			return Format_R32G32B32_UINT;
		case DXGI_FORMAT_R32G32B32_SINT:
			return Format_R32G32B32_SINT;
		case DXGI_FORMAT_R16G16B16A16_FLOAT:
			return Format_R16G16B16A16_FLOAT;
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			return Format_R16G16B16A16_UNORM;
		case DXGI_FORMAT_R16G16B16A16_UINT:
			return Format_R16G16B16A16_UINT;
		case DXGI_FORMAT_R16G16B16A16_SNORM:
			return Format_R16G16B16A16_SNORM;
		case DXGI_FORMAT_R16G16B16A16_SINT:
			return Format_R16G16B16A16_SINT;
		case DXGI_FORMAT_R32G32_FLOAT:
			return Format_R32G32_FLOAT;
		case DXGI_FORMAT_R32G32_UINT:
			return Format_R32G32_UINT;
		case DXGI_FORMAT_R32G32_SINT:
			return Format_R32G32_SINT;
		case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
			return Format_D32_FLOAT_S8X24_UINT;
		case DXGI_FORMAT_R10G10B10A2_UNORM:
			return Format_R10G10B10A2_UNORM;
		case DXGI_FORMAT_R10G10B10A2_UINT:
			return Format_R10G10B10A2_UINT;
		case DXGI_FORMAT_R11G11B10_FLOAT:
			return Format_R11G11B10_FLOAT;
		case DXGI_FORMAT_R8G8B8A8_UNORM:
			return Format_R8G8B8A8_UNORM;
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return Format_R8G8B8A8_UNORM_SRGB;
		case DXGI_FORMAT_R8G8B8A8_UINT:
			return Format_R8G8B8A8_UINT;
		case DXGI_FORMAT_R8G8B8A8_SNORM:
			return Format_R8G8B8A8_SNORM;
		case DXGI_FORMAT_R8G8B8A8_SINT:
			return Format_R8G8B8A8_SINT;
		case DXGI_FORMAT_R16G16_FLOAT:
			return Format_R16G16_FLOAT;
		case DXGI_FORMAT_R16G16_UNORM:
			return Format_R16G16_UNORM;
		case DXGI_FORMAT_R16G16_UINT:
			return Format_R16G16_UINT;
		case DXGI_FORMAT_R16G16_SNORM:
			return Format_R16G16_SNORM;
		case DXGI_FORMAT_R16G16_SINT:
			return Format_R16G16_SINT;
		case DXGI_FORMAT_D32_FLOAT:
			return Format_D32_FLOAT;
		case DXGI_FORMAT_R32_FLOAT:
			return Format_R32_FLOAT;
		case DXGI_FORMAT_R32_UINT:
			return Format_R32_UINT;
		case DXGI_FORMAT_R32_SINT:
			return Format_R32_SINT;
		case DXGI_FORMAT_D24_UNORM_S8_UINT:
			return Format_D24_UNORM_S8_UINT;
		case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
			return Format_R9G9B9E5_SHAREDEXP;
		case DXGI_FORMAT_R8G8_UNORM:
			return Format_R8G8_UNORM;
		case DXGI_FORMAT_R8G8_UINT:
			return Format_R8G8_UINT;
		case DXGI_FORMAT_R8G8_SNORM:
			return Format_R8G8_SNORM;
		case DXGI_FORMAT_R8G8_SINT:
			return Format_R8G8_SINT;
		case DXGI_FORMAT_R16_FLOAT:
			return Format_R16_FLOAT;
		case DXGI_FORMAT_D16_UNORM:
			return Format_D16_UNORM;
		case DXGI_FORMAT_R16_UNORM:
			return Format_R16_UNORM;
		case DXGI_FORMAT_R16_UINT:
			return Format_R16_UINT;
		case DXGI_FORMAT_R16_SNORM:
			return Format_R16_SNORM;
		case DXGI_FORMAT_R16_SINT:
			return Format_R16_SINT;
		case DXGI_FORMAT_R8_UNORM:
			return Format_R8_UNORM;
		case DXGI_FORMAT_R8_UINT:
			return Format_R8_UINT;
		case DXGI_FORMAT_R8_SNORM:
			return Format_R8_SNORM;
		case DXGI_FORMAT_R8_SINT:
			return Format_R8_SINT;
		case DXGI_FORMAT_BC1_UNORM:
			return Format_BC1_UNORM;
		case DXGI_FORMAT_BC1_UNORM_SRGB:
			return Format_BC1_UNORM_SRGB;
		case DXGI_FORMAT_BC2_UNORM:
			return Format_BC2_UNORM;
		case DXGI_FORMAT_BC2_UNORM_SRGB:
			return Format_BC2_UNORM_SRGB;
		case DXGI_FORMAT_BC3_UNORM:
			return Format_BC3_UNORM;
		case DXGI_FORMAT_BC3_UNORM_SRGB:
			return Format_BC3_UNORM_SRGB;
		case DXGI_FORMAT_BC4_UNORM:
			return Format_BC4_UNORM;
		case DXGI_FORMAT_BC4_SNORM:
			return Format_BC4_SNORM;
		case DXGI_FORMAT_BC5_UNORM:
			return Format_BC5_UNORM;
		case DXGI_FORMAT_BC5_SNORM:
			return Format_BC5_SNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM:
			return Format_B8G8R8A8_UNORM;
		case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
			return Format_B8G8R8A8_UNORM_SRGB;
		case DXGI_FORMAT_BC6H_UF16:
			return Format_BC6H_UF16;
		case DXGI_FORMAT_BC6H_SF16:
			return Format_BC6H_SF16;
		case DXGI_FORMAT_BC7_UNORM:
			return Format_BC7_UNORM;
		case DXGI_FORMAT_BC7_UNORM_SRGB:
			return Format_BC7_UNORM_SRGB;
		default:
			break;
		}
		return Format_UNKNOWN;
	}
	constexpr TextureDesc _ConvertTextureDesc_Inv(const D3D12_RESOURCE_DESC& desc)
	{
		TextureDesc retVal;

		switch (desc.Dimension)
		{
		case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
			retVal.type = TextureDesc::Type_TEXTURE_1D;
			retVal.array_size = desc.DepthOrArraySize;
			break;
		default:
		case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
			retVal.type = TextureDesc::Type_TEXTURE_2D;
			retVal.array_size = desc.DepthOrArraySize;
			break;
		case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
			retVal.type = TextureDesc::Type_TEXTURE_3D;
			retVal.depth = desc.DepthOrArraySize;
			break;
		}
		retVal.format = _ConvertFormat_Inv(desc.Format);
		retVal.width = (uint32_t)desc.Width;
		retVal.height = desc.Height;
		retVal.mip_levels = desc.MipLevels;

		return retVal;
	}

	struct RootSignatureOptimizer
	{
		static constexpr uint8_t INVALID_ROOT_PARAMETER = 0xFF;
		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* rootsig_desc = nullptr;
		// This is the bitflag of all root parameters:
		uint64_t root_mask = 0ull;
		// These map shader registers in the binding space (space=0) to root parameters
		uint8_t CBV[DESCRIPTORBINDER_CBV_COUNT];
		uint8_t SRV[DESCRIPTORBINDER_SRV_COUNT];
		uint8_t UAV[DESCRIPTORBINDER_UAV_COUNT];
		uint8_t SAM[DESCRIPTORBINDER_SAMPLER_COUNT];
		uint8_t PUSH;
		// For each root parameter, store some statistics:
		struct RootParameterStatistics
		{
			uint16_t descriptorCopyCount = 0;
			uint16_t is_sampler = 0;
		};
		StackVector<RootParameterStatistics, 64> root_stats;

		void init(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc)
		{
			rootsig_desc = &desc;

			// First, initialize all to point to invalid root parameter:
			for (size_t i = 0; i < arraysize(CBV); ++i)
			{
				CBV[i] = INVALID_ROOT_PARAMETER;
			}
			for (size_t i = 0; i < arraysize(SRV); ++i)
			{
				SRV[i] = INVALID_ROOT_PARAMETER;
			}
			for (size_t i = 0; i < arraysize(UAV); ++i)
			{
				UAV[i] = INVALID_ROOT_PARAMETER;
			}
			for (size_t i = 0; i < arraysize(SAM); ++i)
			{
				SAM[i] = INVALID_ROOT_PARAMETER;
			}
			PUSH = INVALID_ROOT_PARAMETER;

			assert(desc.Desc_1_1.NumParameters < 64u); // root parameter indices should fit into 64-bit root_mask
			root_stats.resize(desc.Desc_1_1.NumParameters); // one stat for each root parameter
			for (UINT root_parameter_index = 0; root_parameter_index < desc.Desc_1_1.NumParameters; ++root_parameter_index)
			{
				const D3D12_ROOT_PARAMETER1& param = desc.Desc_1_1.pParameters[root_parameter_index];
				RootParameterStatistics& stats = root_stats[root_parameter_index];

				if (param.ParameterType != D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) // root constant dirtyness is not tracked, because those are set immediately
				{
					root_mask |= 1ull << root_parameter_index;
				}

				switch (param.ParameterType)
				{
				case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
					for (UINT range_index = 0; range_index < param.DescriptorTable.NumDescriptorRanges; ++range_index)
					{
						const D3D12_DESCRIPTOR_RANGE1& range = param.DescriptorTable.pDescriptorRanges[range_index];
						stats.descriptorCopyCount += range.NumDescriptors == UINT_MAX ? 0 : range.NumDescriptors;
						stats.is_sampler = range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

						if (range.RegisterSpace != 0)
							continue; // we only care for the binding space (space=0)

						switch (range.RangeType)
						{
						case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
							for (UINT i = 0; i < range.NumDescriptors; ++i)
							{
								CBV[range.BaseShaderRegister + i] = (uint8_t)root_parameter_index;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
							for (UINT i = 0; i < range.NumDescriptors; ++i)
							{
								SRV[range.BaseShaderRegister + i] = (uint8_t)root_parameter_index;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
							for (UINT i = 0; i < range.NumDescriptors; ++i)
							{
								UAV[range.BaseShaderRegister + i] = (uint8_t)root_parameter_index;
							}
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
							for (UINT i = 0; i < range.NumDescriptors; ++i)
							{
								SAM[range.BaseShaderRegister + i] = (uint8_t)root_parameter_index;
							}
							break;
						default:
							assert(0);
							break;
						}
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
					if (param.Constants.RegisterSpace == 0)
					{
						assert(PUSH == INVALID_ROOT_PARAMETER); // check that push constant block was not already set, because only one is supported currently
						PUSH = (uint8_t)root_parameter_index;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_CBV:
					if (param.Descriptor.RegisterSpace == 0)
					{
						assert(param.Descriptor.ShaderRegister < arraysize(CBV));
						CBV[param.Descriptor.ShaderRegister] = (uint8_t)root_parameter_index;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_SRV:
					if (param.Descriptor.RegisterSpace == 0)
					{
						assert(param.Descriptor.ShaderRegister < arraysize(CBV));
						SRV[param.Descriptor.ShaderRegister] = (uint8_t)root_parameter_index;
					}
					break;
				case D3D12_ROOT_PARAMETER_TYPE_UAV:
					if (param.Descriptor.RegisterSpace == 0)
					{
						assert(param.Descriptor.ShaderRegister < arraysize(CBV));
						UAV[param.Descriptor.ShaderRegister] = (uint8_t)root_parameter_index;
					}
					break;
				default:
					assert(0);
					break;
				}
			}

		}
	};

	struct SingleDescriptor
	{
		Protean::Allocator::shared_ptr<D3D12Device::AllocationHandler> allocationhandler;
		D3D12_CPU_DESCRIPTOR_HANDLE handle = {};
		D3D12_DESCRIPTOR_HEAP_TYPE type = {};
		int index = -1; // bindless

		bool IsValid() const { return handle.ptr != 0; }
		void init(const D3D12Device* device, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbv)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateConstantBufferView(&cbv, handle);

			// bindless allocation:
			allocationhandler->destroylocker.lock();
			if (!allocationhandler->free_bindless_res.empty())
			{
				index = allocationhandler->free_bindless_res.back();
				allocationhandler->free_bindless_res.pop_back();
			}
			allocationhandler->destroylocker.unlock();
			if (index >= 0)
			{
				assert(index < BINDLESS_RESOURCE_CAPACITY);
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const D3D12Device* device, const D3D12_SHADER_RESOURCE_VIEW_DESC& srv, ID3D12Resource* res)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateShaderResourceView(res, &srv, handle);

			// bindless allocation:
			allocationhandler->destroylocker.lock();
			if (!allocationhandler->free_bindless_res.empty())
			{
				index = allocationhandler->free_bindless_res.back();
				allocationhandler->free_bindless_res.pop_back();
			}
			allocationhandler->destroylocker.unlock();
			if (index >= 0)
			{
				assert(index < BINDLESS_RESOURCE_CAPACITY);
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const D3D12Device* device, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uav, ID3D12Resource* res)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			handle = allocationhandler->descriptors_res.allocate();
			allocationhandler->device->CreateUnorderedAccessView(res, nullptr, &uav, handle);

			// bindless allocation:
			allocationhandler->destroylocker.lock();
			if (!allocationhandler->free_bindless_res.empty())
			{
				index = allocationhandler->free_bindless_res.back();
				allocationhandler->free_bindless_res.pop_back();
			}
			allocationhandler->destroylocker.unlock();
			if (index >= 0)
			{
				assert(index < BINDLESS_RESOURCE_CAPACITY);
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_res.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const D3D12Device* device, const D3D12_SAMPLER_DESC& sam)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
			handle = allocationhandler->descriptors_sam.allocate();
			allocationhandler->device->CreateSampler(&sam, handle);

			// bindless allocation:
			allocationhandler->destroylocker.lock();
			if (!allocationhandler->free_bindless_sam.empty())
			{
				index = allocationhandler->free_bindless_sam.back();
				allocationhandler->free_bindless_sam.pop_back();
			}
			allocationhandler->destroylocker.unlock();
			if (index >= 0)
			{
				assert(index < BINDLESS_SAMPLER_CAPACITY);
				D3D12_CPU_DESCRIPTOR_HANDLE dst_bindless = device->descriptorheap_sam.start_cpu;
				dst_bindless.ptr += index * allocationhandler->device->GetDescriptorHandleIncrementSize(type);
				allocationhandler->device->CopyDescriptorsSimple(1, dst_bindless, handle, type);
			}
		}
		void init(const D3D12Device* device, const D3D12_RENDER_TARGET_VIEW_DESC& rtv, ID3D12Resource* res)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			handle = allocationhandler->descriptors_rtv.allocate();
			allocationhandler->device->CreateRenderTargetView(res, &rtv, handle);
		}
		void init(const D3D12Device* device, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsv, ID3D12Resource* res)
		{
			this->allocationhandler = device->allocationhandler;
			type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			handle = allocationhandler->descriptors_dsv.allocate();
			allocationhandler->device->CreateDepthStencilView(res, &dsv, handle);
		}
		void destroy()
		{
			if (IsValid())
			{
				switch (type)
				{
				case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
					allocationhandler->descriptors_res.free(handle);

					// bindless free:
					if (index >= 0)
					{
						allocationhandler->destroyer_bindless_res.push_back(std::make_pair(index, allocationhandler->framecount));
					}
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
					allocationhandler->descriptors_sam.free(handle);

					// bindless free:
					if (index >= 0)
					{
						allocationhandler->destroyer_bindless_sam.push_back(std::make_pair(index, allocationhandler->framecount));
					}
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
					allocationhandler->descriptors_rtv.free(handle);
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
					allocationhandler->descriptors_dsv.free(handle);
					break;
				case D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES:
				default:
					assert(0);
					break;
				}
			}
			handle = {};
			index = -1;
			type = {};
		}

		// These will allow to use D3D12CalcSubresource() and loop over D3D12 subresource indices which the descriptor includes:
		uint32_t firstMip = 0;
		uint32_t mipCount = 0;
		uint32_t firstSlice = 0;
		uint32_t sliceCount = 0;
		uint64_t buffer_offset = 0;
	};

	struct Resource_DX12
	{
		Protean::Allocator::shared_ptr<D3D12Device::AllocationHandler> allocationhandler;
		ComPtr<D3D12MA::Allocation> allocation;
		ComPtr<ID3D12Resource> resource;
		SingleDescriptor srv;
		SingleDescriptor uav;
		Protean::vector<SingleDescriptor> subresources_srv;
		Protean::vector<SingleDescriptor> subresources_uav;
		SingleDescriptor uav_raw;

		D3D12_GPU_VIRTUAL_ADDRESS gpu_address = 0;

		SingleDescriptor rtv = {};
		SingleDescriptor dsv = {};
		Protean::vector<SingleDescriptor> subresources_rtv;
		Protean::vector<SingleDescriptor> subresources_dsv;
		Protean::vector<SubresourceData> mapped_subresources;
		UINT64 total_size = 0;
		Protean::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;
		Protean::vector<UINT64> rowSizesInBytes;
		Protean::vector<UINT> numRows;
		SparseTextureProperties sparse_texture_properties;

		void destroy_subresources()
		{
			srv.destroy();
			uav.destroy();
			rtv.destroy();
			dsv.destroy();
			for (auto& x : subresources_srv)
			{
				x.destroy();
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				x.destroy();
			}
			subresources_uav.clear();
			for (auto& x : subresources_rtv)
			{
				x.destroy();
			}
			subresources_rtv.clear();
			for (auto& x : subresources_dsv)
			{
				x.destroy();
			}
			subresources_dsv.clear();
		}

		~Resource_DX12()
		{
			std::scoped_lock lck(allocationhandler->destroylocker);
			uint64_t framecount = allocationhandler->framecount;
			if (allocation) allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
			if (resource) allocationhandler->destroyer_resources.push_back(std::make_pair(resource, framecount));
			destroy_subresources();
		}
	};
	struct Sampler_DX12
	{
		Protean::Allocator::shared_ptr<D3D12Device::AllocationHandler> allocationhandler;
		SingleDescriptor descriptor;

		~Sampler_DX12()
		{
			std::scoped_lock lck(allocationhandler->destroylocker);
			uint64_t framecount = allocationhandler->framecount;
			descriptor.destroy();
		}
	};
	struct QueryHeap_DX12
	{
		Protean::Allocator::shared_ptr<D3D12Device::AllocationHandler> allocationhandler;
		ComPtr<ID3D12QueryHeap> heap;

		~QueryHeap_DX12()
		{
			std::scoped_lock lck(allocationhandler->destroylocker);
			uint64_t framecount = allocationhandler->framecount;
			if (heap) allocationhandler->destroyer_queryheaps.push_back(std::make_pair(heap, framecount));
		}
	};
	struct PipelineState_DX12
	{
		Protean::Allocator::shared_ptr<D3D12Device::AllocationHandler> allocationhandler;
		ComPtr<ID3D12PipelineState> resource;
		ComPtr<ID3D12RootSignature> rootSignature;

		Protean::vector<uint8_t> shadercode;
		D3D_PRIMITIVE_TOPOLOGY primitiveTopology;

		ComPtr<ID3D12VersionedRootSignatureDeserializer> rootsig_deserializer;
		const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* rootsig_desc = nullptr;
		Protean::Allocator::shared_ptr<void> rootsig_desc_lifetime_extender;
		RootSignatureOptimizer rootsig_optimizer;

		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawInstancedIndirectCountCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndexedInstancedIndirectCountCommandSignature;
		Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchMeshIndirectCountCommandSignature;

		struct PSO_STREAM
		{
			struct PSO_STREAM1
			{
				CD3DX12_PIPELINE_STATE_STREAM_FLAGS Flags;
				CD3DX12_PIPELINE_STATE_STREAM_VS VS;
				CD3DX12_PIPELINE_STATE_STREAM_HS HS;
				CD3DX12_PIPELINE_STATE_STREAM_DS DS;
				CD3DX12_PIPELINE_STATE_STREAM_GS GS;
				CD3DX12_PIPELINE_STATE_STREAM_PS PS;
				CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER RS;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL1 DSS;
				CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC BD;
				CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PT;
				CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT IL;
				CD3DX12_PIPELINE_STATE_STREAM_IB_STRIP_CUT_VALUE STRIP;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSFormat;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS Formats;
				CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
				CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_MASK SampleMask;
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE ROOTSIG;
			} stream1 = {};

			struct PSO_STREAM2
			{
				CD3DX12_PIPELINE_STATE_STREAM_MS MS;
				CD3DX12_PIPELINE_STATE_STREAM_AS AS;
			} stream2 = {};
		} stream = {};

		~PipelineState_DX12()
		{
			std::scoped_lock lck(allocationhandler->destroylocker);
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_pipelines.push_back(std::make_pair(resource, framecount));
			if (rootSignature) allocationhandler->destroyer_rootSignatures.push_back(std::make_pair(rootSignature, framecount));
		}
	};
	struct BVH_DX12 final : public Resource_DX12
	{
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS desc = {};
		Protean::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		GPUBuffer scratch;
	};
	struct RTPipelineState_DX12
	{
		Protean::Allocator::shared_ptr<D3D12Device::AllocationHandler> allocationhandler;
		ComPtr<ID3D12StateObject> resource;

		ComPtr<ID3D12StateObjectProperties> stateObjectProperties;

		Protean::vector<std::wstring> export_strings;
		Protean::vector<D3D12_EXPORT_DESC> exports;
		Protean::vector<D3D12_DXIL_LIBRARY_DESC> library_descs;
		Protean::vector<std::wstring> group_strings;
		Protean::vector<D3D12_HIT_GROUP_DESC> hitgroup_descs;

		~RTPipelineState_DX12()
		{
			std::scoped_lock lck(allocationhandler->destroylocker);
			uint64_t framecount = allocationhandler->framecount;
			if (resource) allocationhandler->destroyer_stateobjects.push_back(std::make_pair(resource, framecount));
		}
	};
	struct SwapChain_DX12
	{
		Protean::Allocator::shared_ptr<D3D12Device::AllocationHandler> allocationhandler;
#ifdef PLATFORM_XBOX
		uint32_t bufferIndex = 0;
#else
		ComPtr<IDXGISwapChain3> swapChain;
#endif // PLATFORM_XBOX
		Protean::vector<Protean::Allocator::shared_ptr<Resource_DX12>> textures; // shared_ptr is used because they can be given out by GetBackBuffer()

		Texture dummyTexture;
		ColorSpace colorSpace = ColorSpace_SRGB;

		inline uint32_t GetBufferIndex() const
		{
#ifdef PLATFORM_XBOX
			return bufferIndex;
#else
			return swapChain->GetCurrentBackBufferIndex();
#endif // PLATFORM_XBOX
		}
	};
	struct VideoDecoder_DX12
	{
		Protean::Allocator::shared_ptr<D3D12Device::AllocationHandler> allocationhandler;
		ComPtr<ID3D12VideoDecoderHeap> decoder_heap;
		ComPtr<ID3D12VideoDecoder> decoder;

		~VideoDecoder_DX12()
		{
			std::scoped_lock lck(allocationhandler->destroylocker);
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_video_decoder_heaps.push_back(std::make_pair(decoder_heap, framecount));
			allocationhandler->destroyer_video_decoders.push_back(std::make_pair(decoder, framecount));
		}
	};

	template<typename T> struct DX12Type;
	template<> struct DX12Type<GPUResource> { using type = Resource_DX12; };
	template<> struct DX12Type<GPUBuffer> { using type = Resource_DX12; };
	template<> struct DX12Type<Texture> { using type = Resource_DX12; };
	template<> struct DX12Type<Sampler> { using type = Sampler_DX12; };
	template<> struct DX12Type<GPUQueryHeap> { using type = QueryHeap_DX12; };
	template<> struct DX12Type<Shader> { using type = PipelineState_DX12; };
	template<> struct DX12Type<PipelineState> { using type = PipelineState_DX12; };
	template<> struct DX12Type<RaytracingAccelerationStructure> { using type = BVH_DX12; };
	template<> struct DX12Type<RaytracingPipelineState> { using type = RTPipelineState_DX12; };
	template<> struct DX12Type<SwapChain> { using type = SwapChain_DX12; };
	template<> struct DX12Type<VideoDecoder> { using type = VideoDecoder_DX12; };

	template<typename T>
	typename DX12Type<T>::type* to_internal(const T* param)
	{
		return static_cast<typename DX12Type<T>::type*>(param->internal_state.get());
	}

	template<typename T>
	typename DX12Type<T>::type* to_internal(const GPUResource* res)
	{
		return static_cast<typename DX12Type<T>::type*>(res->internal_state.get());
	}

#ifdef PLATFORM_WINDOWS_DESKTOP
	inline void HandleDeviceRemoved(PVOID context, BOOLEAN)
	{
		D3D12Device* removedDevice = (D3D12Device*)context;
		removedDevice->OnDeviceRemoved();
	}
#endif // PLATFORM_WINDOWS_DESKTOP
}
}

#endif // WICKEDENGINE_BUILD_DX12
