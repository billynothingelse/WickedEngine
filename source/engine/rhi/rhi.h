#pragma once

// Protean graphics API: resource descriptions, enums and the GraphicsDevice interface.
// This is the single header needed to talk to the GPU, the backends implement GraphicsDevice
// (rhi/d3d12, rhi/vulkan, rhi/metal).
#include "core/common_include.h"
#include "core/vector.h"
#include "core/allocator.h"
#include "platform/platform.h"

#include <cassert>
#include <memory>
#include <string>
#include <limits>
#include <cstring>
#include <algorithm>
#include <functional>

namespace Protean::Graphics
{
	struct Shader;
	struct GPUResource;
	struct GPUBuffer;
	struct Texture;

	enum ValidationMode : uint8_t
	{
		ValidationMode_Disabled,	// No validation is enabled
		ValidationMode_Enabled,	// CPU command validation
		ValidationMode_GPU,		// CPU and GPU-based validation
		ValidationMode_Verbose		// Print all warnings, errors and info messages
	};

	enum AdapterType : uint8_t
	{
		AdapterType_Other,
		AdapterType_IntegratedGpu,
		AdapterType_DiscreteGpu,
		AdapterType_VirtualGpu,
		AdapterType_Cpu,
	};

	enum GPUPreference : uint8_t
	{
		GPUPreference_Discrete,
		GPUPreference_Integrated,
		GPUPreference_Nvidia,
		GPUPreference_Intel,
		GPUPreference_AMD,
	};

	enum ShaderStage : uint8_t
	{
		ShaderStage_MS,		// Mesh Shader
		ShaderStage_AS,		// Amplification Shader
		ShaderStage_VS,		// Vertex Shader
		ShaderStage_HS,		// Hull Shader
		ShaderStage_DS,		// Domain Shader
		ShaderStage_GS,		// Geometry Shader
		ShaderStage_PS,		// Pixel Shader
		ShaderStage_CS,		// Compute Shader
		ShaderStage_LIB,	// Shader Library
		ShaderStage_Count,
	};
	enum ShaderFormat : uint8_t
	{
		ShaderFormat_NONE,		// Not used
		ShaderFormat_HLSL5,		// DXBC
		ShaderFormat_HLSL6,		// DXIL
		ShaderFormat_SPIRV,		// SPIR-V
		ShaderFormat_HLSL6_XS,	// XBOX Series Native
		ShaderFormat_PS5,		// Playstation 5
		ShaderFormat_METAL,		// Apple Metal
	};
	enum ShaderModel : uint8_t
	{
		ShaderModel_SM_5_0,
		ShaderModel_SM_6_0,
		ShaderModel_SM_6_1,
		ShaderModel_SM_6_2,
		ShaderModel_SM_6_3,
		ShaderModel_SM_6_4,
		ShaderModel_SM_6_5,
		ShaderModel_SM_6_6,
		ShaderModel_SM_6_7,
	};
	enum PrimitiveTopology : uint8_t
	{
		PrimitiveTopology_UNDEFINED,
		PrimitiveTopology_TRIANGLELIST,
		PrimitiveTopology_TRIANGLESTRIP,
		PrimitiveTopology_POINTLIST,
		PrimitiveTopology_LINELIST,
		PrimitiveTopology_LINESTRIP,
		PrimitiveTopology_PATCHLIST,
	};
	enum ComparisonFunc : uint8_t
	{
		ComparisonFunc_NEVER,
		ComparisonFunc_LESS,
		ComparisonFunc_EQUAL,
		ComparisonFunc_LESS_EQUAL,
		ComparisonFunc_GREATER,
		ComparisonFunc_NOT_EQUAL,
		ComparisonFunc_GREATER_EQUAL,
		ComparisonFunc_ALWAYS,
	};
	enum DepthWriteMask : uint8_t
	{
		DepthWriteMask_ZERO,	// Disables depth write
		DepthWriteMask_ALL,	// Enables depth write
	};
	enum StencilOp : uint8_t
	{
		StencilOp_KEEP,
		StencilOp_ZERO,
		StencilOp_REPLACE,
		StencilOp_INCR_SAT,
		StencilOp_DECR_SAT,
		StencilOp_INVERT,
		StencilOp_INCR,
		StencilOp_DECR,
	};
	enum Blend : uint8_t
	{
		Blend_ZERO,
		Blend_ONE,
		Blend_SRC_COLOR,
		Blend_INV_SRC_COLOR,
		Blend_SRC_ALPHA,
		Blend_INV_SRC_ALPHA,
		Blend_DEST_ALPHA,
		Blend_INV_DEST_ALPHA,
		Blend_DEST_COLOR,
		Blend_INV_DEST_COLOR,
		Blend_SRC_ALPHA_SAT,
		Blend_BLEND_FACTOR,
		Blend_INV_BLEND_FACTOR,
		Blend_SRC1_COLOR,
		Blend_INV_SRC1_COLOR,
		Blend_SRC1_ALPHA,
		Blend_INV_SRC1_ALPHA,
	}; 
	enum BlendOp : uint8_t
	{
		BlendOp_ADD,
		BlendOp_SUBTRACT,
		BlendOp_REV_SUBTRACT,
		BlendOp_MIN,
		BlendOp_MAX,
	};
	enum FillMode : uint8_t
	{
		FillMode_WIREFRAME,
		FillMode_SOLID,
	};
	enum CullMode : uint8_t
	{
		CullMode_NONE,
		CullMode_FRONT,
		CullMode_BACK,
	};
	enum InputClassification : uint8_t
	{
		InputClassification_PER_VERTEX_DATA,
		InputClassification_PER_INSTANCE_DATA,
	};
	enum Usage : uint8_t
	{
		Usage_DEFAULT,	// CPU no access, GPU read/write
		Usage_UPLOAD,	    // CPU write, GPU read
		Usage_READBACK,	// CPU read, GPU write
	};
	enum TextureAddressMode : uint8_t
	{
		TextureAddressMode_WRAP,
		TextureAddressMode_MIRROR,
		TextureAddressMode_CLAMP,
		TextureAddressMode_BORDER,
		TextureAddressMode_MIRROR_ONCE,
	};
	enum Filter : uint8_t
	{
		Filter_MIN_MAG_MIP_POINT,
		Filter_MIN_MAG_POINT_MIP_LINEAR,
		Filter_MIN_POINT_MAG_LINEAR_MIP_POINT,
		Filter_MIN_POINT_MAG_MIP_LINEAR,
		Filter_MIN_LINEAR_MAG_MIP_POINT,
		Filter_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		Filter_MIN_MAG_LINEAR_MIP_POINT,
		Filter_MIN_MAG_MIP_LINEAR,
		Filter_ANISOTROPIC,
		Filter_COMPARISON_MIN_MAG_MIP_POINT,
		Filter_COMPARISON_MIN_MAG_POINT_MIP_LINEAR,
		Filter_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT,
		Filter_COMPARISON_MIN_POINT_MAG_MIP_LINEAR,
		Filter_COMPARISON_MIN_LINEAR_MAG_MIP_POINT,
		Filter_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		Filter_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
		Filter_COMPARISON_MIN_MAG_MIP_LINEAR,
		Filter_COMPARISON_ANISOTROPIC,
		Filter_MINIMUM_MIN_MAG_MIP_POINT,
		Filter_MINIMUM_MIN_MAG_POINT_MIP_LINEAR,
		Filter_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		Filter_MINIMUM_MIN_POINT_MAG_MIP_LINEAR,
		Filter_MINIMUM_MIN_LINEAR_MAG_MIP_POINT,
		Filter_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		Filter_MINIMUM_MIN_MAG_LINEAR_MIP_POINT,
		Filter_MINIMUM_MIN_MAG_MIP_LINEAR,
		Filter_MINIMUM_ANISOTROPIC,
		Filter_MAXIMUM_MIN_MAG_MIP_POINT,
		Filter_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR,
		Filter_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT,
		Filter_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR,
		Filter_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT,
		Filter_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
		Filter_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT,
		Filter_MAXIMUM_MIN_MAG_MIP_LINEAR,
		Filter_MAXIMUM_ANISOTROPIC,
	};
	enum SamplerBorderColor : uint8_t
	{
		SamplerBorderColor_TRANSPARENT_BLACK,
		SamplerBorderColor_OPAQUE_BLACK,
		SamplerBorderColor_OPAQUE_WHITE,
	};
	enum Format : uint8_t
	{
		Format_UNKNOWN,

		Format_R32G32B32A32_FLOAT,
		Format_R32G32B32A32_UINT,
		Format_R32G32B32A32_SINT,

		Format_R32G32B32_FLOAT,
		Format_R32G32B32_UINT,
		Format_R32G32B32_SINT,

		Format_R16G16B16A16_FLOAT,
		Format_R16G16B16A16_UNORM,
		Format_R16G16B16A16_UINT,
		Format_R16G16B16A16_SNORM,
		Format_R16G16B16A16_SINT,

		Format_R32G32_FLOAT,
		Format_R32G32_UINT,
		Format_R32G32_SINT,
		Format_D32_FLOAT_S8X24_UINT,	// depth (32-bit) + stencil (8-bit) | SRV: R32_FLOAT (default or depth aspect), R8_UINT (stencil aspect)

		Format_R10G10B10A2_UNORM,
		Format_R10G10B10A2_UINT,
		Format_R11G11B10_FLOAT,
		Format_R8G8B8A8_UNORM,
		Format_R8G8B8A8_UNORM_SRGB,
		Format_R8G8B8A8_UINT,
		Format_R8G8B8A8_SNORM,
		Format_R8G8B8A8_SINT,
		Format_B8G8R8A8_UNORM,
		Format_B8G8R8A8_UNORM_SRGB,
		Format_R16G16_FLOAT,
		Format_R16G16_UNORM,
		Format_R16G16_UINT,
		Format_R16G16_SNORM,
		Format_R16G16_SINT,
		Format_D32_FLOAT,				// depth (32-bit) | SRV: R32_FLOAT
		Format_R32_FLOAT,
		Format_R32_UINT,
		Format_R32_SINT, 
		Format_D24_UNORM_S8_UINT,		// depth (24-bit) + stencil (8-bit) | SRV: R24_INTERNAL (default or depth aspect), R8_UINT (stencil aspect)
		Format_R9G9B9E5_SHAREDEXP,

		Format_R8G8_UNORM,
		Format_R8G8_UINT,
		Format_R8G8_SNORM,
		Format_R8G8_SINT,
		Format_R16_FLOAT,
		Format_D16_UNORM,				// depth (16-bit) | SRV: R16_UNORM
		Format_R16_UNORM,
		Format_R16_UINT,
		Format_R16_SNORM,
		Format_R16_SINT,

		Format_R8_UNORM,
		Format_R8_UINT,
		Format_R8_SNORM,
		Format_R8_SINT,

		// Formats that are not usable in render pass must be below because formats in render pass must be encodable as 6 bits:

		Format_BC1_UNORM,			// Three color channels (5 bits:6 bits:5 bits), with 0 or 1 bit(s) of alpha
		Format_BC1_UNORM_SRGB,		// Three color channels (5 bits:6 bits:5 bits), with 0 or 1 bit(s) of alpha
		Format_BC2_UNORM,			// Three color channels (5 bits:6 bits:5 bits), with 4 bits of alpha
		Format_BC2_UNORM_SRGB,		// Three color channels (5 bits:6 bits:5 bits), with 4 bits of alpha
		Format_BC3_UNORM,			// Three color channels (5 bits:6 bits:5 bits) with 8 bits of alpha
		Format_BC3_UNORM_SRGB,		// Three color channels (5 bits:6 bits:5 bits) with 8 bits of alpha
		Format_BC4_UNORM,			// One color channel (8 bits)
		Format_BC4_SNORM,			// One color channel (8 bits)
		Format_BC5_UNORM,			// Two color channels (8 bits:8 bits)
		Format_BC5_SNORM,			// Two color channels (8 bits:8 bits)
		Format_BC6H_UF16,			// Three color channels (16 bits:16 bits:16 bits) in "half" floating point
		Format_BC6H_SF16,			// Three color channels (16 bits:16 bits:16 bits) in "half" floating point
		Format_BC7_UNORM,			// Three color channels (4 to 7 bits per channel) with 0 to 8 bits of alpha
		Format_BC7_UNORM_SRGB,		// Three color channels (4 to 7 bits per channel) with 0 to 8 bits of alpha

		Format_NV12,				// video YUV420; SRV Luminance aspect: R8_UNORM, SRV Chrominance aspect: R8G8_UNORM
	};
	enum GpuQueryType : uint8_t
	{
		GpuQueryType_TIMESTAMP,			// retrieve time point of gpu execution
		GpuQueryType_OCCLUSION,			// how many samples passed depth test?
		GpuQueryType_OCCLUSION_BINARY,	// depth test passed or not?
	};
	enum IndexBufferFormat : uint8_t
	{
		IndexBufferFormat_UINT16,
		IndexBufferFormat_UINT32,
	};
	enum SubresourceType : uint8_t
	{
		SubresourceType_SRV, // shader resource view
		SubresourceType_UAV, // unordered access view
		SubresourceType_RTV, // render target view
		SubresourceType_DSV, // depth stencil view
	};

	enum ShadingRate : uint8_t
	{
		ShadingRate_RATE_1X1,	// Default/full shading rate
		ShadingRate_RATE_1X2,
		ShadingRate_RATE_2X1,
		ShadingRate_RATE_2X2,
		ShadingRate_RATE_2X4,
		ShadingRate_RATE_4X2,
		ShadingRate_RATE_4X4,

		ShadingRate_RATE_INVALID
	};

	enum PredicationOp : uint8_t
	{
		PredicationOp_EQUAL_ZERO,
		PredicationOp_NOT_EQUAL_ZERO,
	};

	enum ImageAspect : uint8_t
	{
		ImageAspect_COLOR,
		ImageAspect_DEPTH,
		ImageAspect_STENCIL,
		ImageAspect_LUMINANCE,
		ImageAspect_CHROMINANCE,
	};

	enum VideoFrameType : uint8_t
	{
		VideoFrameType_Intra,
		VideoFrameType_Predictive,
	};

	enum VideoProfile : uint8_t
	{
		VideoProfile_H264,	// AVC
		VideoProfile_H265,	// HEVC
	};

	enum ComponentSwizzle : uint8_t
	{
		ComponentSwizzle_R,
		ComponentSwizzle_G,
		ComponentSwizzle_B,
		ComponentSwizzle_A,
		ComponentSwizzle_ZERO,
		ComponentSwizzle_ONE,
	};

	enum ColorSpace : uint8_t
	{
		ColorSpace_SRGB,			// SDR color space (8 or 10 bits per channel)
		ColorSpace_HDR10_ST2084,	// HDR10 color space (10 bits per channel)
		ColorSpace_HDR_LINEAR,		// HDR color space (16 bits per channel)
	};

	// Flags ////////////////////////////////////////////

	enum ColorWrite {
		ColorWrite_DISABLE = 0,
		ColorWrite_ENABLE_RED = 1 << 0,
		ColorWrite_ENABLE_GREEN = 1 << 1,
		ColorWrite_ENABLE_BLUE = 1 << 2,
		ColorWrite_ENABLE_ALPHA = 1 << 3,
		ColorWrite_ENABLE_ALL = ~0,
	};

	enum BindFlag : uint8_t
	{
		BindFlag_NONE = 0,
		BindFlag_VERTEX_BUFFER = 1 << 0,
		BindFlag_INDEX_BUFFER = 1 << 1,
		BindFlag_CONSTANT_BUFFER = 1 << 2,
		BindFlag_SHADER_RESOURCE = 1 << 3,
		BindFlag_RENDER_TARGET = 1 << 4,
		BindFlag_DEPTH_STENCIL = 1 << 5,
		BindFlag_UNORDERED_ACCESS = 1 << 6,
		BindFlag_SHADING_RATE = 1 << 7,
	};

	enum ResourceMiscFlag {
		ResourceMiscFlag_NONE = 0,
		ResourceMiscFlag_TEXTURECUBE = 1 << 0,
		ResourceMiscFlag_INDIRECT_ARGS = 1 << 1,
		ResourceMiscFlag_BUFFER_RAW = 1 << 2,
		ResourceMiscFlag_BUFFER_STRUCTURED = 1 << 3,
		ResourceMiscFlag_RAY_TRACING = 1 << 4,
		ResourceMiscFlag_PREDICATION = 1 << 5,
		ResourceMiscFlag_TRANSIENT_ATTACHMENT = 1 << 6,	// hint: used in renderpass, without needing to write content to memory (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
		ResourceMiscFlag_SPARSE = 1 << 7,	// sparse resource without backing memory allocation
		ResourceMiscFlag_ALIASING_BUFFER = 1 << 8,			// memory allocation will be suitable for buffers
		ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS = 1 << 9,// memory allocation will be suitable for textures that are non render targets nor depth stencils
		ResourceMiscFlag_ALIASING_TEXTURE_RT_DS = 1 << 10,	// memory allocation will be suitable for textures that are either render targets or depth stencils
		ResourceMiscFlag_TYPED_FORMAT_CASTING = 1 << 11,	// enable casting formats between same type and different modifiers: eg. UNORM -> SRGB
		ResourceMiscFlag_TYPELESS_FORMAT_CASTING = 1 << 12,	// enable casting formats to other formats that have the same bit-width and channel layout: eg. R32_FLOAT -> R32_UINT
		ResourceMiscFlag_VIDEO_DECODE = 1 << 13,	// resource is usabe in video decoding operations (for buffers it is indicating a bitstream buffer, for textures it is a DPB and output texture if DPB_AND_OUTPUT_COINCIDE is supported)
		ResourceMiscFlag_VIDEO_DECODE_OUTPUT_ONLY = 1 << 14,	// resource is usabe in video decoding operations but as output only and not as DPB (used for DPB textures when DPB_AND_OUTPUT_COINCIDE is NOT supported)
		ResourceMiscFlag_VIDEO_DECODE_DPB_ONLY = 1 << 15,	// resource is usabe in video decoding operations but as strictly DPB only (used for output textures when DPB_AND_OUTPUT_COINCIDE is NOT supported)
		ResourceMiscFlag_NO_DEFAULT_DESCRIPTORS = 1 << 16, // skips creation of default descriptors for resources
		ResourceMiscFlag_SHARED = 1 << 17, // shared texture

		ResourceMiscFlag_VIDEO_COMPATIBILITY_H264 = 1 << 18,	// required for vulkan resource creation for every resource (bitstream buffer, DPB, output) that will be used in a H264 decode session
		ResourceMiscFlag_VIDEO_COMPATIBILITY_H265 = 1 << 19,	// required for vulkan resource creation for every resource (bitstream buffer, DPB, output) that will be used in a H265 decode session
	};

	enum GraphicsDeviceCapability {
		GraphicsDeviceCapability_NONE = 0,
		GraphicsDeviceCapability_TESSELLATION = 1 << 0,
		GraphicsDeviceCapability_CONSERVATIVE_RASTERIZATION = 1 << 1,
		GraphicsDeviceCapability_RASTERIZER_ORDERED_VIEWS = 1 << 2,
		GraphicsDeviceCapability_UAV_LOAD_FORMAT_COMMON = 1 << 3, // eg: R16G16B16A16_FLOAT, R8G8B8A8_UNORM and more common ones
		GraphicsDeviceCapability_UAV_LOAD_FORMAT_R11G11B10_FLOAT = 1 << 4,
		GraphicsDeviceCapability_VARIABLE_RATE_SHADING = 1 << 5,
		GraphicsDeviceCapability_VARIABLE_RATE_SHADING_TIER2 = 1 << 6,
		GraphicsDeviceCapability_MESH_SHADER = 1 << 7,
		GraphicsDeviceCapability_RAYTRACING = 1 << 8,
		GraphicsDeviceCapability_PREDICATION = 1 << 9,
		GraphicsDeviceCapability_SAMPLER_MINMAX = 1 << 10,
		GraphicsDeviceCapability_DEPTH_BOUNDS_TEST = 1 << 11,
		GraphicsDeviceCapability_SPARSE_BUFFER = 1 << 12,
		GraphicsDeviceCapability_SPARSE_TEXTURE2D = 1 << 13,
		GraphicsDeviceCapability_SPARSE_TEXTURE3D = 1 << 14,
		GraphicsDeviceCapability_SPARSE_NULL_MAPPING = 1 << 15,
		GraphicsDeviceCapability_ALIASING_GENERIC = 1 << 16, // allows using ResourceMiscFlag::ALIASING (non resource type specific version)
		GraphicsDeviceCapability_DEPTH_RESOLVE_MIN_MAX = 1 << 17,
		GraphicsDeviceCapability_STENCIL_RESOLVE_MIN_MAX = 1 << 18,
		GraphicsDeviceCapability_CACHE_COHERENT_UMA = 1 << 19,	// https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_architecture
		GraphicsDeviceCapability_VIDEO_DECODE_H264 = 1 << 20,
		GraphicsDeviceCapability_VIDEO_DECODE_H265 = 1 << 21,
		GraphicsDeviceCapability_R9G9B9E5_SHAREDEXP_RENDERABLE = 1 << 22, // indicates supporting R9G9B9E5_SHAREDEXP format for rendering to
		GraphicsDeviceCapability_COPY_BETWEEN_DIFFERENT_IMAGE_ASPECTS_NOT_SUPPORTED = 1 << 23, // indicates that CopyTexture src and dst ImageAspect must match
		
		// Compat:
		GraphicsDeviceCapability_GENERIC_SPARSE_TILE_POOL = GraphicsDeviceCapability_ALIASING_GENERIC,
	};

	enum ResourceState {
		// Common resource states:
		ResourceState_UNDEFINED = 0,						// invalid state (don't preserve contents)
		ResourceState_SHADER_RESOURCE = 1 << 0,			// shader resource, read only
		ResourceState_SHADER_RESOURCE_COMPUTE = 1 << 1,	// shader resource, read only, non-pixel shader
		ResourceState_UNORDERED_ACCESS = 1 << 2,			// shader resource, write enabled
		ResourceState_COPY_SRC = 1 << 3,					// copy from
		ResourceState_COPY_DST = 1 << 4,					// copy to

		// Texture specific resource states:
		ResourceState_RENDERTARGET = 1 << 5,				// render target, write enabled
		ResourceState_DEPTHSTENCIL = 1 << 6,				// depth stencil, write enabled
		ResourceState_DEPTHSTENCIL_READONLY = 1 << 7,		// depth stencil, read only
		ResourceState_SHADING_RATE_SOURCE = 1 << 8,		// shading rate control per tile

		// GPUBuffer specific resource states:
		ResourceState_VERTEX_BUFFER = 1 << 9,				// vertex buffer, read only
		ResourceState_INDEX_BUFFER = 1 << 10,				// index buffer, read only
		ResourceState_CONSTANT_BUFFER = 1 << 11,			// constant buffer, read only
		ResourceState_INDIRECT_ARGUMENT = 1 << 12,		// argument buffer to DrawIndirect() or DispatchIndirect()
		ResourceState_RAYTRACING_ACCELERATION_STRUCTURE = 1 << 13, // acceleration structure storage or scratch
		ResourceState_PREDICATION = 1 << 14,				// storage for predication comparison value

		// Other:
		ResourceState_VIDEO_DECODE_SRC = 1 << 15,			// video decode operation source (bitstream buffer or DPB texture)
		ResourceState_VIDEO_DECODE_DST = 1 << 16,			// video decode operation destination output texture
		ResourceState_VIDEO_DECODE_DPB = 1 << 17,			// video decode operation destination DPB texture
		ResourceState_SWAPCHAIN = 1 << 18,				// resource state of swap chain's back buffer texture when it's not rendering
	};

	enum RenderPassFlags {
		RenderPassFlags_NONE = 0,
		RenderPassFlags_ALLOW_UAV_WRITES = 1 << 0,			// allows UAV writes to happen within render pass
		RenderPassFlags_SUSPENDING = 1 << 1,				// suspends the renderpass to be continued in the next submitted command list
		RenderPassFlags_RESUMING = 1 << 2,					// resumes the renderpass that was suspended in the previously submitted command list
	};

	enum VideoDecoderSupportFlags {
		VideoDecoderSupportFlags_NONE = 0,
		VideoDecoderSupportFlags_DPB_AND_OUTPUT_COINCIDE = 1 << 0,				// the video decoder supports using the DPB texture as output shader resource. If not supported, then DPB_AND_OUTPUT_DISTINCT must be supported.
		VideoDecoderSupportFlags_DPB_AND_OUTPUT_DISTINCT = 1 << 1,				// the video decoder supports outputting to a texture that is not part of the DPB as part of the decode operation. If not supported, then DPB_AND_OUTPUT_COINCIDE must be supported.
		VideoDecoderSupportFlags_DPB_INDIVIDUAL_TEXTURES_SUPPORTED = 1 << 2,		// the video decoder supports using a DPB that is not an array texture, so each slot can be an individually allocated texture
	};


	// Descriptor structs:

	struct Viewport
	{
		float top_left_x = 0;
		float top_left_y = 0;
		float width = 0;
		float height = 0;
		float min_depth = 0;
		float max_depth = 1;
	};

	struct InputLayout
	{
		static const uint32_t APPEND_ALIGNED_ELEMENT = ~0u; // automatically figure out AlignedByteOffset depending on Format

		struct Element
		{
			std::string semantic_name;
			uint32_t semantic_index = 0;
			Format format = Format_UNKNOWN;
			uint32_t input_slot = 0;
			uint32_t aligned_byte_offset = APPEND_ALIGNED_ELEMENT;
			InputClassification input_slot_class = InputClassification_PER_VERTEX_DATA;
		};
		Protean::vector<Element> elements;
	};

	union ClearValue
	{
		float color[4];
		struct ClearDepthStencil
		{
			float depth;
			uint32_t stencil;
		} depth_stencil;
	};

	struct Swizzle
	{
		ComponentSwizzle r = ComponentSwizzle_R;
		ComponentSwizzle g = ComponentSwizzle_G;
		ComponentSwizzle b = ComponentSwizzle_B;
		ComponentSwizzle a = ComponentSwizzle_A;
	};

	struct TextureDesc
	{
		enum Type : uint8_t
		{
			Type_TEXTURE_1D,
			Type_TEXTURE_2D,
			Type_TEXTURE_3D,
		} type = Type_TEXTURE_2D;
		Format format = Format_UNKNOWN;
		Usage usage = Usage_DEFAULT;
		BindFlag bind_flags = BindFlag_NONE;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		uint32_t array_size = 1;
		uint32_t mip_levels = 1;
		uint32_t sample_count = 1;
		ClearValue clear = {};
		Swizzle swizzle;
		ResourceMiscFlag misc_flags = ResourceMiscFlag_NONE;
		ResourceState layout = ResourceState_SHADER_RESOURCE;
	};

	struct SamplerDesc
	{
		Filter filter = Filter_MIN_MAG_MIP_POINT;
		TextureAddressMode address_u = TextureAddressMode_CLAMP;
		TextureAddressMode address_v = TextureAddressMode_CLAMP;
		TextureAddressMode address_w = TextureAddressMode_CLAMP;
		float mip_lod_bias = 0;
		uint32_t max_anisotropy = 0;
		ComparisonFunc comparison_func = ComparisonFunc_NEVER;
		SamplerBorderColor border_color = SamplerBorderColor_TRANSPARENT_BLACK;
		float min_lod = 0;
		float max_lod = std::numeric_limits<float>::max();
	};

	struct RasterizerState
	{
		FillMode fill_mode = FillMode_SOLID;
		CullMode cull_mode = CullMode_NONE;
		bool front_counter_clockwise = false;
		int32_t depth_bias = 0;
		float depth_bias_clamp = 0;
		float slope_scaled_depth_bias = 0;
		bool depth_clip_enable = false;
		bool multisample_enable = false;
		bool antialiased_line_enable = false;
		bool conservative_rasterization_enable = false;
		uint32_t forced_sample_count = 0;
	};
	inline static constexpr RasterizerState default_rasterizerstate;

	struct DepthStencilState
	{
		bool depth_enable = false;
		DepthWriteMask depth_write_mask = DepthWriteMask_ZERO;
		ComparisonFunc depth_func = ComparisonFunc_NEVER;
		bool stencil_enable = false;
		uint8_t stencil_read_mask = 0xff;
		uint8_t stencil_write_mask = 0xff;

		struct DepthStencilOp
		{
			StencilOp stencil_fail_op = StencilOp_KEEP;
			StencilOp stencil_depth_fail_op = StencilOp_KEEP;
			StencilOp stencil_pass_op = StencilOp_KEEP;
			ComparisonFunc stencil_func = ComparisonFunc_NEVER;
		};
		DepthStencilOp front_face;
		DepthStencilOp back_face;
		bool depth_bounds_test_enable = false;
	};
	inline static constexpr DepthStencilState default_depthstencilstate;

	struct BlendState
	{
		bool alpha_to_coverage_enable = false;
		bool independent_blend_enable = false;

		struct RenderTargetBlendState
		{
			bool blend_enable = false;
			Blend src_blend = Blend_SRC_ALPHA;
			Blend dest_blend = Blend_INV_SRC_ALPHA;
			BlendOp blend_op = BlendOp_ADD;
			Blend src_blend_alpha = Blend_ONE;
			Blend dest_blend_alpha = Blend_ONE;
			BlendOp blend_op_alpha = BlendOp_ADD;
			ColorWrite render_target_write_mask = ColorWrite_ENABLE_ALL;
		};
		RenderTargetBlendState render_target[8];
	};
	inline static constexpr BlendState default_blendstate;

	struct GPUBufferDesc
	{
		uint64_t size = 0;
		uint32_t stride = 0; // only needed for structured buffer types!
		uint32_t alignment = 0;
		Usage usage = Usage_DEFAULT;
		Format format = Format_UNKNOWN; // only needed for typed buffer!
		BindFlag bind_flags = BindFlag_NONE;
		ResourceMiscFlag misc_flags = ResourceMiscFlag_NONE;
	};

	struct GPUQueryHeapDesc
	{
		GpuQueryType type = GpuQueryType_TIMESTAMP;
		uint32_t query_count = 0;
	};

	// This structure contains pointers to render state config, the pointers must be valid until the pipeline state is in use!
	struct PipelineStateDesc
	{
		const Shader* vs = nullptr;
		const Shader* ps = nullptr;
		const Shader* hs = nullptr;
		const Shader* ds = nullptr;
		const Shader* gs = nullptr;
		const Shader* ms = nullptr;
		const Shader* as = nullptr;
		const BlendState* bs = nullptr;
		const RasterizerState* rs = nullptr;
		const DepthStencilState* dss = nullptr;
		const InputLayout* il = nullptr;
		PrimitiveTopology pt = PrimitiveTopology_TRIANGLELIST;
		uint32_t patch_control_points = 3;
		uint32_t sample_mask = 0xFFFFFFFF;
	};

	struct GPUBarrier
	{
		enum Type {
			Type_MEMORY,		// UAV accesses
			Type_IMAGE,		// image layout transition
			Type_BUFFER,		// buffer state transition
			Type_ALIASING,	// memory aliasing transition
		} type = Type_MEMORY;

		struct Memory
		{
			const GPUResource* resource;
		};
		struct Image
		{
			const Texture* texture;
			ResourceState layout_before;
			ResourceState layout_after;
			int mip;
			int slice;
			const ImageAspect* aspect;
		};
		struct Buffer
		{
			const GPUBuffer* buffer;
			ResourceState state_before;
			ResourceState state_after;
		};
		struct Aliasing
		{
			const GPUResource* resource_before;
			const GPUResource* resource_after;
		};
		union
		{
			Memory memory;
			Image image;
			Buffer buffer;
			Aliasing aliasing;
		};

		static GPUBarrier Memory(const GPUResource* resource = nullptr)
		{
			GPUBarrier barrier;
			barrier.type = Type_MEMORY;
			barrier.memory.resource = resource;
			return barrier;
		}
		static GPUBarrier Image(const Texture* texture, ResourceState before, ResourceState after,
			int mip = -1, int slice = -1, const ImageAspect* aspect = nullptr)
		{
			GPUBarrier barrier;
			barrier.type = Type_IMAGE;
			barrier.image.texture = texture;
			barrier.image.layout_before = before;
			barrier.image.layout_after = after;
			barrier.image.mip = mip;
			barrier.image.slice = slice;
			barrier.image.aspect = aspect;
			return barrier;
		}
		static GPUBarrier Buffer(const GPUBuffer* buffer, ResourceState before, ResourceState after)
		{
			GPUBarrier barrier;
			barrier.type = Type_BUFFER;
			barrier.buffer.buffer = buffer;
			barrier.buffer.state_before = before;
			barrier.buffer.state_after = after;
			return barrier;
		}
		static GPUBarrier Aliasing(const GPUResource* before, const GPUResource* after)
		{
			GPUBarrier barrier;
			barrier.type = Type_ALIASING;
			barrier.aliasing.resource_before = before;
			barrier.aliasing.resource_after = after;
			return barrier;
		}
	};

	struct SwapChainDesc
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t buffer_count = 2;
		Format format = Format_R10G10B10A2_UNORM;
		bool fullscreen = false;
		bool vsync = true;
		float clear_color[4] = { 0,0,0,1 };
		bool allow_hdr = true;
	};

	struct SubresourceData
	{
		const void* data_ptr = nullptr;	// pointer to the beginning of the subresource data (pointer to beginning of resource + subresource offset)
		uint32_t row_pitch = 0;			// bytes between two rows of a texture (2D and 3D textures)
		uint32_t slice_pitch = 0;		// bytes between two depth slices of a texture (3D textures only)
	};

	struct Rect
	{
		int32_t left = 0;	// start width
		int32_t top = 0;	// start height
		int32_t right = 0;	// end width
		int32_t bottom = 0;	// end height

		constexpr void from_viewport(const Viewport& vp)
		{
			left = int32_t(vp.top_left_x);
			right = int32_t(vp.top_left_x + vp.width);
			top = int32_t(vp.top_left_y);
			bottom = int32_t(vp.top_left_y + vp.height);
		}
	};

	struct Box
	{
		uint32_t left = 0;		// start width
		uint32_t top = 0;		// start height
		uint32_t front = 0;		// start depth
		uint32_t right = 0;		// end width
		uint32_t bottom = 0;	// end height
		uint32_t back = 0;		// end depth
	};

	struct SparseTextureProperties
	{
		uint32_t tile_width = 0;				// width of 1 tile in texels
		uint32_t tile_height = 0;				// height of 1 tile in texels
		uint32_t tile_depth = 0;				// depth of 1 tile in texels
		uint32_t total_tile_count = 0;			// number of tiles for entire resource
		uint32_t packed_mip_start = 0;			// first mip of packed mipmap levels, these cannot be individually mapped and they cannot use a box mapping
		uint32_t packed_mip_count = 0;			// number of packed mipmap levels, these cannot be individually mapped and they cannot use a box mapping
		uint32_t packed_mip_tile_offset = 0;	// offset of the tiles for packed mip data relative to the entire resource
		uint32_t packed_mip_tile_count = 0;		// how many tiles are required for the packed mipmaps
	};

	struct VideoDesc
	{
		uint32_t width = 0;					// must meet the codec specific alignment requirements
		uint32_t height = 0;				// must meet the codec specific alignment requirements
		uint32_t bit_rate = 0;				// can be 0, it means that decoding will be prepared for worst case
		Format format = Format_NV12;
		VideoProfile profile = VideoProfile_H264;
		const void* pps_datas = nullptr;	// array of picture parameter set structures. The structure type depends on video codec
		size_t pps_count = 0;				// number of picture parameter set structures in the pps_datas array
		const void* sps_datas = nullptr;	// array of sequence parameter set structures. The structure type depends on video codec
		size_t sps_count = 0;				// number of sequence parameter set structures in the sps_datas array
		uint32_t num_dpb_slots = 0;			// The number of decode picture buffer slots. Usually it is required to be at least number_of_reference_frames + 1
	};

	struct SizeAlignment
	{
		uint64_t size = 0;
		uint64_t alignment = 0;
	};


	// Resources:

	struct Sampler
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		SamplerDesc desc;

		const SamplerDesc& GetDesc() const { return desc; }
	};

	struct Shader
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		ShaderStage stage = ShaderStage_Count;
	};

	struct GPUResource
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		// These are only valid if the resource was created with CPU access (USAGE::UPLOAD or USAGE::READBACK)
		void* mapped_data = nullptr;	// for buffers, it is a pointer to the buffer data; for textures, it is a pointer to texture data with linear tiling;
		size_t mapped_size = 0;			// for buffers, it is the full buffer size; for textures it is the full texture size including all subresources;

		uint32_t sparse_page_size = 0;	// specifies the required alignment of backing allocation for sparse tile pool

		enum Type : uint8_t
		{
			Type_BUFFER,
			Type_TEXTURE,
			Type_RAYTRACING_ACCELERATION_STRUCTURE,
			Type_UNKNOWN_TYPE,
		} type = Type_UNKNOWN_TYPE;
		constexpr bool IsTexture() const { return type == Type_TEXTURE; }
		constexpr bool IsBuffer() const { return type == Type_BUFFER; }
		constexpr bool IsAccelerationStructure() const { return type == Type_RAYTRACING_ACCELERATION_STRUCTURE; }

		// Dynamic allocation and destruction of this object is not allowed because virtual table is not used. Placement new is allowed.
		static void* operator new (size_t) = delete;
		static void* operator new[](size_t) = delete;
		static void  operator delete (void*) = delete;
		static void  operator delete[](void*) = delete;
		static void* operator new(size_t, void* p) noexcept { return p; }
		static void* operator new[](size_t, void* p) noexcept { return p; }
	};

	struct GPUBuffer final : public GPUResource
	{
		GPUBufferDesc desc;

		constexpr const GPUBufferDesc& GetDesc() const { return desc; }

	};

	struct Texture final : public GPUResource
	{
		TextureDesc	desc;

		// These are only valid if the texture was created with CPU access (USAGE::UPLOAD or USAGE::READBACK)
		const SubresourceData* mapped_subresources = nullptr;	// an array of subresource mappings in the following memory layout: slice0|mip0, slice0|mip1, slice0|mip2, ... sliceN|mipN
		size_t mapped_subresource_count = 0;					// the array size of mapped_subresources (number of slices * number of miplevels)

		// These are only valid if texture was created with ResourceMiscFlag_SPARSE flag:
		const SparseTextureProperties* sparse_properties = nullptr;

#if defined(_WIN32)
		void* shared_handle = nullptr; /* HANDLE */
#else
		int shared_handle = 0;
#endif

		constexpr const TextureDesc& GetDesc() const { return desc; }
	};

	struct VideoDecoder
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		VideoDesc desc;
		constexpr const VideoDesc& GetDesc() const { return desc; }
		VideoDecoderSupportFlags support = VideoDecoderSupportFlags_NONE;
	};

	struct VideoDecodeOperation
	{
		enum Flags
		{
			FLAG_EMPTY = 0,
			FLAG_SESSION_RESET = 1 << 0, // first usage of decoder needs reset
		};
		uint32_t flags = FLAG_EMPTY;
		const GPUBuffer* stream = nullptr;
		uint64_t stream_offset = 0; // must be aligned with GraphicsDevice::GetVideoDecodeBitstreamAlignment()
		uint64_t stream_size = 0;
		VideoFrameType frame_type = VideoFrameType_Intra;
		uint32_t reference_priority = 0; // nal_ref_idc from nal unit header
		int decoded_frame_index = 0; // frame index in order of decoding
		const void* slice_header = nullptr; // slice header for current frame
		const void* pps = nullptr; // picture parameter set for current slice header
		const void* sps = nullptr; // sequence parameter set for current picture parameter set
		int poc[2] = {}; // PictureOrderCount Top and Bottom fields
		uint32_t current_dpb = 0; // DPB slot for current output picture
		uint8_t dpb_reference_count = 0; // number of references in dpb_reference_slots array
		const uint8_t* dpb_reference_slots = nullptr; // dpb slot indices that are used as reference pictures
		const int* dpb_poc = nullptr; // for each DPB reference slot, indicate the PictureOrderCount
		const int* dpb_framenum = nullptr; // for each DPB reference slot, indicate the framenum value
		const Texture* DPB = nullptr; // DPB texture with arraysize = num_references + 1
		const Texture* output = nullptr; // output of the operation, it should be nullptr if DPB_AND_OUTPUT_COINCIDE is used (because in that case the DPB will be used as output instead of a separate output)
	};

	struct RenderPassImage
	{
		enum Type {
			Type_RENDERTARGET,
			Type_DEPTH_STENCIL,
			Type_RESOLVE, // resolve render target (color)
			Type_RESOLVE_DEPTH,
			Type_SHADING_RATE_SOURCE
		} type = Type_RENDERTARGET;
		enum LoadOp {
			LoadOp_LOAD,
			LoadOp_CLEAR,
			LoadOp_DONTCARE,
		} loadop = LoadOp_LOAD;
		const Texture* texture = nullptr;
		int subresource = -1;
		enum StoreOp {
			StoreOp_STORE,
			StoreOp_DONTCARE,
		} storeop = StoreOp_STORE;
		ResourceState layout_before = ResourceState_UNDEFINED;	// layout before the render pass
		ResourceState layout = ResourceState_UNDEFINED;	// layout within the render pass
		ResourceState layout_after = ResourceState_UNDEFINED;	// layout after the render pass
		enum DepthResolveMode {
			DepthResolveMode_Min,
			DepthResolveMode_Max,
		} depth_resolve_mode = DepthResolveMode_Min;

		static RenderPassImage RenderTarget(
			const Texture* resource,
			LoadOp load_op = LoadOp_LOAD,
			StoreOp store_op = StoreOp_STORE,
			ResourceState layout_before = ResourceState_SHADER_RESOURCE,
			ResourceState layout_after = ResourceState_SHADER_RESOURCE,
			int subresource_RTV = -1
		)
		{
			RenderPassImage image;
			image.type = Type_RENDERTARGET;
			image.texture = resource;
			image.loadop = load_op;
			image.storeop = store_op;
			image.layout_before = layout_before;
			image.layout = ResourceState_RENDERTARGET;
			image.layout_after = layout_after;
			image.subresource = subresource_RTV;
			return image;
		}

		static RenderPassImage DepthStencil(
			const Texture* resource,
			LoadOp load_op = LoadOp_LOAD,
			StoreOp store_op = StoreOp_STORE,
			ResourceState layout_before = ResourceState_DEPTHSTENCIL,
			ResourceState layout = ResourceState_DEPTHSTENCIL,
			ResourceState layout_after = ResourceState_DEPTHSTENCIL,
			int subresource_DSV = -1
		)
		{
			RenderPassImage image;
			image.type = Type_DEPTH_STENCIL;
			image.texture = resource;
			image.loadop = load_op;
			image.storeop = store_op;
			image.layout_before = layout_before;
			image.layout = layout;
			image.layout_after = layout_after;
			image.subresource = subresource_DSV;
			return image;
		}

		static RenderPassImage Resolve(
			const Texture* resource,
			ResourceState layout_before = ResourceState_SHADER_RESOURCE,
			ResourceState layout_after = ResourceState_SHADER_RESOURCE,
			int subresource_SRV = -1
		)
		{
			RenderPassImage image;
			image.type = Type_RESOLVE;
			image.texture = resource;
			image.layout_before = layout_before;
			image.layout = ResourceState_COPY_DST;
			image.layout_after = layout_after;
			image.subresource = subresource_SRV;
			return image;
		}

		static RenderPassImage ResolveDepth(
			const Texture* resource,
			DepthResolveMode depth_resolve_mode = DepthResolveMode_Min,
			ResourceState layout_before = ResourceState_SHADER_RESOURCE,
			ResourceState layout_after = ResourceState_SHADER_RESOURCE,
			int subresource_SRV = -1
		)
		{
			RenderPassImage image;
			image.type = Type_RESOLVE_DEPTH;
			image.texture = resource;
			image.layout_before = layout_before;
			image.layout = ResourceState_COPY_DST;
			image.layout_after = layout_after;
			image.subresource = subresource_SRV;
			image.depth_resolve_mode = depth_resolve_mode;
			return image;
		}

		static RenderPassImage ShadingRateSource(
			const Texture* resource,
			ResourceState layout_before = ResourceState_SHADING_RATE_SOURCE,
			ResourceState layout_after = ResourceState_SHADING_RATE_SOURCE
		)
		{
			RenderPassImage image;
			image.type = Type_SHADING_RATE_SOURCE;
			image.texture = resource;
			image.layout_before = layout_before;
			image.layout = ResourceState_SHADING_RATE_SOURCE;
			image.layout_after = layout_after;
			return image;
		}
	};

	struct RenderPassInfo
	{
		Format rt_formats[8] = {};
		uint32_t rt_count = 0;
		Format ds_format = Format_UNKNOWN;
		uint32_t sample_count = 1;

		constexpr uint64_t get_hash() const
		{
			union Hasher
			{
				struct
				{
					uint64_t rt_format_0 : 6;
					uint64_t rt_format_1 : 6;
					uint64_t rt_format_2 : 6;
					uint64_t rt_format_3 : 6;
					uint64_t rt_format_4 : 6;
					uint64_t rt_format_5 : 6;
					uint64_t rt_format_6 : 6;
					uint64_t rt_format_7 : 6;
					uint64_t ds_format : 6;
					uint64_t sample_count : 3;
				} bits;
				uint64_t value;
			} hasher = {};
			static_assert(sizeof(Hasher) == sizeof(uint64_t));
			hasher.bits.rt_format_0 = (uint64_t)rt_formats[0];
			hasher.bits.rt_format_1 = (uint64_t)rt_formats[1];
			hasher.bits.rt_format_2 = (uint64_t)rt_formats[2];
			hasher.bits.rt_format_3 = (uint64_t)rt_formats[3];
			hasher.bits.rt_format_4 = (uint64_t)rt_formats[4];
			hasher.bits.rt_format_5 = (uint64_t)rt_formats[5];
			hasher.bits.rt_format_6 = (uint64_t)rt_formats[6];
			hasher.bits.rt_format_7 = (uint64_t)rt_formats[7];
			hasher.bits.ds_format = (uint64_t)ds_format;
			hasher.bits.sample_count = (uint64_t)sample_count;
			return hasher.value;
		}
		static constexpr RenderPassInfo from(const RenderPassImage* images, uint32_t image_count)
		{
			RenderPassInfo info;
			for (uint32_t i = 0; i < image_count; ++i)
			{
				const RenderPassImage& image = images[i];
				const TextureDesc& desc = image.texture->GetDesc();
				switch (image.type)
				{
				case RenderPassImage::Type_RENDERTARGET:
					info.rt_formats[info.rt_count++] = desc.format;
					info.sample_count = desc.sample_count;
					break;
				case RenderPassImage::Type_DEPTH_STENCIL:
					info.ds_format = desc.format;
					info.sample_count = desc.sample_count;
					break;
				default:
					break;
				}
			}
			return info;
		}
		static constexpr RenderPassInfo from(const SwapChainDesc& swapchain_desc)
		{
			RenderPassInfo info;
			info.rt_count = 1;
			info.rt_formats[0] = swapchain_desc.format;
			return info;
		}
	};

	struct GPUQueryHeap
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		GPUQueryHeapDesc desc;

		constexpr const GPUQueryHeapDesc& GetDesc() const { return desc; }
	};

	struct PipelineState
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		PipelineStateDesc desc;

		constexpr const PipelineStateDesc& GetDesc() const { return desc; }
	};

	struct SwapChain
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		SwapChainDesc desc;

		constexpr const SwapChainDesc& GetDesc() const { return desc; }
	};

	struct RaytracingAccelerationStructureDesc
	{
		enum Flags
		{
			FLAG_EMPTY = 0,
			FLAG_ALLOW_UPDATE = 1 << 0,
			FLAG_ALLOW_COMPACTION = 1 << 1,
			FLAG_PREFER_FAST_TRACE = 1 << 2,
			FLAG_PREFER_FAST_BUILD = 1 << 3,
			FLAG_MINIMIZE_MEMORY = 1 << 4,
		};
		uint32_t flags = FLAG_EMPTY;

		enum Type {
			Type_BOTTOMLEVEL,
			Type_TOPLEVEL,
		} type = Type_BOTTOMLEVEL;

		struct BottomLevel
		{
			struct Geometry
			{
				enum FLAGS
				{
					FLAG_EMPTY = 0,
					FLAG_OPAQUE = 1 << 0,
					FLAG_NO_DUPLICATE_ANYHIT_INVOCATION = 1 << 1,
					FLAG_USE_TRANSFORM = 1 << 2,
				};
				uint32_t flags = FLAG_EMPTY;

				enum Type {
					Type_TRIANGLES,
					Type_PROCEDURAL_AABBS,
				} type = Type_TRIANGLES;

				struct Triangles
				{
					GPUBuffer vertex_buffer;
					GPUBuffer index_buffer;
					uint32_t index_count = 0;
					uint64_t index_offset = 0;
					uint32_t vertex_count = 0;
					uint64_t vertex_byte_offset = 0;
					uint32_t vertex_stride = 0;
					IndexBufferFormat index_format = IndexBufferFormat_UINT32;
					Format vertex_format = Format_R32G32B32_FLOAT;
					GPUBuffer transform_3x4_buffer;
					uint32_t transform_3x4_buffer_offset = 0;
				} triangles;
				struct Procedural_AABBs
				{
					GPUBuffer aabb_buffer;
					uint32_t offset = 0;
					uint32_t count = 0;
					uint32_t stride = 0;
				} aabbs;

			};
			Protean::vector<Geometry> geometries;
		} bottom_level;

		struct TopLevel
		{
			struct Instance
			{
				enum FLAGS
				{
					FLAG_EMPTY = 0,
					FLAG_TRIANGLE_CULL_DISABLE = 1 << 0,
					FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE = 1 << 1,
					FLAG_FORCE_OPAQUE = 1 << 2,
					FLAG_FORCE_NON_OPAQUE = 1 << 3,
				};
				float transform[3][4];
				uint32_t instance_id : 24;
				uint32_t instance_mask : 8;
				uint32_t instance_contribution_to_hit_group_index : 24;
				uint32_t flags : 8;
				const GPUResource* bottom_level = nullptr;
			};
			GPUBuffer instance_buffer;
			uint32_t offset = 0;
			uint32_t count = 0;
		} top_level;
	};
	struct RaytracingAccelerationStructure final : public GPUResource
	{
		RaytracingAccelerationStructureDesc desc;

		size_t size = 0;

		constexpr const RaytracingAccelerationStructureDesc& GetDesc() const { return desc; }
	};

	struct ShaderLibrary
	{
		enum Type {
			Type_RAYGENERATION,
			Type_MISS,
			Type_CLOSESTHIT,
			Type_ANYHIT,
			Type_INTERSECTION,
		} type = Type_RAYGENERATION;
		const Shader* shader = nullptr;
		std::string function_name;
	};
	struct ShaderHitGroup
	{
		enum Type {
			Type_GENERAL, // raygen or miss
			Type_TRIANGLES,
			Type_PROCEDURAL,
		} type = Type_TRIANGLES;
		std::string name;
		uint32_t general_shader = ~0u;
		uint32_t closest_hit_shader = ~0u;
		uint32_t any_hit_shader = ~0u;
		uint32_t intersection_shader = ~0u;
	};
	struct RaytracingPipelineStateDesc
	{
		Protean::vector<ShaderLibrary> shader_libraries;
		Protean::vector<ShaderHitGroup> hit_groups;
		uint32_t max_trace_recursion_depth = 1;
		uint32_t max_attribute_size_in_bytes = 0;
		uint32_t max_payload_size_in_bytes = 0;
	};
	struct RaytracingPipelineState
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		RaytracingPipelineStateDesc desc;

		constexpr const RaytracingPipelineStateDesc& GetDesc() const { return desc; }
	};

	struct PipelineHash
	{
		const PipelineState* pso = nullptr;
		uint64_t renderpass_hash = 0;

		constexpr bool operator==(const PipelineHash& other) const
		{
			return (pso == other.pso) && (renderpass_hash == other.renderpass_hash);
		}
		constexpr uint64_t get_hash() const
		{
			union
			{
				const PipelineState* ptr;
				uint64_t value;
			} pso_hasher = {};
			static_assert(sizeof(pso_hasher) == sizeof(uint64_t));
			pso_hasher.ptr = pso; // reinterpret_cast in constexpr workaround
			return (pso_hasher.value ^ (renderpass_hash << 1)) >> 1;
		}
	};

	struct ShaderTable
	{
		const GPUBuffer* buffer = nullptr;
		uint64_t offset = 0;
		uint64_t size = 0;
		uint64_t stride = 0;
	};
	struct DispatchRaysDesc
	{
		ShaderTable ray_generation;
		ShaderTable miss;
		ShaderTable hit_group;
		ShaderTable callable;
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
	};

	struct SparseResourceCoordinate
	{
		uint32_t x = 0;		// tile offset of buffer or texture in width
		uint32_t y = 0;		// tile offset of texture in height
		uint32_t z = 0;		// tile offset of 3D texture in depth
		uint32_t mip = 0;	// mip level of texture resource
		uint32_t slice = 0;	// array slice of texture resource
	};
	struct SparseRegionSize
	{
		uint32_t width = 1;		// number of tiles to be mapped in X dimension (buffer or texture)
		uint32_t height = 1;	// number of tiles to be mapped in Y dimension (texture only)
		uint32_t depth = 1;		// number of tiles to be mapped in Z dimension (3D texture only)
	};
	enum TileRangeFlags {
		TileRangeFlags_None = 0,		// map page to tile memory
		TileRangeFlags_Null = 1 << 0,	// set page to null
	};
	struct SparseUpdateCommand
	{
		const GPUResource* sparse_resource = nullptr;			// the resource to do sparse mapping for (this requires resource to be created with ResourceMisc::SPARSE)
		uint32_t num_resource_regions = 0;						// number of: coordinates, sizes
		const SparseResourceCoordinate* coordinates = nullptr;	// mapping coordinates within sparse resource (num_resource_regions array size)
		const SparseRegionSize* sizes = nullptr;				// mapping sizes within sparse resource (num_resource_regions array size)
		const GPUBuffer* tile_pool = nullptr;					// this buffer must have been created with ResourceMisc::TILE_POOL
		const TileRangeFlags* range_flags = nullptr;			// flags (num_ranges array size)
		const uint32_t* range_start_offsets = nullptr;			// offset within tile pool (in pages) (num_ranges array size)
		const uint32_t* range_tile_counts = nullptr;			// number of tiles to be mapped (num_ranges array size)
	};

	struct GPUBufferCopyCommand
	{
		const GPUBuffer* dst = nullptr;		// destination buffer object for copy operation
		uint64_t dst_offset = 0;			// offset into destination buffer in bytes
		const GPUBuffer* src = nullptr;		// source buffer object for copy operation
		uint64_t src_offset = 0;			// offset into source buffer in bytes
		uint64_t size = 0;					// size of copy operation in bytes
	};


	constexpr bool IsFormatSRGB(Format format)
	{
		switch (format)
		{
		case Format_R8G8B8A8_UNORM_SRGB:
		case Format_B8G8R8A8_UNORM_SRGB:
		case Format_BC1_UNORM_SRGB:
		case Format_BC2_UNORM_SRGB:
		case Format_BC3_UNORM_SRGB:
		case Format_BC7_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}
	constexpr bool IsFormatUnorm(Format format)
	{
		switch (format)
		{
		case Format_R16G16B16A16_UNORM:
		case Format_R10G10B10A2_UNORM:
		case Format_R8G8B8A8_UNORM:
		case Format_R8G8B8A8_UNORM_SRGB:
		case Format_B8G8R8A8_UNORM:
		case Format_B8G8R8A8_UNORM_SRGB:
		case Format_R16G16_UNORM:
		case Format_D24_UNORM_S8_UINT:
		case Format_R8G8_UNORM:
		case Format_D16_UNORM:
		case Format_R16_UNORM:
		case Format_R8_UNORM:
		case Format_BC1_UNORM:
		case Format_BC1_UNORM_SRGB:
		case Format_BC2_UNORM:
		case Format_BC2_UNORM_SRGB:
		case Format_BC3_UNORM:
		case Format_BC3_UNORM_SRGB:
		case Format_BC4_UNORM:
		case Format_BC5_UNORM:
		case Format_BC7_UNORM:
		case Format_BC7_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}
	constexpr bool IsFormatBlockCompressed(Format format)
	{
		switch (format)
		{
		case Format_BC1_UNORM:
		case Format_BC1_UNORM_SRGB:
		case Format_BC2_UNORM:
		case Format_BC2_UNORM_SRGB:
		case Format_BC3_UNORM:
		case Format_BC3_UNORM_SRGB:
		case Format_BC4_UNORM:
		case Format_BC4_SNORM:
		case Format_BC5_UNORM:
		case Format_BC5_SNORM:
		case Format_BC6H_UF16:
		case Format_BC6H_SF16:
		case Format_BC7_UNORM:
		case Format_BC7_UNORM_SRGB:
			return true;
		default:
			return false;
		}
	}
	constexpr bool IsFormatDepthSupport(Format format)
	{
		switch (format)
		{
		case Format_D16_UNORM:
		case Format_D32_FLOAT:
		case Format_D32_FLOAT_S8X24_UINT:
		case Format_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}
	constexpr bool IsFormatStencilSupport(Format format)
	{
		switch (format)
		{
		case Format_D32_FLOAT_S8X24_UINT:
		case Format_D24_UNORM_S8_UINT:
			return true;
		default:
			return false;
		}
	}
	constexpr uint32_t GetFormatPlaneCount(Format format)
	{
		switch (format)
		{
		case Format_D32_FLOAT_S8X24_UINT:
		case Format_D24_UNORM_S8_UINT:
		case Format_NV12:
			return 2;
		default:
			return 1;
		}
	}
	constexpr uint32_t GetFormatBlockSize(Format format)
	{
		if(IsFormatBlockCompressed(format))
		{
			return 4u;
		}
		return 1u;
	}

	// Returns the byte size of one element for a given texture format
	//	For uncompressed formats, element = pixel
	//	For block compressed formats, element = block
	//	plane can be specified for planar formats to get stride of specific plane, or left as default ~0u to get stride as if planes were interleaved
	constexpr uint32_t GetFormatStride(Format format, uint32_t plane = ~0u)
	{
		switch (format)
		{
		case Format_R32G32B32A32_FLOAT:
		case Format_R32G32B32A32_UINT:
		case Format_R32G32B32A32_SINT:
		case Format_BC2_UNORM:
		case Format_BC2_UNORM_SRGB:
		case Format_BC3_UNORM:
		case Format_BC3_UNORM_SRGB:
		case Format_BC5_SNORM:
		case Format_BC5_UNORM:
		case Format_BC6H_UF16:
		case Format_BC6H_SF16:
		case Format_BC7_UNORM:
		case Format_BC7_UNORM_SRGB:
			return 16u;

		case Format_R32G32B32_FLOAT:
		case Format_R32G32B32_UINT:
		case Format_R32G32B32_SINT:
			return 12u;

		case Format_R16G16B16A16_FLOAT:
		case Format_R16G16B16A16_UNORM:
		case Format_R16G16B16A16_UINT:
		case Format_R16G16B16A16_SNORM:
		case Format_R16G16B16A16_SINT:
		case Format_R32G32_FLOAT:
		case Format_R32G32_UINT:
		case Format_R32G32_SINT:
		case Format_BC1_UNORM:
		case Format_BC1_UNORM_SRGB:
		case Format_BC4_SNORM:
		case Format_BC4_UNORM:
			return 8u;

		case Format_R10G10B10A2_UNORM:
		case Format_R10G10B10A2_UINT:
		case Format_R11G11B10_FLOAT:
		case Format_R8G8B8A8_UNORM:
		case Format_R8G8B8A8_UNORM_SRGB:
		case Format_R8G8B8A8_UINT:
		case Format_R8G8B8A8_SNORM:
		case Format_R8G8B8A8_SINT:
		case Format_B8G8R8A8_UNORM:
		case Format_B8G8R8A8_UNORM_SRGB:
		case Format_R16G16_FLOAT:
		case Format_R16G16_UNORM:
		case Format_R16G16_UINT:
		case Format_R16G16_SNORM:
		case Format_R16G16_SINT:
		case Format_D32_FLOAT:
		case Format_R32_FLOAT:
		case Format_R32_UINT:
		case Format_R32_SINT:
		case Format_R9G9B9E5_SHAREDEXP:
			return 4u;

		case Format_R8G8_UNORM:
		case Format_R8G8_UINT:
		case Format_R8G8_SNORM:
		case Format_R8G8_SINT:
		case Format_R16_FLOAT:
		case Format_D16_UNORM:
		case Format_R16_UNORM:
		case Format_R16_UINT:
		case Format_R16_SNORM:
		case Format_R16_SINT:
			return 2u;

		case Format_R8_UNORM:
		case Format_R8_UINT:
		case Format_R8_SNORM:
		case Format_R8_SINT:
			return 1u;

		case Format_D24_UNORM_S8_UINT:
			return plane == ~0u ? 4u : (plane == 0 ? 3u : 1u);
		case Format_D32_FLOAT_S8X24_UINT:
			return plane == ~0u ? 8u : (plane == 0 ? 4u : 1u); // 8 is chosen for interleaved instead of 5 for safe alignment
		case Format_NV12:
			return plane == ~0u ? 2u : 1u; // warning: really it should be 1 byte for luminance, 2 bytes for 2x2 chrominance, so this is overestimated as 1 for both

		default:
			assert(0); // didn't catch format!
			return 16u;
		}
	}
	constexpr Format GetFormatNonSRGB(Format format)
	{
		switch (format)
		{
		case Format_R8G8B8A8_UNORM_SRGB:
			return Format_R8G8B8A8_UNORM;
		case Format_B8G8R8A8_UNORM_SRGB:
			return Format_B8G8R8A8_UNORM;
		case Format_BC1_UNORM_SRGB:
			return Format_BC1_UNORM;
		case Format_BC2_UNORM_SRGB:
			return Format_BC2_UNORM;
		case Format_BC3_UNORM_SRGB:
			return Format_BC3_UNORM;
		case Format_BC7_UNORM_SRGB:
			return Format_BC7_UNORM;
		default:
			return format;
		}
	}
	constexpr Format GetFormatSRGB(Format format)
	{
		switch (format)
		{
		case Format_R8G8B8A8_UNORM:
		case Format_R8G8B8A8_UNORM_SRGB:
			return Format_R8G8B8A8_UNORM_SRGB;
		case Format_B8G8R8A8_UNORM:
		case Format_B8G8R8A8_UNORM_SRGB:
			return Format_B8G8R8A8_UNORM_SRGB;
		case Format_BC1_UNORM:
		case Format_BC1_UNORM_SRGB:
			return Format_BC1_UNORM_SRGB;
		case Format_BC2_UNORM:
		case Format_BC2_UNORM_SRGB:
			return Format_BC2_UNORM_SRGB;
		case Format_BC3_UNORM:
		case Format_BC3_UNORM_SRGB:
			return Format_BC3_UNORM_SRGB;
		case Format_BC7_UNORM:
		case Format_BC7_UNORM_SRGB:
			return Format_BC7_UNORM_SRGB;
		default:
			return Format_UNKNOWN;
		}
	}
	constexpr const char* GetFormatString(Format format)
	{
		switch (format)
		{
		case Protean::Graphics::Format_UNKNOWN:
			return "UNKNOWN";
		case Protean::Graphics::Format_R32G32B32A32_FLOAT:
			return "R32G32B32A32_FLOAT";
		case Protean::Graphics::Format_R32G32B32A32_UINT:
			return "R32G32B32A32_UINT";
		case Protean::Graphics::Format_R32G32B32A32_SINT:
			return "R32G32B32A32_SINT";
		case Protean::Graphics::Format_R32G32B32_FLOAT:
			return "R32G32B32_FLOAT";
		case Protean::Graphics::Format_R32G32B32_UINT:
			return "R32G32B32_UINT";
		case Protean::Graphics::Format_R32G32B32_SINT:
			return "R32G32B32_SINT";
		case Protean::Graphics::Format_R16G16B16A16_FLOAT:
			return "R16G16B16A16_FLOAT";
		case Protean::Graphics::Format_R16G16B16A16_UNORM:
			return "R16G16B16A16_UNORM";
		case Protean::Graphics::Format_R16G16B16A16_UINT:
			return "R16G16B16A16_UINT";
		case Protean::Graphics::Format_R16G16B16A16_SNORM:
			return "R16G16B16A16_SNORM";
		case Protean::Graphics::Format_R16G16B16A16_SINT:
			return "R16G16B16A16_SINT";
		case Protean::Graphics::Format_R32G32_FLOAT:
			return "R32G32_FLOAT";
		case Protean::Graphics::Format_R32G32_UINT:
			return "R32G32_UINT";
		case Protean::Graphics::Format_R32G32_SINT:
			return "R32G32_SINT";
		case Protean::Graphics::Format_D32_FLOAT_S8X24_UINT:
			return "D32_FLOAT_S8X24_UINT";
		case Protean::Graphics::Format_R10G10B10A2_UNORM:
			return "R10G10B10A2_UNORM";
		case Protean::Graphics::Format_R10G10B10A2_UINT:
			return "R10G10B10A2_UINT";
		case Protean::Graphics::Format_R11G11B10_FLOAT:
			return "R11G11B10_FLOAT";
		case Protean::Graphics::Format_R8G8B8A8_UNORM:
			return "R8G8B8A8_UNORM";
		case Protean::Graphics::Format_R8G8B8A8_UNORM_SRGB:
			return "R8G8B8A8_UNORM_SRGB";
		case Protean::Graphics::Format_R8G8B8A8_UINT:
			return "R8G8B8A8_UINT";
		case Protean::Graphics::Format_R8G8B8A8_SNORM:
			return "R8G8B8A8_SNORM";
		case Protean::Graphics::Format_R8G8B8A8_SINT:
			return "R8G8B8A8_SINT";
		case Protean::Graphics::Format_B8G8R8A8_UNORM:
			return "B8G8R8A8_UNORM";
		case Protean::Graphics::Format_B8G8R8A8_UNORM_SRGB:
			return "B8G8R8A8_UNORM_SRGB";
		case Protean::Graphics::Format_R16G16_FLOAT:
			return "R16G16_FLOAT";
		case Protean::Graphics::Format_R16G16_UNORM:
			return "R16G16_UNORM";
		case Protean::Graphics::Format_R16G16_UINT:
			return "R16G16_UINT";
		case Protean::Graphics::Format_R16G16_SNORM:
			return "R16G16_SNORM";
		case Protean::Graphics::Format_R16G16_SINT:
			return "R16G16_SINT";
		case Protean::Graphics::Format_D32_FLOAT:
			return "D32_FLOAT";
		case Protean::Graphics::Format_R32_FLOAT:
			return "R32_FLOAT";
		case Protean::Graphics::Format_R32_UINT:
			return "R32_UINT";
		case Protean::Graphics::Format_R32_SINT:
			return "R32_SINT";
		case Protean::Graphics::Format_D24_UNORM_S8_UINT:
			return "D24_UNORM_S8_UINT";
		case Protean::Graphics::Format_R9G9B9E5_SHAREDEXP:
			return "R9G9B9E5_SHAREDEXP";
		case Protean::Graphics::Format_R8G8_UNORM:
			return "R8G8_UNORM";
		case Protean::Graphics::Format_R8G8_UINT:
			return "R8G8_UINT";
		case Protean::Graphics::Format_R8G8_SNORM:
			return "R8G8_SNORM";
		case Protean::Graphics::Format_R8G8_SINT:
			return "R8G8_SINT";
		case Protean::Graphics::Format_R16_FLOAT:
			return "R16_FLOAT";
		case Protean::Graphics::Format_D16_UNORM:
			return "D16_UNORM";
		case Protean::Graphics::Format_R16_UNORM:
			return "R16_UNORM";
		case Protean::Graphics::Format_R16_UINT:
			return "R16_UINT";
		case Protean::Graphics::Format_R16_SNORM:
			return "R16_SNORM";
		case Protean::Graphics::Format_R16_SINT:
			return "R16_SINT";
		case Protean::Graphics::Format_R8_UNORM:
			return "R8_UNORM";
		case Protean::Graphics::Format_R8_UINT:
			return "R8_UINT";
		case Protean::Graphics::Format_R8_SNORM:
			return "R8_SNORM";
		case Protean::Graphics::Format_R8_SINT:
			return "R8_SINT";
		case Protean::Graphics::Format_BC1_UNORM:
			return "BC1_UNORM";
		case Protean::Graphics::Format_BC1_UNORM_SRGB:
			return "BC1_UNORM_SRGB";
		case Protean::Graphics::Format_BC2_UNORM:
			return "BC2_UNORM";
		case Protean::Graphics::Format_BC2_UNORM_SRGB:
			return "BC2_UNORM_SRGB";
		case Protean::Graphics::Format_BC3_UNORM:
			return "BC3_UNORM";
		case Protean::Graphics::Format_BC3_UNORM_SRGB:
			return "BC3_UNORM_SRGB";
		case Protean::Graphics::Format_BC4_UNORM:
			return "BC4_UNORM";
		case Protean::Graphics::Format_BC4_SNORM:
			return "BC4_SNORM";
		case Protean::Graphics::Format_BC5_UNORM:
			return "BC5_UNORM";
		case Protean::Graphics::Format_BC5_SNORM:
			return "BC5_SNORM";
		case Protean::Graphics::Format_BC6H_UF16:
			return "BC6H_UF16";
		case Protean::Graphics::Format_BC6H_SF16:
			return "BC6H_SF16";
		case Protean::Graphics::Format_BC7_UNORM:
			return "BC7_UNORM";
		case Protean::Graphics::Format_BC7_UNORM_SRGB:
			return "BC7_UNORM_SRGB";
		case Protean::Graphics::Format_NV12:
			return "NV12";
		default:
			return "";
		}
	}
	constexpr IndexBufferFormat GetIndexBufferFormat(Format format)
	{
		switch (format)
		{
		default:
		case Format_R32_UINT:
			return IndexBufferFormat_UINT32;
		case Format_R16_UINT:
			return IndexBufferFormat_UINT16;
		}
	}
	constexpr IndexBufferFormat GetIndexBufferFormat(uint32_t vertex_count)
	{
		return vertex_count > 65536 ? IndexBufferFormat_UINT32 : IndexBufferFormat_UINT16;
	}
	constexpr Format GetIndexBufferFormatRaw(uint32_t vertex_count)
	{
		return vertex_count > 65536 ? Format_R32_UINT : Format_R16_UINT;
	}
	constexpr uint32_t GetIndexStride(IndexBufferFormat format)
	{
		switch (format) {
			default:
			case IndexBufferFormat_UINT32:
				return sizeof(uint32_t);
			case IndexBufferFormat_UINT16:
				return sizeof(uint16_t);
		}
	}
	constexpr const char* GetIndexBufferFormatString(IndexBufferFormat format)
	{
		switch (format)
		{
		default:
		case IndexBufferFormat_UINT32:
			return "UINT32";
		case IndexBufferFormat_UINT16:
			return "UINT16";
		}
	}

	constexpr const char GetComponentSwizzleChar(ComponentSwizzle value)
	{
		switch (value)
		{
		default:
		case Protean::Graphics::ComponentSwizzle_R:
			return 'R';
		case Protean::Graphics::ComponentSwizzle_G:
			return 'G';
		case Protean::Graphics::ComponentSwizzle_B:
			return 'B';
		case Protean::Graphics::ComponentSwizzle_A:
			return 'A';
		case Protean::Graphics::ComponentSwizzle_ZERO:
			return '0';
		case Protean::Graphics::ComponentSwizzle_ONE:
			return '1';
		}
	}
	struct SwizzleString
	{
		char chars[5] = {};
		constexpr operator const char*() const { return chars; }
	};
	constexpr const SwizzleString GetSwizzleString(Swizzle swizzle)
	{
		SwizzleString ret;
		ret.chars[0] = GetComponentSwizzleChar(swizzle.r);
		ret.chars[1] = GetComponentSwizzleChar(swizzle.g);
		ret.chars[2] = GetComponentSwizzleChar(swizzle.b);
		ret.chars[3] = GetComponentSwizzleChar(swizzle.a);
		ret.chars[4] = 0;
		return ret;
	}
	constexpr Swizzle SwizzleFromString(const char* str)
	{
		Swizzle swizzle;
		if (str == nullptr)
			return swizzle;
		ComponentSwizzle* comp = (ComponentSwizzle*)&swizzle;
		for (int i = 0; i < 4; ++i)
		{
			switch (str[i])
			{
			case 'r':
			case 'R':
			case 'x':
			case 'X':
				*comp = ComponentSwizzle_R;
				break;
			case 'g':
			case 'G':
			case 'y':
			case 'Y':
				*comp = ComponentSwizzle_G;
				break;
			case 'b':
			case 'B':
			case 'z':
			case 'Z':
				*comp = ComponentSwizzle_B;
				break;
			case 'a':
			case 'A':
			case 'w':
			case 'W':
				*comp = ComponentSwizzle_A;
				break;
			case '0':
				*comp = ComponentSwizzle_ZERO;
				break;
			case '1':
				*comp = ComponentSwizzle_ONE;
				break;
			case 0:
				return swizzle;
			}
			comp++;
		}
		return swizzle;
	}

	// Get mipmap count for a given texture dimension.
	//	width, height, depth: dimensions of the texture
	//	min_dimension: break when all dimensions go below a specified dimension (optional, default: 1x1x1)
	constexpr uint32_t GetMipCount(uint32_t width, uint32_t height, uint32_t depth = 1u, uint32_t min_dimension = 1u)
	{
		uint32_t mips = 1;
		while (width > min_dimension || height > min_dimension || depth > min_dimension)
		{
			width = std::max(min_dimension, width >> 1u);
			height = std::max(min_dimension, height >> 1u);
			depth = std::max(min_dimension, depth >> 1u);
			mips++;
		}
		return mips;
	}

	// Get mipmap count from a texture description (if the description specified 0 mipmaps then this will return the max allowed)
	constexpr uint32_t GetMipCount(const TextureDesc& desc)
	{
		return desc.mip_levels == 0 ? GetMipCount(desc.width, desc.height, desc.depth) : desc.mip_levels;
	}

	// Returns the plane slice index for an aspect
	constexpr uint32_t GetPlaneSlice(ImageAspect aspect)
	{
		switch (aspect)
		{
		case Protean::Graphics::ImageAspect_COLOR:
		case Protean::Graphics::ImageAspect_DEPTH:
		case Protean::Graphics::ImageAspect_LUMINANCE:
			return 0;
		case Protean::Graphics::ImageAspect_STENCIL:
		case Protean::Graphics::ImageAspect_CHROMINANCE:
			return 1;
		default:
			assert(0); // invalid aspect
			break;
		}
		return 0;
	}

	// Returns ImageAspect based on plane and format
	constexpr ImageAspect GetImageAspect(Format format, uint32_t plane)
	{
		switch (format)
		{
		case Format_D24_UNORM_S8_UINT:
		case Format_D32_FLOAT_S8X24_UINT:
			return plane == 0 ? ImageAspect_DEPTH : ImageAspect_STENCIL;
		case Format_NV12:
			return plane == 0 ? ImageAspect_LUMINANCE : ImageAspect_CHROMINANCE;
		default:
			return IsFormatDepthSupport(format) ? ImageAspect_DEPTH : ImageAspect_COLOR;
		}
	}

	// Computes the subresource index for indexing SubresourceData arrays
	constexpr uint32_t ComputeSubresource(uint32_t mip, uint32_t slice, uint32_t plane, uint32_t mip_count, uint32_t array_size)
	{
		return mip + slice * mip_count + plane * mip_count * array_size;
	}

	// Computes the subresource index for indexing SubresourceData arrays
	constexpr uint32_t ComputeSubresource(uint32_t mip, uint32_t slice, ImageAspect aspect, uint32_t mip_count, uint32_t array_size)
	{
		return ComputeSubresource(mip, slice, GetPlaneSlice(aspect), mip_count, array_size);
	}

	// Compute the texture memory usage for one row in a specific mip level
	//	plane can be specified optionally, the value ~0u means all planes
	constexpr size_t ComputeTextureMipRowPitch(const TextureDesc& desc, uint32_t mip, uint32_t plane = ~0u)
	{
		const uint32_t bytes_per_block = GetFormatStride(desc.format, plane);
		const uint32_t pixels_per_block = GetFormatBlockSize(desc.format);
		const uint32_t mip_width = std::max(1u, desc.width >> mip);
		const uint32_t num_blocks_x = (mip_width + pixels_per_block - 1) / pixels_per_block;
		return num_blocks_x * bytes_per_block * desc.sample_count;
	}

	// Compute the texture memory usage for one mip level
	//	plane can be specified optionally, the value ~0u means all planes
	constexpr size_t ComputeTextureMipMemorySizeInBytes(const TextureDesc& desc, uint32_t mip, uint32_t plane = ~0u)
	{
		const uint32_t bytes_per_block = GetFormatStride(desc.format, plane);
		const uint32_t pixels_per_block = GetFormatBlockSize(desc.format);
		const uint32_t mip_width = std::max(1u, desc.width >> mip);
		const uint32_t mip_height = std::max(1u, desc.height >> mip);
		const uint32_t mip_depth = std::max(1u, desc.depth >> mip);
		const uint32_t num_blocks_x = (mip_width + pixels_per_block - 1) / pixels_per_block;
		const uint32_t num_blocks_y = (mip_height + pixels_per_block - 1) / pixels_per_block;
		return num_blocks_x * num_blocks_y * mip_depth * bytes_per_block * desc.sample_count;
	}

	// Compute the texture memory usage of the tightly packed pixel data
	//	This doesn't reflect GPU specific texture memory requirements, like alignment and metadata.
	//	For full resource placement allocation size the GraphicsDevice::GetDeviceTextureMemoryRequirements should be used
	//	plane can be specified optionally, the value ~0u means all planes
	constexpr size_t ComputeTextureMemorySizeInBytes(const TextureDesc& desc, uint32_t plane = ~0u)
	{
		// An uninitialized / not-yet-created texture (e.g. a component whose
		// GPU resource has not been built yet) has an UNKNOWN format and no
		// memory footprint. Return early so we don't hit GetFormatStride's
		// unhandled- format assert on a legitimately empty descriptor.
		if (desc.format == Format_UNKNOWN)
		{
			return 0;
		}

		size_t size = 0;
		const uint32_t bytes_per_block = GetFormatStride(desc.format, plane);
		const uint32_t pixels_per_block = GetFormatBlockSize(desc.format);
		const uint32_t mips = GetMipCount(desc);
		for (uint32_t layer = 0; layer < desc.array_size; ++layer)
		{
			for (uint32_t mip = 0; mip < mips; ++mip)
			{
				const uint32_t mip_width = std::max(1u, desc.width >> mip);
				const uint32_t mip_height = std::max(1u, desc.height >> mip);
				const uint32_t mip_depth = std::max(1u, desc.depth >> mip);
				const uint32_t num_blocks_x = (mip_width + pixels_per_block - 1) / pixels_per_block;
				const uint32_t num_blocks_y = (mip_height + pixels_per_block - 1) / pixels_per_block;
				size += num_blocks_x * num_blocks_y * mip_depth * bytes_per_block;
			}
		}
		size *= desc.sample_count;
		return size;
	}

	constexpr uint32_t GetTextureSubresourceCount(const TextureDesc& desc)
	{
		const uint32_t planes = GetFormatPlaneCount(desc.format);
		const uint32_t mips = GetMipCount(desc);
		return desc.array_size * mips * planes;
	}

	// Creates texture SubresourceData array
	//	alignment	: it can be used to force GPU-specific rowpitch alignment for linear tile mode (in bytes)
	inline void CreateTextureSubresourceDatas(const TextureDesc& desc, void* data_ptr, Protean::vector<SubresourceData>& subresource_datas, uint32_t alignment = 1)
	{
		const uint32_t planes = GetFormatPlaneCount(desc.format);
		const uint32_t pixels_per_block = GetFormatBlockSize(desc.format);
		const uint32_t mips = GetMipCount(desc);
		subresource_datas.resize(GetTextureSubresourceCount(desc));
		size_t subresource_data_offset = 0;
		for (uint32_t plane = 0; plane < planes; ++plane)
		{
			const uint32_t bytes_per_block = GetFormatStride(desc.format, plane);
			for (uint32_t layer = 0; layer < desc.array_size; ++layer)
			{
				for (uint32_t mip = 0; mip < mips; ++mip)
				{
					const uint32_t mip_width = std::max(1u, desc.width >> mip);
					const uint32_t mip_height = std::max(1u, desc.height >> mip);
					const uint32_t mip_depth = std::max(1u, desc.depth >> mip);
					const uint32_t num_blocks_x = (mip_width + pixels_per_block - 1) / pixels_per_block;
					const uint32_t num_blocks_y = (mip_height + pixels_per_block - 1) / pixels_per_block;
					const uint32_t subresource = ComputeSubresource(mip, layer, plane, mips, desc.array_size);
					SubresourceData& subresource_data = subresource_datas[subresource];
					subresource_data.data_ptr = (uint8_t*)data_ptr + subresource_data_offset;
					subresource_data.row_pitch = align((uint32_t)num_blocks_x * bytes_per_block, alignment);
					subresource_data.slice_pitch = (uint32_t)subresource_data.row_pitch * num_blocks_y;
					subresource_data_offset += subresource_data.slice_pitch * mip_depth;
				}
			}
		}
	}


	WI_DISABLE_DEPRECATED_BEGIN
	// Deprecated, kept for back-compat:
	struct [[deprecated]] RenderPassAttachment
	{
		enum Type {
			Type_RENDERTARGET,
			Type_DEPTH_STENCIL,
			Type_RESOLVE, // resolve render target (color)
			Type_RESOLVE_DEPTH,
			Type_SHADING_RATE_SOURCE
		} type = Type_RENDERTARGET;
		enum LoadOp {
			LoadOp_LOAD,
			LoadOp_CLEAR,
			LoadOp_DONTCARE,
		} loadop = LoadOp_LOAD;
		Texture texture;
		int subresource = -1;
		enum StoreOp {
			StoreOp_STORE,
			StoreOp_DONTCARE,
		} storeop = StoreOp_STORE;
		ResourceState initial_layout = ResourceState_UNDEFINED;	// layout before the render pass
		ResourceState subpass_layout = ResourceState_UNDEFINED;	// layout within the render pass
		ResourceState final_layout = ResourceState_UNDEFINED;		// layout after the render pass
		enum DepthResolveMode {
			DepthResolveMode_Min,
			DepthResolveMode_Max,
		} depth_resolve_mode = DepthResolveMode_Min;

		static RenderPassAttachment RenderTarget(
			const Texture& resource,
			LoadOp load_op = LoadOp_LOAD,
			StoreOp store_op = StoreOp_STORE,
			ResourceState initial_layout = ResourceState_SHADER_RESOURCE,
			ResourceState subpass_layout = ResourceState_RENDERTARGET,
			ResourceState final_layout = ResourceState_SHADER_RESOURCE,
			int subresource_RTV = -1
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type_RENDERTARGET;
			attachment.texture = resource;
			attachment.loadop = load_op;
			attachment.storeop = store_op;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = subpass_layout;
			attachment.final_layout = final_layout;
			attachment.subresource = subresource_RTV;
			return attachment;
		}

		static RenderPassAttachment DepthStencil(
			const Texture& resource,
			LoadOp load_op = LoadOp_LOAD,
			StoreOp store_op = StoreOp_STORE,
			ResourceState initial_layout = ResourceState_DEPTHSTENCIL,
			ResourceState subpass_layout = ResourceState_DEPTHSTENCIL,
			ResourceState final_layout = ResourceState_DEPTHSTENCIL,
			int subresource_DSV = -1
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type_DEPTH_STENCIL;
			attachment.texture = resource;
			attachment.loadop = load_op;
			attachment.storeop = store_op;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = subpass_layout;
			attachment.final_layout = final_layout;
			attachment.subresource = subresource_DSV;
			return attachment;
		}

		static RenderPassAttachment Resolve(
			const Texture& resource,
			ResourceState initial_layout = ResourceState_SHADER_RESOURCE,
			ResourceState final_layout = ResourceState_SHADER_RESOURCE,
			int subresource_SRV = -1
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type_RESOLVE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.final_layout = final_layout;
			attachment.subresource = subresource_SRV;
			return attachment;
		}

		static RenderPassAttachment ResolveDepth(
			const Texture& resource,
			DepthResolveMode depth_resolve_mode = DepthResolveMode_Min,
			ResourceState initial_layout = ResourceState_SHADER_RESOURCE,
			ResourceState final_layout = ResourceState_SHADER_RESOURCE,
			int subresource_SRV = -1
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type_RESOLVE_DEPTH;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.final_layout = final_layout;
			attachment.subresource = subresource_SRV;
			attachment.depth_resolve_mode = depth_resolve_mode;
			return attachment;
		}

		static RenderPassAttachment ShadingRateSource(
			const Texture& resource,
			ResourceState initial_layout = ResourceState_SHADING_RATE_SOURCE,
			ResourceState final_layout = ResourceState_SHADING_RATE_SOURCE
		)
		{
			RenderPassAttachment attachment;
			attachment.type = Type_SHADING_RATE_SOURCE;
			attachment.texture = resource;
			attachment.initial_layout = initial_layout;
			attachment.subpass_layout = ResourceState_SHADING_RATE_SOURCE;
			attachment.final_layout = final_layout;
			return attachment;
		}

		constexpr operator RenderPassImage() const
		{
			RenderPassImage image;
			switch (type)
			{
			default:
			case Type_RENDERTARGET:
				image.type = RenderPassImage::Type_RENDERTARGET;
				break;
			case Type_DEPTH_STENCIL:
				image.type = RenderPassImage::Type_DEPTH_STENCIL;
				break;
			case Type_RESOLVE:
				image.type = RenderPassImage::Type_RESOLVE;
				break;
			case Type_RESOLVE_DEPTH:
				image.type = RenderPassImage::Type_RESOLVE_DEPTH;
				break;
			case Type_SHADING_RATE_SOURCE:
				image.type = RenderPassImage::Type_SHADING_RATE_SOURCE;
				break;
			}
			switch (depth_resolve_mode)
			{
			default:
			case DepthResolveMode_Min:
				image.depth_resolve_mode = RenderPassImage::DepthResolveMode_Min;
				break;
			case DepthResolveMode_Max:
				image.depth_resolve_mode = RenderPassImage::DepthResolveMode_Max;
				break;
			}
			switch (loadop)
			{
			case RenderPassAttachment::LoadOp_LOAD:
				image.loadop = RenderPassImage::LoadOp_LOAD;
				break;
			case RenderPassAttachment::LoadOp_CLEAR:
				image.loadop = RenderPassImage::LoadOp_CLEAR;
				break;
			case RenderPassAttachment::LoadOp_DONTCARE:
				image.loadop = RenderPassImage::LoadOp_DONTCARE;
				break;
			default:
				break;
			}
			switch (storeop)
			{
			case RenderPassAttachment::StoreOp_STORE:
				image.storeop = RenderPassImage::StoreOp_STORE;
				break;
			case RenderPassAttachment::StoreOp_DONTCARE:
				image.storeop = RenderPassImage::StoreOp_DONTCARE;
				break;
			default:
				break;
			}
			image.layout_before = initial_layout;
			image.layout = subpass_layout;
			image.layout_after = final_layout;
			image.texture = &texture;
			image.subresource = subresource;
			return image;
		}
	};
	// Deprecated, kept for back-compat:
	struct [[deprecated]] RenderPassDesc
	{
		enum Flags {
			Flags_EMPTY = 0,
			Flags_ALLOW_UAV_WRITES = 1 << 0,
		};
		Flags flags = Flags_EMPTY;
		Protean::vector<RenderPassAttachment> attachments;
	};
	// Deprecated, kept for back-compat:
	struct [[deprecated]] RenderPass
	{
		bool valid = false;
		RenderPassDesc desc;

		constexpr const RenderPassDesc& GetDesc() const { return desc; }
		constexpr bool IsValid() const { return valid; }
	};
	WI_DISABLE_DEPRECATED_END
}

template<>
struct enable_bitmask_operators<Protean::Graphics::ColorWrite> : std::true_type {};
template<>
struct enable_bitmask_operators<Protean::Graphics::BindFlag> : std::true_type {};
template<>
struct enable_bitmask_operators<Protean::Graphics::ResourceMiscFlag> : std::true_type {};
template<>
struct enable_bitmask_operators<Protean::Graphics::GraphicsDeviceCapability> : std::true_type {};
template<>
struct enable_bitmask_operators<Protean::Graphics::ResourceState> : std::true_type {};
WI_DISABLE_DEPRECATED_BEGIN
template<>
struct enable_bitmask_operators<Protean::Graphics::RenderPassDesc::Flags> : std::true_type {};
WI_DISABLE_DEPRECATED_END
template<>
struct enable_bitmask_operators<Protean::Graphics::RenderPassFlags> : std::true_type {};
template<>
struct enable_bitmask_operators<Protean::Graphics::VideoDecoderSupportFlags> : std::true_type {};

namespace std
{
	template <>
	struct hash<Protean::Graphics::PipelineHash>
	{
		inline uint64_t operator()(const Protean::Graphics::PipelineHash& hash) const
		{
			return hash.get_hash();
		}
	};
}

// ---------------------------------------------------------------------------
// Graphics device interface
// ---------------------------------------------------------------------------

namespace Protean::Graphics
{
	// CommandList can be used to record graphics commands from a CPU thread
	//	Use GraphicsDevice::BeginCommandList() to start a command list
	//	Use GraphicsDevice::SubmitCommandLists() to give all started command lists to the GPU for execution
	//	CommandList recording is not thread safe
	struct CommandList
	{
		void* internal_state = nullptr;
		constexpr bool IsValid() const { return internal_state != nullptr; }
	};

	// Descriptor binding counts:
	//	It's OK increase these limits if not enough
	//	But it's better to refactor shaders to use bindless descriptors if they require more resources
	static constexpr uint32_t DESCRIPTORBINDER_CBV_COUNT = 14;
	static constexpr uint32_t DESCRIPTORBINDER_SRV_COUNT = 16;
	static constexpr uint32_t DESCRIPTORBINDER_UAV_COUNT = 16;
	static constexpr uint32_t DESCRIPTORBINDER_SAMPLER_COUNT = 8;
	static constexpr uint32_t DESCRIPTORBINDER_ALL_COUNT = DESCRIPTORBINDER_CBV_COUNT + DESCRIPTORBINDER_SRV_COUNT + DESCRIPTORBINDER_UAV_COUNT + DESCRIPTORBINDER_SAMPLER_COUNT;
	struct DescriptorBindingTable
	{
		GPUBuffer CBV[DESCRIPTORBINDER_CBV_COUNT];
		uint64_t CBV_offset[DESCRIPTORBINDER_CBV_COUNT] = {};
		GPUResource SRV[DESCRIPTORBINDER_SRV_COUNT];
		int SRV_index[DESCRIPTORBINDER_SRV_COUNT] = {};
		GPUResource UAV[DESCRIPTORBINDER_UAV_COUNT];
		int UAV_index[DESCRIPTORBINDER_UAV_COUNT] = {};
		Sampler SAM[DESCRIPTORBINDER_SAMPLER_COUNT];
	};

	// Bindless descriptor limits:
	//	The device can allocate less than this, depending on capabilities
	static constexpr uint32_t BINDLESS_RESOURCE_CAPACITY = 500000;
	static constexpr uint32_t BINDLESS_SAMPLER_CAPACITY = 256; // it is chosen to be addressable by 8 bits

	// Max number of push constants:
	//	The number indicates how many 32 bit values can be set for common shaders
	static constexpr uint32_t PUSH_CONSTANT_COUNT = 22;

	// How many constant buffers are in the "root"
	//	These are faster to bind but there is a more limited number available, depending on device
	static constexpr uint32_t ROOT_CBV_COUNT = 3;

	// static sampler slot bindings will begin from this number
	static constexpr uint32_t STATIC_SAMPLER_SLOT_BEGIN = 100;
	static constexpr uint32_t STATIC_SAMPLER_COUNT = 10;

	enum QUEUE_TYPE
	{
		QUEUE_GRAPHICS,
		QUEUE_COMPUTE,
		QUEUE_COPY,
		QUEUE_VIDEO_DECODE,

		QUEUE_COUNT,
	};

	class GraphicsDevice
	{
	protected:
		static constexpr uint32_t BUFFERCOUNT = 2;
		uint64_t FRAMECOUNT = 0;
		size_t SHADER_IDENTIFIER_SIZE = 0;
		size_t TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = 0;
		uint64_t TIMESTAMP_FREQUENCY = 0;
		uint64_t VIDEO_DECODE_BITSTREAM_ALIGNMENT = 1u;
		GraphicsDeviceCapability capabilities = GraphicsDeviceCapability_NONE;
		uint32_t VARIABLE_RATE_SHADING_TILE_SIZE = 0;
		uint32_t vendorId = 0;
		uint32_t deviceId = 0;
		std::string adapterName;
		std::string driverDescription;
		ValidationMode validationMode = ValidationMode_Disabled;
		AdapterType adapterType = AdapterType_Other;

	public:
		virtual ~GraphicsDevice() = default;

		// Create a SwapChain. If the SwapChain is to be recreated, the window handle can be nullptr.
		virtual bool CreateSwapChain(const SwapChainDesc* desc, Protean::Platform::window_type window, SwapChain* swapchain) const = 0;
		// Create a buffer with a callback to initialize its data. Note: don't read from callback's dest pointer, reads will be very slow! Use memcpy to write to it to make sure only writes happen!
		virtual bool CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void* dest)>& init_callback, GPUBuffer* buffer, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const = 0;
		virtual bool CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const = 0;
		virtual bool CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader, const char* entrypoint = "main") const = 0;
		virtual bool CreateSampler(const SamplerDesc* desc, Sampler* sampler) const = 0;
		virtual bool CreateQueryHeap(const GPUQueryHeapDesc* desc, GPUQueryHeap* queryheap) const = 0;
		// Creates a graphics pipeline state. If renderpass_info is specified, then it will be only compatible with that renderpass info, but it will be created immediately (it can also take longer to be created)
		virtual bool CreatePipelineState(const PipelineStateDesc* desc, PipelineState* pso, const RenderPassInfo* renderpass_info = nullptr) const = 0;
		virtual bool CreateRaytracingAccelerationStructure(const RaytracingAccelerationStructureDesc* desc, RaytracingAccelerationStructure* bvh) const { return false; }
		virtual bool CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const { return false; }
		virtual bool CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const { return false; };

		virtual int CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change = nullptr, const ImageAspect* aspect = nullptr, const Swizzle* swizzle = nullptr, float min_lod_clamp = 0) const = 0;
		virtual int CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size = ~0, const Format* format_change = nullptr, const uint32_t* structuredbuffer_stride_change = nullptr) const = 0;

		virtual void DeleteSubresources(GPUResource* resource) = 0;

		virtual int GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource = -1) const = 0;
		virtual int GetDescriptorIndex(const Sampler* sampler) const = 0;

		virtual void WriteShadingRateValue(ShadingRate rate, void* dest) const {};
		virtual void WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const {}
		virtual void WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const {}

		// Set a debug name which will be visible in graphics debuggers
		virtual void SetName(GPUResource* pResource, const char* name) const {}
		virtual void SetName(Shader* shader, const char* name) const {}

		// Begin a new command list for GPU command recording.
		//	This will be valid until SubmitCommandLists() is called.
		virtual CommandList BeginCommandList(QUEUE_TYPE queue = QUEUE_GRAPHICS) = 0;
		// Submit all command list that were used with BeginCommandList before this call.
		//	This will make every command list to be in "available" state and restarts them
		virtual void SubmitCommandLists() = 0;

		// The CPU will wait until all submitted GPU work is finished execution
		virtual void WaitForGPU() const = 0;

		// The current PipelineState cache will be cleared. It is useful to clear this when reloading shaders, to avoid accumulating unused pipeline states
		virtual void ClearPipelineStateCache() = 0;

		// Returns the number of active pipelines. Active pipelines are the pipelines that were compiled internally for a set of render target formats
		//	One PipelineState object can be compiled internally for multiple render target or depth-stencil formats, or sample counts
		virtual size_t GetActivePipelineCount() const = 0;

		// Returns the number of elapsed frames (submits)
		//	It is incremented when calling SubmitCommandLists()
		constexpr uint64_t GetFrameCount() const { return FRAMECOUNT; }

		// Check whether the graphics device supports a feature or not
		constexpr bool CheckCapability(GraphicsDeviceCapability capability) const { return has_flag(capabilities, capability); }

		// Returns the buffer count, which is the array size of buffered resources used by both the CPU and GPU
		static constexpr uint32_t GetBufferCount() { return BUFFERCOUNT; }
		// Returns the current buffer index, which is in range [0, GetBufferCount() - 1]
		constexpr uint32_t GetBufferIndex() const { return GetFrameCount() % GetBufferCount(); }

		// Returns whether the graphics debug layer is enabled. It can be enabled when creating the device.
		constexpr bool IsDebugDevice() const { return validationMode != ValidationMode_Disabled; }

		// Get GPU-specific metrics:
		constexpr size_t GetShaderIdentifierSize() const { return SHADER_IDENTIFIER_SIZE; }
		constexpr size_t GetTopLevelAccelerationStructureInstanceSize() const { return TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE; }
		constexpr uint32_t GetVariableRateShadingTileSize() const { return VARIABLE_RATE_SHADING_TILE_SIZE; }
		constexpr uint64_t GetTimestampFrequency() const { return TIMESTAMP_FREQUENCY; }
		constexpr uint64_t GetVideoDecodeBitstreamAlignment() const { return VIDEO_DECODE_BITSTREAM_ALIGNMENT; }

		// Get information about the graphics device manufacturer:
		constexpr uint32_t GetVendorId() const { return vendorId; }
		constexpr uint32_t GetDeviceId() const { return deviceId; }
		constexpr const std::string& GetAdapterName() const { return adapterName; }
		constexpr const std::string& GetDriverDescription() const { return driverDescription; }
		constexpr AdapterType GetAdapterType() const { return adapterType; }

		// Get the shader binary format that the underlying graphics API consumes
		virtual ShaderFormat GetShaderFormat() const = 0;

		// Get a Texture resource that represents the current back buffer of the SwapChain
		virtual Texture GetBackBuffer(const SwapChain* swapchain) const = 0;
		// Returns the current color space of the swapchain output
		virtual ColorSpace GetSwapChainColorSpace(const SwapChain* swapchain) const = 0;
		// Returns true if the swapchain could support HDR output regardless of current format
		//	Returns false if the swapchain couldn't support HDR output
		virtual bool IsSwapChainSupportsHDR(const SwapChain* swapchain) const = 0;

		// Returns the minimum required alignment for buffer offsets when creating subresources
		virtual uint32_t GetMinOffsetAlignment(const GPUBufferDesc* desc) const = 0;

		// Returns the real GPU dependent memory allocation size of a texture.
		//	It includes GPU-specific memory allocations, not only the pixel data size. For pixel data size only, use the ComputeTextureMemorySizeInBytes helper function
		virtual SizeAlignment GetDeviceTextureMemoryRequirements(const TextureDesc* desc) const = 0;

		struct MemoryUsage
		{
			uint64_t budget = 0ull;		// total video memory available for use by the current application (in bytes)
			uint64_t usage = 0ull;		// used video memory by the current application (in bytes)
		};
		// Returns video memory statistics for the current application
		virtual MemoryUsage GetMemoryUsage() const = 0;

		// Returns the maximum amount of viewports that can be bound at once
		virtual uint32_t GetMaxViewportCount() const = 0;

		// Performs a batched mapping of sparse resource pages to a tile pool
		virtual void SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count) {};

		// Copies buffer data, submitted immediately to a GPU COPY QUEUE independently from the SubmitCommandLists, so this can straddle frame boundaries. All other queues will be synced to the completion of this.
		//	Name can be provided for debugging purposes optionally
		virtual void CopyBufferAsync(const GPUBufferCopyCommand* commands, uint32_t command_count, const char* name = nullptr) const {}

		// Returns an identifier string for the graphics device subclass
		virtual const char* GetTag() const { return ""; }

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Command List functions are below:
		//	- These are used to record rendering commands to a CommandList
		//	- To get a CommandList that can be recorded into, call BeginCommandList()
		//	- These commands are not immediately executed, but they begin executing on the GPU after calling SubmitCommandLists()
		//	- These are not thread safe, only a single thread should use a single CommandList at one time

		// Tell the command list to wait for an other command list which was started before it
		//	The granularity of this is at least that the beginning of the command list will wait for the end of the other command list
		//	On some platform like PS5 this can be implemented by waiting exactly at the wait insertion point within the command lists which is more precise
		virtual void WaitCommandList(CommandList cmd, CommandList wait_for) = 0;
		virtual void RenderPassBegin(const SwapChain* swapchain, CommandList cmd) = 0;
		virtual void RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags = RenderPassFlags_NONE) = 0;
		virtual void RenderPassBegin(const RenderPassImage* images, uint32_t image_count, const GPUQueryHeap* occlusionqueries, CommandList cmd, RenderPassFlags flags = RenderPassFlags_NONE) { RenderPassBegin(images,image_count,cmd,flags); }
		virtual void RenderPassEnd(CommandList cmd) = 0;
		virtual void BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd) = 0;
		virtual void BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd) = 0;
		virtual void BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) = 0;
		virtual void BindResources(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) = 0;
		virtual void BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource = -1) = 0;
		virtual void BindUAVs(const GPUResource *const* resources, uint32_t slot, uint32_t count, CommandList cmd) = 0;
		virtual void BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd) = 0;
		virtual void BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset = 0ull) = 0;
		virtual void BindVertexBuffers(const GPUBuffer *const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd) = 0;
		virtual void BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd) = 0;
		virtual void BindStencilRef(uint32_t value, CommandList cmd) = 0;
		virtual void BindBlendFactor(float r, float g, float b, float a, CommandList cmd) = 0;
		virtual void BindShadingRate(ShadingRate rate, CommandList cmd) {}
		virtual void BindPipelineState(const PipelineState* pso, CommandList cmd) = 0;
		virtual void BindComputeShader(const Shader* cs, CommandList cmd) = 0;
		virtual void BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd) = 0;
		virtual void Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd) = 0;
		virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd) = 0;
		virtual void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
		virtual void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd) = 0;
		virtual void DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) = 0;
		virtual void DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) = 0;
		virtual void DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) = 0;
		virtual void DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) = 0;
		virtual void Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) = 0;
		virtual void DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) = 0;
		virtual void DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd) {}
		virtual void DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd) {}
		virtual void DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd) {}
		virtual void CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd) = 0;
		virtual void CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd) = 0;
		virtual void CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox = nullptr, ImageAspect dst_aspect = ImageAspect_COLOR, ImageAspect src_aspect = ImageAspect_COLOR) = 0;
		virtual void QueryBegin(const GPUQueryHeap *heap, uint32_t index, CommandList cmd) = 0;
		virtual void QueryEnd(const GPUQueryHeap *heap, uint32_t index, CommandList cmd) = 0;
		virtual void QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd) = 0;
		virtual void QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd) {}
		virtual void Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd) = 0;
		virtual void BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src = nullptr) {}
		virtual void BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd) {}
		virtual void DispatchRays(const DispatchRaysDesc* desc, CommandList cmd) {}
		virtual void PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset = 0) = 0;
		virtual void PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd) {}
		virtual void PredicationEnd(CommandList cmd) {}
		virtual void ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd) = 0;
		virtual void VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd) {}

		virtual void EventBegin(const char* name, CommandList cmd) = 0;
		virtual void EventEnd(CommandList cmd) = 0;
		virtual void SetMarker(const char* name, CommandList cmd) = 0;

		virtual RenderPassInfo GetRenderPassInfo(CommandList cmd) = 0;

		// Some useful helpers:

		// This can be used to create a buffer filled from CPU data pointer, the copy will be done on the CPU (and additional GPU copy for non-UMA)
		bool CreateBuffer(const GPUBufferDesc* desc, const void* initial_data, GPUBuffer* buffer, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const
		{
			if (initial_data == nullptr)
			{
				return CreateBuffer2(desc, nullptr, buffer, alias, alias_offset);
			}
			return CreateBuffer2(desc, [&](void* dest) { std::memcpy(dest, initial_data, desc->size); }, buffer, alias, alias_offset);
		}

		// This can be used to create a buffer filled with a single value, the data initialization will be done on the CPU (and additional GPU copy for non-UMA)
		bool CreateBufferCleared(const GPUBufferDesc* desc, uint8_t value, GPUBuffer* buffer, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const
		{
			return CreateBuffer2(desc, [&](void* dest) { std::memset(dest, value, desc->size); }, buffer, alias, alias_offset);
		}

		// This can be used to create a buffer filled with a single value, the data initialization will be done on the CPU (and additional GPU copy for non-UMA)
		//	This version can clear with larger than byte types but you must ensure that buffer size is multiple of the clear value size
		template <typename T>
		bool CreateBufferClearedWithType(const GPUBufferDesc* desc, T value, GPUBuffer* buffer, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const
		{
			return CreateBuffer2(desc, [&](void* dest) { std::fill((T*)dest, (T*)dest + (desc->size / sizeof(T)), value); }, buffer, alias, alias_offset);
		}

		// This can be used to create a buffer filled with zeroes, the data initialization will be done on the CPU (and additional GPU copy for non-UMA)
		bool CreateBufferZeroed(const GPUBufferDesc* desc, GPUBuffer* buffer, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const
		{
			return CreateBufferCleared(desc, 0, buffer, alias, alias_offset);
		}

		// This can be used to create a texture filled with a single value
		bool CreateTextureCleared(const TextureDesc* desc, uint8_t value, Texture* texture, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const
		{
			Protean::vector<uint8_t> texturedata(ComputeTextureMemorySizeInBytes(*desc), value);
			Protean::vector<SubresourceData> initdata;
			CreateTextureSubresourceDatas(*desc, texturedata.data(), initdata);
			return CreateTexture(desc, initdata.data(), texture, alias, alias_offset);
		}

		// This can be used to create a texture filled with zeroes
		bool CreateTextureZeroed(const TextureDesc* desc, Texture* texture, const GPUResource* alias = nullptr, uint64_t alias_offset = 0ull) const
		{
			return CreateTextureCleared(desc, 0, texture, alias, alias_offset);
		}

		// Execute a single GPU barrier
		void Barrier(const GPUBarrier& barrier, CommandList cmd)
		{
			Barrier(&barrier, 1, cmd);
		}

		// Execute a global GPU memory barrier
		void Barrier(CommandList cmd)
		{
			Barrier(GPUBarrier::Memory(), cmd);
		}

		struct GPULinearAllocator
		{
			GPUBuffer buffer;
			uint64_t offset = 0ull;
			uint64_t alignment = 0ull;
			void reset()
			{
				offset = 0ull;
			}
		};
		virtual GPULinearAllocator& GetFrameAllocator(CommandList cmd) = 0;

		struct GPUAllocation
		{
			void* data = nullptr;	// application can write to this. Reads might be not supported or slow. The offset is already applied
			GPUBuffer buffer;		// application can bind it to the GPU
			uint64_t offset = 0;	// allocation's offset from the GPUbuffer's beginning

			// Returns true if the allocation was successful
			inline bool IsValid() const { return data != nullptr && buffer.IsValid(); }
		};

		// Allocates temporary memory that the CPU can write and GPU can read. 
		//	It is only alive for one frame and automatically invalidated after that.
		GPUAllocation AllocateGPU(uint64_t dataSize, CommandList cmd)
		{
			GPUAllocation allocation;
			if (dataSize == 0)
				return allocation;

			GPULinearAllocator& allocator = GetFrameAllocator(cmd);

			const uint64_t free_space = allocator.buffer.desc.size - allocator.offset;
			if (dataSize > free_space)
			{
				GPUBufferDesc desc;
				desc.usage = Usage_UPLOAD;
				desc.bind_flags = BindFlag_CONSTANT_BUFFER | BindFlag_VERTEX_BUFFER | BindFlag_INDEX_BUFFER | BindFlag_SHADER_RESOURCE;
				desc.misc_flags = ResourceMiscFlag_BUFFER_RAW;
				if (CheckCapability(GraphicsDeviceCapability_RAYTRACING))
				{
					desc.misc_flags |= ResourceMiscFlag_RAY_TRACING;
				}
				allocator.alignment = GetMinOffsetAlignment(&desc);
				desc.size = align((allocator.buffer.desc.size + dataSize) * 2, allocator.alignment);
				CreateBuffer(&desc, nullptr, &allocator.buffer);
				SetName(&allocator.buffer, "frame_allocator");
				allocator.offset = 0;
			}

			allocation.buffer = allocator.buffer;
			allocation.offset = allocator.offset;
			allocation.data = (void*)((size_t)allocator.buffer.mapped_data + allocator.offset);

			allocator.offset += align(dataSize, allocator.alignment);

			assert(allocation.IsValid());
			return allocation;
		}

		// Updates a Usage_DEFAULT buffer data
		//	Since it uses a GPU Copy operation, appropriate synchronization is expected
		//	And it cannot be used inside a RenderPass
		void UpdateBuffer(const GPUBuffer* buffer, const void* data, CommandList cmd, uint64_t size = ~0, uint64_t offset = 0)
		{
			if (buffer == nullptr || data == nullptr)
				return;
			size = std::min(buffer->desc.size, size);
			if (size == 0)
				return;
			GPUAllocation allocation = AllocateGPU(size, cmd);
			std::memcpy(allocation.data, data, size);
			CopyBuffer(buffer, offset, &allocation.buffer, allocation.offset, size, cmd);
		}

		// Helper util to bind a constant buffer with data for a specific command list:
		//	This will be done on the CPU to an UPLOAD buffer, so this can be used inside a RenderPass
		//	But this will be only visible on the command list it was bound to
		template<typename T>
		void BindDynamicConstantBuffer(const T& data, uint32_t slot, CommandList cmd)
		{
			GPUAllocation allocation = AllocateGPU(sizeof(T), cmd);
			std::memcpy(allocation.data, &data, sizeof(T));
			BindConstantBuffer(&allocation.buffer, slot, cmd, allocation.offset);
		}

		// Simplified renderpass beginning for a single render target and optional clear (if clear is false, it will use load operation)
		void RenderPassBegin(const Texture* rendertarget, CommandList cmd, bool clear = true)
		{
			RenderPassImage rp[] = {
				RenderPassImage::RenderTarget(rendertarget, clear ? RenderPassImage::LoadOp_CLEAR : RenderPassImage::LoadOp_LOAD),
			};
			RenderPassBegin(rp, arraysize(rp), cmd);
		}

		// Creates subresources for mipgen where each number directly refers to either SRV or UAV for the same numbered mip level:
		void CreateMipgenSubresources(Texture& texture)
		{
			for (uint32_t i = 0; i < texture.desc.mip_levels; ++i)
			{
				int subresource_index;
				subresource_index = CreateSubresource(&texture, SubresourceType_SRV, 0, 1, i, 1);
				assert(subresource_index == i);
				subresource_index = CreateSubresource(&texture, SubresourceType_UAV, 0, 1, i, 1);
				assert(subresource_index == i);
			}
		}

		// Submits every queued GPU work so far and waits for GPU to finish to free up resources
		void FlushDeallocations()
		{
			for (uint32_t i = 0; i < BUFFERCOUNT; ++i)
			{
				SubmitCommandLists();
			}
		}

		// Resource aliasing helper utility
		struct AliasingAllocator
		{
			GraphicsDevice* device = nullptr;
			uint64_t size = 0;
			uint64_t alignment = 0;
			bool rt_ds = false;

			// Add a single texture into the heap, return its offset
			uint64_t Add(const TextureDesc* desc)
			{
				rt_ds |= has_flag(desc->bind_flags, BindFlag_RENDER_TARGET) | has_flag(desc->bind_flags, BindFlag_DEPTH_STENCIL);
				SizeAlignment sizealign = device->GetDeviceTextureMemoryRequirements(desc);
				size = align(size, sizealign.alignment);
				alignment = std::max(alignment, sizealign.alignment);
				uint64_t offset = size;
				size += sizealign.size;
				return offset;
			}

			// Add multiple textures into the heap on the same offset, aliasing each others' memory, returns the single offset for all of them
			uint64_t Add(std::initializer_list<const TextureDesc*> descs)
			{
				uint64_t batch_size = 0;
				uint64_t batch_alignment = 0;
				for (const TextureDesc* desc : descs)
				{
					rt_ds |= has_flag(desc->bind_flags, BindFlag_RENDER_TARGET) | has_flag(desc->bind_flags, BindFlag_DEPTH_STENCIL);
					SizeAlignment sizealign = device->GetDeviceTextureMemoryRequirements(desc);
					batch_size = std::max(batch_size, sizealign.size);
					batch_alignment = std::max(batch_alignment, sizealign.alignment);
				}
				size = align(size, batch_alignment);
				alignment = std::max(alignment, batch_alignment);
				uint64_t offset = size;
				size += batch_size;
				return offset;
			}

			// Return the buffer that can hold all the resources that were allocated up to this point
			GPUBuffer Commit()
			{
				GPUBuffer heap;
				GPUBufferDesc bd;
				bd.usage = Usage_DEFAULT;
				bd.size = size;
				bd.alignment = (uint32_t)alignment;
				bd.misc_flags = rt_ds ? ResourceMiscFlag_ALIASING_TEXTURE_RT_DS : ResourceMiscFlag_ALIASING_TEXTURE_NON_RT_DS;
				device->CreateBuffer(&bd, nullptr, &heap);
				return heap;
			}
		};
		AliasingAllocator CreateAliasingAllocator()
		{
			AliasingAllocator ret;
			ret.device = this;
			return ret;
		}

		WI_DISABLE_DEPRECATED_BEGIN
		// Deprecated, kept for back-compat:
		[[deprecated]]
		bool CreateRenderPass(const RenderPassDesc* desc, RenderPass* renderpass) const
		{
			renderpass->valid = true;
			renderpass->desc = *desc;
			return true;
		}
		// Deprecated, kept for back-compat:
		[[deprecated]]
		void RenderPassBegin(const RenderPass* renderpass, CommandList cmd)
		{
			RenderPassFlags flags = {};
			if (has_flag(renderpass->desc.flags, RenderPassDesc::Flags_ALLOW_UAV_WRITES))
			{
				flags |= RenderPassFlags_ALLOW_UAV_WRITES;
			}
			RenderPassImage rp[32] = {};
			for (size_t i = 0; i < renderpass->desc.attachments.size(); ++i)
			{
				rp[i] = renderpass->desc.attachments[i];
			}
			RenderPassBegin(rp, (uint32_t)renderpass->desc.attachments.size(), cmd, flags);
		}
		WI_DISABLE_DEPRECATED_END
	};


	// This is a helper to get access to a global device instance
	//	- The engine uses this, but it is not necessary to use a single global device object
	//	- This is not a lifetime managing object, just a way to globally expose a reference to an object by pointer
	//	- Defined in rhi.cpp so that all modules share the same device
	GraphicsDevice*& GetDevice();

}
