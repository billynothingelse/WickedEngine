#pragma once
// Internal header shared by the Metal backend translation units.
// Not part of the public graphics API, include rhi/rhi.h instead.
#ifdef __APPLE__
#include "rhi/metal/metal_device.h"
#include "core/timer.h"
#include "debug/backlog.h"

#include <Metal/MTL4AccelerationStructure.hpp>
#include <VideoToolbox/VideoToolbox.h>
#include <CoreMedia/CoreMedia.h>
#include <CoreVideo/CoreVideo.h>

__asm__(".linker_option \"-framework\", \"Foundation\"");
__asm__(".linker_option \"-framework\", \"QuartzCore\"");
__asm__(".linker_option \"-framework\", \"Metal\"");
__asm__(".linker_option \"-framework\", \"CoreFoundation\"");
__asm__(".linker_option \"-framework\", \"CoreVideo\"");
__asm__(".linker_option \"-framework\", \"CoreMedia\"");
__asm__(".linker_option \"-framework\", \"VideoToolbox\"");

#include "codecs/h264.h"

namespace Protean::Graphics
{

namespace MetalInternal
{
	static constexpr MTL::SparsePageSize sparse_page_size = MTL::SparsePageSize256;

	constexpr MTL::AttributeFormat _ConvertAttributeFormat(Format value)
	{
		switch (value) {
			case Format_R32G32B32A32_FLOAT:
				return MTL::AttributeFormatFloat4;
			case Format_R32G32B32_FLOAT:
				return MTL::AttributeFormatFloat3;
			case Format_R32G32_FLOAT:
				return MTL::AttributeFormatFloat2;
			case Format_R32_FLOAT:
				return MTL::AttributeFormatFloat;
				
			case Format_R16G16B16A16_FLOAT:
				return MTL::AttributeFormatHalf4;
			case Format_R16G16_FLOAT:
				return MTL::AttributeFormatHalf2;
			case Format_R16_FLOAT:
				return MTL::AttributeFormatHalf;
				
			case Format_R32G32B32A32_UINT:
				return MTL::AttributeFormatUInt4;
			case Format_R32G32B32_UINT:
				return MTL::AttributeFormatUInt3;
			case Format_R32G32_UINT:
				return MTL::AttributeFormatUInt2;
			case Format_R32_UINT:
				return MTL::AttributeFormatUInt;
				
			case Format_R32G32B32A32_SINT:
				return MTL::AttributeFormatInt4;
			case Format_R32G32B32_SINT:
				return MTL::AttributeFormatInt3;
			case Format_R32G32_SINT:
				return MTL::AttributeFormatInt2;
			case Format_R32_SINT:
				return MTL::AttributeFormatInt;
				
			case Format_R16G16B16A16_UINT:
				return MTL::AttributeFormatUShort4;
			case Format_R16G16_UINT:
				return MTL::AttributeFormatUShort2;
			case Format_R16_UINT:
				return MTL::AttributeFormatUShort;
				
			case Format_R16G16B16A16_SINT:
				return MTL::AttributeFormatShort4;
			case Format_R16G16_SINT:
				return MTL::AttributeFormatShort2;
			case Format_R16_SINT:
				return MTL::AttributeFormatShort;
				
			case Format_R16G16B16A16_UNORM:
				return MTL::AttributeFormatUShort4Normalized;
			case Format_R16G16_UNORM:
				return MTL::AttributeFormatUShort2Normalized;
			case Format_R16_UNORM:
				return MTL::AttributeFormatUShortNormalized;
				
			case Format_R16G16B16A16_SNORM:
				return MTL::AttributeFormatShort4Normalized;
			case Format_R16G16_SNORM:
				return MTL::AttributeFormatShort2Normalized;
			case Format_R16_SNORM:
				return MTL::AttributeFormatShortNormalized;
				
			case Format_R8G8B8A8_UINT:
				return MTL::AttributeFormatUChar4;
			case Format_R8G8_UINT:
				return MTL::AttributeFormatUChar2;
			case Format_R8_UINT:
				return MTL::AttributeFormatUChar;
				
			case Format_B8G8R8A8_UNORM:
				return MTL::AttributeFormatUChar4Normalized_BGRA;
			case Format_R8G8B8A8_UNORM:
				return MTL::AttributeFormatUChar4Normalized;
			case Format_R8G8_UNORM:
				return MTL::AttributeFormatUChar2Normalized;
			case Format_R8_UNORM:
				return MTL::AttributeFormatUCharNormalized;
				
			case Format_R8G8B8A8_SNORM:
				return MTL::AttributeFormatChar4Normalized;
			case Format_R8G8_SNORM:
				return MTL::AttributeFormatChar2Normalized;
			case Format_R8_SNORM:
				return MTL::AttributeFormatCharNormalized;
				
			case Format_R10G10B10A2_UNORM:
				return MTL::AttributeFormatUInt1010102Normalized;
			case Format_R11G11B10_FLOAT:
				return MTL::AttributeFormatFloatRG11B10;
			case Format_R9G9B9E5_SHAREDEXP:
				return MTL::AttributeFormatFloatRGB9E5;
				
			default:
				break;
		}
		return MTL::AttributeFormatInvalid;
	}
	constexpr MTL::VertexFormat _ConvertVertexFormat(Format value)
	{
		switch (value)
		{
		default:
		case Format_UNKNOWN:
			return MTL::VertexFormatInvalid;
		case Format_R32G32B32A32_FLOAT:
			return MTL::VertexFormatFloat4;
		case Format_R32G32B32A32_UINT:
			return MTL::VertexFormatUInt4;
		case Format_R32G32B32A32_SINT:
			return MTL::VertexFormatInt4;
		case Format_R32G32B32_FLOAT:
			return MTL::VertexFormatFloat3;
		case Format_R32G32B32_UINT:
			return MTL::VertexFormatUInt3;
		case Format_R32G32B32_SINT:
			return MTL::VertexFormatInt3;
		case Format_R16G16B16A16_FLOAT:
			return MTL::VertexFormatHalf4;
		case Format_R16G16B16A16_UNORM:
			return MTL::VertexFormatUShort4Normalized;
		case Format_R16G16B16A16_UINT:
			return MTL::VertexFormatUShort4;
		case Format_R16G16B16A16_SNORM:
			return MTL::VertexFormatShort4Normalized;
		case Format_R16G16B16A16_SINT:
			return MTL::VertexFormatShort4;
		case Format_R32G32_FLOAT:
			return MTL::VertexFormatFloat2;
		case Format_R32G32_UINT:
			return MTL::VertexFormatUInt2;
		case Format_R32G32_SINT:
			return MTL::VertexFormatInt2;
		case Format_D32_FLOAT_S8X24_UINT:
			return MTL::VertexFormatInvalid;
		case Format_R10G10B10A2_UNORM:
			return MTL::VertexFormatUInt1010102Normalized;
		case Format_R10G10B10A2_UINT:
			return MTL::VertexFormatInvalid;
		case Format_R11G11B10_FLOAT:
			return MTL::VertexFormatFloatRG11B10;
		case Format_R8G8B8A8_UNORM:
			return MTL::VertexFormatUChar4Normalized;
		case Format_R8G8B8A8_UNORM_SRGB:
			return MTL::VertexFormatInvalid;
		case Format_R8G8B8A8_UINT:
			return MTL::VertexFormatUChar4;
		case Format_R8G8B8A8_SNORM:
			return MTL::VertexFormatChar4Normalized;
		case Format_R8G8B8A8_SINT:
			return MTL::VertexFormatChar4;
		case Format_R16G16_FLOAT:
			return MTL::VertexFormatHalf2;
		case Format_R16G16_UNORM:
			return MTL::VertexFormatUShort2Normalized;
		case Format_R16G16_UINT:
			return MTL::VertexFormatUShort2;
		case Format_R16G16_SNORM:
			return MTL::VertexFormatShort2Normalized;
		case Format_R16G16_SINT:
			return MTL::VertexFormatShort2;
		case Format_D32_FLOAT:
			return MTL::VertexFormatInvalid;
		case Format_R32_FLOAT:
			return MTL::VertexFormatFloat;
		case Format_R32_UINT:
			return MTL::VertexFormatUInt;
		case Format_R32_SINT:
			return MTL::VertexFormatInt;
		case Format_D24_UNORM_S8_UINT:
			return MTL::VertexFormatInvalid;
		case Format_R9G9B9E5_SHAREDEXP:
			return MTL::VertexFormatFloatRGB9E5;
		case Format_R8G8_UNORM:
			return MTL::VertexFormatUChar2Normalized;
		case Format_R8G8_UINT:
			return MTL::VertexFormatUChar2;
		case Format_R8G8_SNORM:
			return MTL::VertexFormatChar2Normalized;
		case Format_R8G8_SINT:
			return MTL::VertexFormatChar2;
		case Format_R16_FLOAT:
			return MTL::VertexFormatHalf;
		case Format_D16_UNORM:
			return MTL::VertexFormatInvalid;
		case Format_R16_UNORM:
			return MTL::VertexFormatUShortNormalized;
		case Format_R16_UINT:
			return MTL::VertexFormatUShort;
		case Format_R16_SNORM:
			return MTL::VertexFormatShortNormalized;
		case Format_R16_SINT:
			return MTL::VertexFormatShort;
		case Format_R8_UNORM:
			return MTL::VertexFormatUCharNormalized;
		case Format_R8_UINT:
			return MTL::VertexFormatUChar;
		case Format_R8_SNORM:
			return MTL::VertexFormatCharNormalized;
		case Format_R8_SINT:
			return MTL::VertexFormatChar;
		}
		return MTL::VertexFormatInvalid;
	}
	constexpr MTL::PixelFormat _ConvertPixelFormat(Format value)
	{
	   switch (value)
	   {
	   case Format_UNKNOWN:
		   return MTL::PixelFormatInvalid;
	   case Format_R32G32B32A32_FLOAT:
		   return MTL::PixelFormatRGBA32Float;
	   case Format_R32G32B32A32_UINT:
		   return MTL::PixelFormatRGBA32Uint;
	   case Format_R32G32B32A32_SINT:
		   return MTL::PixelFormatRGBA32Sint;
	   case Format_R32G32B32_FLOAT:
		   return MTL::PixelFormatInvalid;
	   case Format_R32G32B32_UINT:
		   return MTL::PixelFormatInvalid;
	   case Format_R32G32B32_SINT:
		   return MTL::PixelFormatInvalid;
	   case Format_R16G16B16A16_FLOAT:
		   return MTL::PixelFormatRGBA16Float;
	   case Format_R16G16B16A16_UNORM:
		   return MTL::PixelFormatRGBA16Unorm;
	   case Format_R16G16B16A16_UINT:
		   return MTL::PixelFormatRGBA16Uint;
	   case Format_R16G16B16A16_SNORM:
		   return MTL::PixelFormatRGBA16Snorm;
	   case Format_R16G16B16A16_SINT:
		   return MTL::PixelFormatRGBA16Sint;
	   case Format_R32G32_FLOAT:
		   return MTL::PixelFormatRG32Float;
	   case Format_R32G32_UINT:
		   return MTL::PixelFormatRG32Uint;
	   case Format_R32G32_SINT:
		   return MTL::PixelFormatRG32Sint;
	   case Format_D32_FLOAT_S8X24_UINT:
		   return MTL::PixelFormatDepth32Float_Stencil8;
	   case Format_R10G10B10A2_UNORM:
		   return MTL::PixelFormatBGR10A2Unorm;
	   case Format_R10G10B10A2_UINT:
		   return MTL::PixelFormatInvalid;
	   case Format_R11G11B10_FLOAT:
		   return MTL::PixelFormatRG11B10Float;
	   case Format_R8G8B8A8_UNORM:
		   return MTL::PixelFormatRGBA8Unorm;
	   case Format_R8G8B8A8_UNORM_SRGB:
		   return MTL::PixelFormatRGBA8Unorm_sRGB;
	   case Format_R8G8B8A8_UINT:
		   return MTL::PixelFormatRGBA8Uint;
	   case Format_R8G8B8A8_SNORM:
		   return MTL::PixelFormatRGBA8Snorm;
	   case Format_R8G8B8A8_SINT:
		   return MTL::PixelFormatRGBA8Sint;
	   case Format_R16G16_FLOAT:
		   return MTL::PixelFormatRG16Float;
	   case Format_R16G16_UNORM:
		   return MTL::PixelFormatRG16Unorm;
	   case Format_R16G16_UINT:
		   return MTL::PixelFormatRG16Uint;
	   case Format_R16G16_SNORM:
		   return MTL::PixelFormatRG16Snorm;
	   case Format_R16G16_SINT:
		   return MTL::PixelFormatRG16Sint;
	   case Format_D32_FLOAT:
		   return MTL::PixelFormatDepth32Float;
	   case Format_R32_FLOAT:
		   return MTL::PixelFormatR32Float;
	   case Format_R32_UINT:
		   return MTL::PixelFormatR32Uint;
	   case Format_R32_SINT:
		   return MTL::PixelFormatR32Sint;
	   case Format_D24_UNORM_S8_UINT:
		   return MTL::PixelFormatDepth24Unorm_Stencil8;
	   case Format_R9G9B9E5_SHAREDEXP:
		   return MTL::PixelFormatRGB9E5Float;
	   case Format_R8G8_UNORM:
		   return MTL::PixelFormatRG8Unorm;
	   case Format_R8G8_UINT:
		   return MTL::PixelFormatRG8Uint;
	   case Format_R8G8_SNORM:
		   return MTL::PixelFormatRG8Snorm;
	   case Format_R8G8_SINT:
		   return MTL::PixelFormatRG8Sint;
	   case Format_R16_FLOAT:
		   return MTL::PixelFormatR16Float;
	   case Format_D16_UNORM:
		   return MTL::PixelFormatDepth16Unorm;
	   case Format_R16_UNORM:
		   return MTL::PixelFormatR16Unorm;
	   case Format_R16_UINT:
		   return MTL::PixelFormatR16Uint;
	   case Format_R16_SNORM:
		   return MTL::PixelFormatR16Snorm;
	   case Format_R16_SINT:
		   return MTL::PixelFormatR16Sint;
	   case Format_R8_UNORM:
		   return MTL::PixelFormatR8Unorm;
	   case Format_R8_UINT:
		   return MTL::PixelFormatR8Uint;
	   case Format_R8_SNORM:
		   return MTL::PixelFormatR8Snorm;
	   case Format_R8_SINT:
		   return MTL::PixelFormatR8Sint;
	   case Format_BC1_UNORM:
		   return MTL::PixelFormatBC1_RGBA;
	   case Format_BC1_UNORM_SRGB:
		   return MTL::PixelFormatBC1_RGBA_sRGB;
	   case Format_BC2_UNORM:
		   return MTL::PixelFormatBC2_RGBA;
	   case Format_BC2_UNORM_SRGB:
		   return MTL::PixelFormatBC2_RGBA_sRGB;
	   case Format_BC3_UNORM:
		   return MTL::PixelFormatBC3_RGBA;
	   case Format_BC3_UNORM_SRGB:
		   return MTL::PixelFormatBC3_RGBA_sRGB;
	   case Format_BC4_UNORM:
		   return MTL::PixelFormatBC4_RUnorm;
	   case Format_BC4_SNORM:
		   return MTL::PixelFormatBC4_RSnorm;
	   case Format_BC5_UNORM:
		   return MTL::PixelFormatBC5_RGUnorm;
	   case Format_BC5_SNORM:
		   return MTL::PixelFormatBC5_RGSnorm;
	   case Format_B8G8R8A8_UNORM:
		   return MTL::PixelFormatBGRA8Unorm;
	   case Format_B8G8R8A8_UNORM_SRGB:
		   return MTL::PixelFormatBGRA8Unorm_sRGB;
	   case Format_BC6H_UF16:
		   return MTL::PixelFormatBC6H_RGBUfloat;
	   case Format_BC6H_SF16:
		   return MTL::PixelFormatBC6H_RGBFloat;
	   case Format_BC7_UNORM:
		   return MTL::PixelFormatBC7_RGBAUnorm;
	   case Format_BC7_UNORM_SRGB:
		   return MTL::PixelFormatBC7_RGBAUnorm_sRGB;
	   case Format_NV12:
		   return MTL::PixelFormatInvalid;
	   }
	   return MTL::PixelFormatInvalid;
	}
	constexpr MTL::BlendOperation _ConvertBlendOp(BlendOp value)
	{
		switch (value)
		{
			case BlendOp_ADD:
				return MTL::BlendOperationAdd;
			case BlendOp_SUBTRACT:
				return MTL::BlendOperationSubtract;
			case BlendOp_REV_SUBTRACT:
				return MTL::BlendOperationReverseSubtract;
			case BlendOp_MIN:
				return MTL::BlendOperationMin;
			case BlendOp_MAX:
				return MTL::BlendOperationMax;
		}
		return MTL::BlendOperationUnspecialized;
	}
	constexpr MTL::BlendFactor _ConvertBlendFactor(Blend value)
	{
		switch (value)
		{
			case Blend_ZERO:
				return MTL::BlendFactorZero;
			case Blend_ONE:
				return MTL::BlendFactorOne;
			case Blend_BLEND_FACTOR:
				return MTL::BlendFactorBlendColor;
			case Blend_INV_BLEND_FACTOR:
				return MTL::BlendFactorOneMinusBlendColor;
			case Blend_DEST_ALPHA:
				return MTL::BlendFactorDestinationAlpha;
			case Blend_DEST_COLOR:
				return MTL::BlendFactorDestinationColor;
			case Blend_INV_DEST_ALPHA:
				return MTL::BlendFactorOneMinusDestinationAlpha;
			case Blend_INV_DEST_COLOR:
				return MTL::BlendFactorOneMinusDestinationColor;
			case Blend_SRC_ALPHA:
				return MTL::BlendFactorSourceAlpha;
			case Blend_SRC_COLOR:
				return MTL::BlendFactorSourceColor;
			case Blend_INV_SRC_ALPHA:
				return MTL::BlendFactorOneMinusSourceAlpha;
			case Blend_INV_SRC_COLOR:
				return MTL::BlendFactorOneMinusSourceColor;
			case Blend_SRC1_ALPHA:
				return MTL::BlendFactorSource1Alpha;
			case Blend_SRC1_COLOR:
				return MTL::BlendFactorSource1Color;
			case Blend_INV_SRC1_ALPHA:
				return MTL::BlendFactorOneMinusSource1Alpha;
			case Blend_INV_SRC1_COLOR:
				return MTL::BlendFactorOneMinusSource1Color;
			case Blend_SRC_ALPHA_SAT:
				return MTL::BlendFactorSourceAlphaSaturated;
		}
		return MTL::BlendFactorUnspecialized;
	}
	constexpr MTL::CompareFunction _ConvertCompareFunction(ComparisonFunc value)
	{
		switch (value)
		{
			case ComparisonFunc_ALWAYS:
				return MTL::CompareFunctionAlways;
			case ComparisonFunc_NEVER:
				return MTL::CompareFunctionNever;
			case ComparisonFunc_EQUAL:
				return MTL::CompareFunctionEqual;
			case ComparisonFunc_NOT_EQUAL:
				return MTL::CompareFunctionNotEqual;
			case ComparisonFunc_LESS:
				return MTL::CompareFunctionLess;
			case ComparisonFunc_LESS_EQUAL:
				return MTL::CompareFunctionLessEqual;
			case ComparisonFunc_GREATER:
				return MTL::CompareFunctionGreater;
			case ComparisonFunc_GREATER_EQUAL:
				return MTL::CompareFunctionGreaterEqual;
		}
		return MTL::CompareFunctionNever;
	}
	constexpr MTL::StencilOperation _ConvertStencilOperation(StencilOp value)
	{
		switch (value)
		{
			case StencilOp_KEEP:
				return MTL::StencilOperationKeep;
			case StencilOp_REPLACE:
				return MTL::StencilOperationReplace;
			case StencilOp_ZERO:
				return MTL::StencilOperationZero;
			case StencilOp_INVERT:
				return MTL::StencilOperationInvert;
			case StencilOp_INCR:
				return MTL::StencilOperationIncrementWrap;
			case StencilOp_INCR_SAT:
				return MTL::StencilOperationIncrementClamp;
			case StencilOp_DECR:
				return MTL::StencilOperationDecrementWrap;
			case StencilOp_DECR_SAT:
				return MTL::StencilOperationDecrementClamp;
		}
		return MTL::StencilOperationKeep;
	}
	constexpr MTL::TextureSwizzle _ConvertComponentSwizzle(ComponentSwizzle value)
	{
		switch (value)
		{
			case ComponentSwizzle_R:
				return MTL::TextureSwizzleRed;
			case ComponentSwizzle_G:
				return MTL::TextureSwizzleGreen;
			case ComponentSwizzle_B:
				return MTL::TextureSwizzleBlue;
			case ComponentSwizzle_A:
				return MTL::TextureSwizzleAlpha;
			case ComponentSwizzle_ONE:
				return MTL::TextureSwizzleOne;
			case ComponentSwizzle_ZERO:
				return MTL::TextureSwizzleZero;
			default:
				break;
		}
		return MTL::TextureSwizzleOne;
	}
	constexpr MTL::SamplerAddressMode _ConvertAddressMode(TextureAddressMode value)
	{
		switch (value) {
			case TextureAddressMode_CLAMP:
				return MTL::SamplerAddressModeClampToEdge;
			case TextureAddressMode_WRAP:
				return MTL::SamplerAddressModeRepeat;
			case TextureAddressMode_MIRROR_ONCE:
				return MTL::SamplerAddressModeMirrorClampToEdge;
			case TextureAddressMode_MIRROR:
				return MTL::SamplerAddressModeMirrorRepeat;
			case TextureAddressMode_BORDER:
				return MTL::SamplerAddressModeClampToBorderColor;
			default:
				break;
		}
		return MTL::SamplerAddressModeClampToEdge;
	}
	inline NS::SharedPtr<MTL::TextureDescriptor> _ConvertTextureDesc(const TextureDesc& desc, MTL::Device* device, bool textureUlongAtomics)
	{
		NS::SharedPtr<MTL::TextureDescriptor> descriptor = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
		descriptor->setWidth(desc.width);
		descriptor->setHeight(desc.height);
		descriptor->setDepth(desc.depth);
		descriptor->setArrayLength(desc.array_size);
		descriptor->setMipmapLevelCount(GetMipCount(desc));
		descriptor->setPixelFormat(_ConvertPixelFormat(desc.format));
		
		uint32_t sample_count = desc.sample_count;
		while (sample_count > 1 && !device->supportsTextureSampleCount(sample_count))
		{
			sample_count /= 2;
		}
		descriptor->setSampleCount(sample_count);
		
		switch (desc.type)
		{
			case TextureDesc::Type_TEXTURE_1D:
				//descriptor->setTextureType(desc->array_size > 1 ? MTL::TextureType1DArray : MTL::TextureType1D);
				descriptor->setTextureType(desc.array_size > 1 ? MTL::TextureType2DArray : MTL::TextureType2D); // NOTE: This seems to be broken! Real Texture1D type doesn't work in shaders, but creating Texture2D instead works! Issue FB21629558
				break;
			case TextureDesc::Type_TEXTURE_2D:
				if(desc.sample_count > 1)
				{
					descriptor->setTextureType(desc.array_size > 1 ? MTL::TextureType2DMultisampleArray : MTL::TextureType2DMultisample);
				}
				else
				{
					descriptor->setTextureType(desc.array_size > 1 ? MTL::TextureType2DArray : MTL::TextureType2D);
				}
				break;
			case TextureDesc::Type_TEXTURE_3D:
				descriptor->setTextureType(MTL::TextureType3D);
				break;
			default:
				break;
		}
		if (has_flag(desc.misc_flags, ResourceMiscFlag_TEXTURECUBE))
		{
			descriptor->setTextureType(desc.array_size > 6 ? MTL::TextureTypeCubeArray : MTL::TextureTypeCube);
			descriptor->setArrayLength(desc.array_size / 6);
		}
		
		MTL::ResourceOptions resource_options = {};
		if (has_flag(desc.misc_flags, ResourceMiscFlag_TRANSIENT_ATTACHMENT))
		{
			resource_options |= MTL::ResourceStorageModeMemoryless;
			descriptor->setStorageMode(MTL::StorageModeMemoryless);
		}
		if (desc.usage == Usage_DEFAULT && !device->hasUnifiedMemory())
		{
			// Discrete GPU path:
			resource_options |= MTL::ResourceStorageModePrivate;
			descriptor->setStorageMode(MTL::StorageModePrivate);
		}
		else if (
				 has_flag(desc.misc_flags, ResourceMiscFlag_SPARSE) ||
				 has_flag(desc.bind_flags, BindFlag_RENDER_TARGET) ||
				 has_flag(desc.bind_flags, BindFlag_DEPTH_STENCIL) ||
				 has_flag(desc.bind_flags, BindFlag_UNORDERED_ACCESS)
				 )
		{
			// optimized storage for render efficiency even on UMA GPU:
			resource_options |= MTL::ResourceStorageModePrivate;
			descriptor->setStorageMode(MTL::StorageModePrivate);
		}
		else if (desc.usage == Usage_UPLOAD)
		{
			resource_options |= MTL::ResourceStorageModeShared;
			resource_options |= MTL::ResourceOptionCPUCacheModeWriteCombined;
			descriptor->setStorageMode(MTL::StorageModeShared);
			descriptor->setTextureType(MTL::TextureTypeTextureBuffer);
		}
		else if (desc.usage == Usage_READBACK)
		{
			resource_options |= MTL::ResourceStorageModeShared;
			descriptor->setStorageMode(MTL::StorageModeShared);
			descriptor->setTextureType(MTL::TextureTypeTextureBuffer);
		}
		else
		{
			// CPU accessible or UMA GPU zero-copy optimized:
			resource_options |= MTL::ResourceStorageModeShared;
			descriptor->setStorageMode(MTL::StorageModeShared);
		}
		descriptor->setResourceOptions(resource_options);
		
		MTL::TextureUsage usage = {};
		if (has_flag(desc.bind_flags, BindFlag_RENDER_TARGET) || has_flag(desc.bind_flags, BindFlag_DEPTH_STENCIL))
		{
			usage |= MTL::TextureUsageRenderTarget;
		}
		if (has_flag(desc.bind_flags, BindFlag_UNORDERED_ACCESS))
		{
			usage |= MTL::TextureUsageShaderWrite;
			usage |= MTL::TextureUsageRenderTarget; // support for ClearUAV
			switch (descriptor->pixelFormat())
			{
				case MTL::PixelFormatR32Uint:
				case MTL::PixelFormatR32Sint:
					usage |= MTL::TextureUsageShaderAtomic;
					break;
				case MTL::PixelFormatRG32Uint:
					if (textureUlongAtomics) // workaround for: https://github.com/turanszkij/WickedEngine/issues/1597
					{
						usage |= MTL::TextureUsageShaderAtomic;
					}
					break;
				default:
					break;
			}
		}
		if (has_flag(desc.bind_flags, BindFlag_SHADER_RESOURCE))
		{
			usage |= MTL::TextureUsageShaderRead;
		}
		descriptor->setUsage(usage);
		
		descriptor->setAllowGPUOptimizedContents(true);
		
		return descriptor;
	}

	inline IRDescriptorTableEntry create_entry(MTL::Texture* res, float min_lod_clamp = 0, uint32_t metadata = 0)
	{
		IRDescriptorTableEntry entry;
		IRDescriptorTableSetTexture(&entry, res, min_lod_clamp, metadata);
		return entry;
	}
	inline IRDescriptorTableEntry create_entry(MTL::ResourceID resourceID, float min_lod_clamp = 0, uint32_t metadata = 0)
	{
		// This is the expanded version of IRDescriptorTableSetTexture, made to be compatible with texture view pooling
		IRDescriptorTableEntry entry;
		entry.gpuVA = 0;
		entry.textureViewID = resourceID._impl;
		entry.metadata = (uint64_t)(*((uint32_t*)&min_lod_clamp) | ((uint64_t)metadata) << 32);
		return entry;
	}
	inline IRDescriptorTableEntry create_entry(MTL::Buffer* res, uint64_t size, uint64_t offset = 0, MTL::Texture* texture_buffer_view = nullptr, Format format = Format_UNKNOWN, bool structured = false)
	{
		IRDescriptorTableEntry entry;
		IRBufferView view = {};
		view.buffer = res;
		view.bufferSize = size;
		view.bufferOffset = offset;
		if (texture_buffer_view == nullptr)
		{
			view.typedBuffer = structured || (format != Format_UNKNOWN);
		}
		else
		{
			view.typedBuffer = true;
			view.textureBufferView = texture_buffer_view;
			view.textureViewOffsetInElements = uint32_t(offset / (uint64_t)GetFormatStride(format));
		}
		IRDescriptorTableSetBufferView(&entry, &view);
		return entry;
	}
	inline IRDescriptorTableEntry create_entry(MTL::Buffer* res, uint64_t size, uint64_t offset = 0, MTL::ResourceID resourceID = {}, Format format = Format_UNKNOWN, bool structured = false)
	{
		// This is the expanded version of IRDescriptorTableSetBufferView, made to be compatible with texture view pooling
		IRDescriptorTableEntry entry;
		entry.gpuVA = res->gpuAddress() + offset;
		entry.textureViewID = resourceID._impl;
		entry.metadata = (size & kIRBufSizeMask) << kIRBufSizeOffset;
		bool typedBuffer = false;
		uint64_t textureViewOffsetInElements = 0;
		if (entry.textureViewID == 0)
		{
			typedBuffer = structured || (format != Format_UNKNOWN);
		}
		else
		{
			typedBuffer = true;
			textureViewOffsetInElements = uint32_t(offset / (uint64_t)GetFormatStride(format));
		}
		entry.metadata |= ((uint64_t)textureViewOffsetInElements & kIRTexViewMask) << kIRTexViewOffset;
		entry.metadata |= (uint64_t)typedBuffer << kIRTypedBufferOffset;
		return entry;
	}
	inline IRDescriptorTableEntry create_entry(MTL::SamplerState* sam, float lod_bias = 0)
	{
		IRDescriptorTableEntry entry;
		IRDescriptorTableSetSampler(&entry, sam, lod_bias);
		return entry;
	}

	struct Buffer_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Buffer> buffer;
		MTL::GPUAddress gpu_address = {};
		
		struct Subresource
		{
			IRDescriptorTableEntry entry = {};
			uint64_t offset = 0;
			uint64_t size = 0;
			int index = -1;
#ifndef USE_TEXTURE_VIEW_POOL
			NS::SharedPtr<MTL::Texture> view;
#endif // USE_TEXTURE_VIEW_POOL
			
			bool IsValid() const { return entry.gpuVA != 0; }
		};
		Subresource srv;
		Subresource uav;
		Protean::vector<Subresource> subresources_srv;
		Protean::vector<Subresource> subresources_uav;
		
		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			
#ifndef USE_TEXTURE_VIEW_POOL
			allocationhandler->destroyer_resources.push_back(std::make_pair(srv.view, framecount));
			allocationhandler->destroyer_resources.push_back(std::make_pair(uav.view, framecount));
			for (auto& x : subresources_srv)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x.view, framecount));
			}
			for (auto& x : subresources_uav)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x.view, framecount));
			}
#endif // USE_TEXTURE_VIEW_POOL
			
			if (srv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(srv.index, framecount));
			srv = {};
			
			if (uav.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(uav.index, framecount));
			uav = {};
			
			for (auto& x : subresources_srv)
			{
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_uav.clear();
		}

		~Buffer_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Texture_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Texture> texture;
		NS::SharedPtr<MTL::Texture> texture_chroma; // NV12 chrominance plane (RG8), half resolution
		
		// Things for readback and upload texture types, linear tiling, using buffer:
		NS::SharedPtr<MTL::Buffer> buffer;
		Protean::vector<SubresourceData> mapped_subresources;
		
		struct Subresource
		{
			IRDescriptorTableEntry entry = {};
			int index = -1;
#ifndef USE_TEXTURE_VIEW_POOL
			NS::SharedPtr<MTL::Texture> view;
#endif // USE_TEXTURE_VIEW_POOL
			
			uint32_t firstMip = 0;
			uint32_t mipCount = 0;
			uint32_t firstSlice = 0;
			uint32_t sliceCount = 0;
			
			bool IsValid() const { return entry.textureViewID != 0 || mipCount > 0 || sliceCount > 0; }
		};
		Subresource srv;
		Subresource uav;
		Subresource rtv;
		Subresource dsv;
		Protean::vector<Subresource> subresources_srv;
		Protean::vector<Subresource> subresources_uav;
		Protean::vector<Subresource> subresources_rtv;
		Protean::vector<Subresource> subresources_dsv;
		
		SparseTextureProperties sparse_properties;
		
		void destroy_subresources()
		{
			uint64_t framecount = allocationhandler->framecount;
			
#ifndef USE_TEXTURE_VIEW_POOL
			allocationhandler->destroyer_resources.push_back(std::make_pair(srv.view, framecount));
			allocationhandler->destroyer_resources.push_back(std::make_pair(uav.view, framecount));
			for (auto& x : subresources_srv)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x.view, framecount));
			}
			for (auto& x : subresources_uav)
			{
				allocationhandler->destroyer_resources.push_back(std::make_pair(x.view, framecount));
			}
#endif // USE_TEXTURE_VIEW_POOL
			
			if (srv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(srv.index, framecount));
			srv = {};
			
			if (uav.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(uav.index, framecount));
			uav = {};
			
			if (rtv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(rtv.index, framecount));
			
			if (dsv.index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(dsv.index, framecount));
			
			for (auto& x : subresources_srv)
			{
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_srv.clear();
			for (auto& x : subresources_uav)
			{
				if (x.index >= 0)
					allocationhandler->destroyer_bindless_res.push_back(std::make_pair(x.index, framecount));
			}
			subresources_uav.clear();
			
			// RTV and DSV can just be cleared, they don't have any gpu-referenced resources:
			subresources_rtv.clear();
			subresources_dsv.clear();
		}

		~Texture_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			if (texture.get() != nullptr)
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(texture), framecount));
			if (texture_chroma.get() != nullptr)
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(texture_chroma), framecount));
			if (buffer.get() != nullptr)
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
			destroy_subresources();
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Sampler_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		int index = -1;
		NS::SharedPtr<MTL::SamplerState> sampler;
		IRDescriptorTableEntry entry = {};

		~Sampler_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_samplers.push_back(std::make_pair(std::move(sampler), framecount));
			if (index >= 0)
				allocationhandler->destroyer_bindless_sam.push_back(std::make_pair(index, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct QueryHeap_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Buffer> buffer;
		NS::SharedPtr<MTL4::CounterHeap> counter_heap;

		~QueryHeap_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(buffer), framecount));
			allocationhandler->destroyer_counter_heaps.push_back(std::make_pair(std::move(counter_heap), framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct Shader_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::Library> library;
		NS::SharedPtr<MTL::Function> function;
		NS::SharedPtr<MTL::ComputePipelineState> compute_pipeline;
		MetalDevice::ShaderAdditionalData additional_data;

		~Shader_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_libraries.push_back(std::make_pair(std::move(library), framecount));
			allocationhandler->destroyer_functions.push_back(std::make_pair(std::move(function), framecount));
			allocationhandler->destroyer_compute_pipelines.push_back(std::make_pair(std::move(compute_pipeline), framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct PipelineState_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::RenderPipelineDescriptor> descriptor;
		NS::SharedPtr<MTL::MeshRenderPipelineDescriptor> ms_descriptor;
		NS::SharedPtr<MTL::RenderPipelineState> render_pipeline;
		NS::SharedPtr<MTL::DepthStencilState> depth_stencil_state;
		MTL::Size numthreads_as = {};
		MTL::Size numthreads_ms = {};
		bool needs_draw_params = false;
		IRGeometryEmulationPipelineDescriptor gs_desc = {};

		~PipelineState_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_render_pipelines.push_back(std::make_pair(std::move(render_pipeline), framecount));
			allocationhandler->destroyer_depth_stencil_states.push_back(std::make_pair(std::move(depth_stencil_state), framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct BVH_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		NS::SharedPtr<MTL::AccelerationStructure> acceleration_structure;
		NS::SharedPtr<MTL::Buffer> scratch;
		NS::SharedPtr<MTL::Buffer> tlas_header_instancecontributions;
		int tlas_descriptor_index = -1;
		IRDescriptorTableEntry tlas_entry = {};
		MTL::ResourceID resourceid = {};

		~BVH_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(acceleration_structure), framecount));
			allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(scratch), framecount));
			if (tlas_header_instancecontributions.get() != nullptr)
				allocationhandler->destroyer_resources.push_back(std::make_pair(std::move(tlas_header_instancecontributions), framecount));
			if (tlas_descriptor_index >= 0)
				allocationhandler->destroyer_bindless_res.push_back(std::make_pair(tlas_descriptor_index, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};
	struct RTPipelineState_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;

		~RTPipelineState_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroylocker.unlock();
		}
	};
	struct SwapChain_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		ColorSpace colorSpace = ColorSpace_SRGB;
		CA::MetalLayer* layer = nullptr;
		NS::SharedPtr<CA::MetalDrawable> current_drawable;
		NS::SharedPtr<MTL::Texture> current_texture;

		~SwapChain_Metal()
		{
			if (allocationhandler == nullptr)
				return;
			allocationhandler->destroylocker.lock();
			uint64_t framecount = allocationhandler->framecount;
			allocationhandler->destroyer_resources.push_back(std::make_pair(current_texture, framecount));
			allocationhandler->destroyer_drawables.push_back(std::make_pair(current_drawable, framecount));
			allocationhandler->destroylocker.unlock();
		}
	};

	static void video_output_callback(void* decompressionOutputRefCon, void* sourceFrameRefCon, OSStatus status, VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer, CMTime presentationTimeStamp, CMTime presentationDuration);
	using CVMetalTextureCacheRef = struct __CVMetalTextureCache*;
	using CVMetalTextureRef = CVImageBufferRef;
	extern "C" CVReturn CVMetalTextureCacheCreate(CFAllocatorRef allocator, CFDictionaryRef cacheAttributes, void* metalDevice, CFDictionaryRef textureAttributes, CVMetalTextureCacheRef* cacheOut);
	extern "C" CVReturn CVMetalTextureCacheCreateTextureFromImage(CFAllocatorRef allocator, CVMetalTextureCacheRef textureCache, CVImageBufferRef sourceImage, CFDictionaryRef textureAttributes, NS::UInteger pixelFormat, size_t width, size_t height, size_t planeIndex, CVMetalTextureRef* textureOut);
	extern "C" void CVMetalTextureCacheFlush(CVMetalTextureCacheRef textureCache, uint64_t options);
	extern "C" void* CVMetalTextureGetTexture(CVMetalTextureRef image);

	struct VideoDecoder_Metal
	{
		Protean::Allocator::shared_ptr<MetalDevice::AllocationHandler> allocationhandler;
		CMVideoFormatDescriptionRef format_desc = nullptr;
		VTDecompressionSessionRef session = nullptr;
		CVPixelBufferRef decoded = nullptr;
		CFDictionaryRef output_attrs = nullptr;
		CVMetalTextureCacheRef texture_cache = nullptr;
		std::mutex locker;
		OSStatus decode_status = noErr;
		uint64_t frames_decoded = 0;
		bool logged_error = false;
		Protean::vector<uint8_t> sample_bytes;

		struct PendingCopy
		{
			CVPixelBufferRef image = nullptr;
			CVMetalTextureRef planes[2] = {};
			MTL::Texture* textures[2] = {};
			uint64_t frame = 0;
		};
		Protean::vector<PendingCopy> pending_copies;

		void release_pending(uint64_t framecount, bool all)
		{
			const uint32_t buffercount = GraphicsDevice::GetBufferCount();
			const uint32_t count = (uint32_t)pending_copies.size();
			uint32_t keep = 0;
			bool released = false;
			for (uint32_t i = 0; i < count; ++i)
			{
				PendingCopy entry = pending_copies[i];
				if (!all && entry.frame + buffercount >= framecount)
				{
					pending_copies[keep++] = entry;
					continue;
				}
				released = true;
				if (allocationhandler != nullptr)
				{
					std::scoped_lock lock(allocationhandler->destroylocker);
					allocationhandler->remove_resident(entry.textures[0]);
					allocationhandler->remove_resident(entry.textures[1]);
				}
				if (entry.planes[0] != nullptr)
					CFRelease(entry.planes[0]);
				if (entry.planes[1] != nullptr)
					CFRelease(entry.planes[1]);
				if (entry.image != nullptr)
					CFRelease(entry.image);
			}
			pending_copies.resize(keep);
			if (released && texture_cache != nullptr)
				CVMetalTextureCacheFlush(texture_cache, 0);
		}
		
		~VideoDecoder_Metal()
		{
			if (session != nullptr)
				VTDecompressionSessionWaitForAsynchronousFrames(session);
			release_pending(0, true);
			destroy_session();
			if (format_desc != nullptr)
			{
				CFRelease(format_desc);
				format_desc = nullptr;
			}
			if (output_attrs != nullptr)
			{
				CFRelease(output_attrs);
				output_attrs = nullptr;
			}
			if (texture_cache != nullptr)
			{
				CFRelease((CFTypeRef)texture_cache);
				texture_cache = nullptr;
			}
		}
		void destroy_session()
		{
			if (session != nullptr)
			{
				VTDecompressionSessionWaitForAsynchronousFrames(session);
				VTDecompressionSessionInvalidate(session);
				CFRelease(session);
				session = nullptr;
			}
			std::scoped_lock lock(locker);
			if (decoded != nullptr)
			{
				CFRelease(decoded);
				decoded = nullptr;
			}
		}
	};

	static void video_output_callback(void* decompressionOutputRefCon, void* sourceFrameRefCon, OSStatus status, VTDecodeInfoFlags infoFlags, CVImageBufferRef imageBuffer, CMTime presentationTimeStamp, CMTime presentationDuration)
	{
		(void)sourceFrameRefCon;
		(void)infoFlags;
		(void)presentationTimeStamp;
		(void)presentationDuration;
		VideoDecoder_Metal* decoder = (VideoDecoder_Metal*)decompressionOutputRefCon;
		if (decoder == nullptr)
			return;
		std::scoped_lock lock(decoder->locker);
		decoder->decode_status = status;
		if (decoder->decoded != nullptr)
		{
			CFRelease(decoder->decoded);
			decoder->decoded = nullptr;
		}
		if (status == noErr && imageBuffer != nullptr)
		{
			decoder->decoded = (CVPixelBufferRef)CFRetain(imageBuffer);
		}
	}

	template<typename T> struct MetalType;
	template<> struct MetalType<GPUBuffer> { using type = Buffer_Metal; };
	template<> struct MetalType<Texture> { using type = Texture_Metal; };
	template<> struct MetalType<Sampler> { using type = Sampler_Metal; };
	template<> struct MetalType<GPUQueryHeap> { using type = QueryHeap_Metal; };
	template<> struct MetalType<Shader> { using type = Shader_Metal; };
	template<> struct MetalType<RaytracingAccelerationStructure> { using type = BVH_Metal; };
	template<> struct MetalType<PipelineState> { using type = PipelineState_Metal; };
	template<> struct MetalType<RaytracingPipelineState> { using type = RTPipelineState_Metal; };
	template<> struct MetalType<SwapChain> { using type = SwapChain_Metal; };
	template<> struct MetalType<VideoDecoder> { using type = VideoDecoder_Metal; };


	template<typename T>
	typename MetalType<T>::type* to_internal(const T* param)
	{
		return static_cast<typename MetalType<T>::type*>(param->internal_state.get());
	}

	template<typename T>
	typename MetalType<T>::type* to_internal(const GPUResource* res)
	{
		return static_cast<typename MetalType<T>::type*>(res->internal_state.get());
	}
}
}

#endif // __APPLE__
