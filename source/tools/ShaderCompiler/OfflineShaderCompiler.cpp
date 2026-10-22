#include "Protean.h"

#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>
#include <cstdlib>

class nullbuf_t : public std::streambuf
{
protected:
	virtual int_type overflow(int_type ch) override
	{
		return traits_type::not_eof(ch);
	}
} nullbuf;

std::mutex locker;
struct ShaderEntry
{
	std::string name;
	Protean::Graphics::ShaderStage stage = Protean::Graphics::ShaderStage::Count;
	Protean::Graphics::ShaderModel minshadermodel = Protean::Graphics::ShaderModel::SM_6_0;
	struct Permutation
	{
		Protean::vector<std::string> defines;
		Permutation() = default;
		Permutation(std::initializer_list<std::string> init)
		{
			for (auto& x : init)
			{
				defines.push_back(x);
			}
		}
	};
	Protean::vector<Permutation> permutations;
	std::string entrypoint = "main";
};
Protean::vector<ShaderEntry> shaders = {
	{"hairparticle_simulateCS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS", Protean::Graphics::ShaderStage::CS},
	{"generateMIPChainCubeCS_float4", Protean::Graphics::ShaderStage::CS},
	{"generateMIPChainCubeArrayCS_float4", Protean::Graphics::ShaderStage::CS},
	{"generateMIPChain3DCS_float4", Protean::Graphics::ShaderStage::CS},
	{"generateMIPChain2DCS_float4", Protean::Graphics::ShaderStage::CS},
	{"blockcompressCS_BC1", Protean::Graphics::ShaderStage::CS},
	{"blockcompressCS_BC3", Protean::Graphics::ShaderStage::CS},
	{"blockcompressCS_BC4", Protean::Graphics::ShaderStage::CS},
	{"blockcompressCS_BC5", Protean::Graphics::ShaderStage::CS},
	{"blockcompressCS_BC6H", Protean::Graphics::ShaderStage::CS},
	{"blockcompressCS_BC6H_cubemap", Protean::Graphics::ShaderStage::CS},
	{"blur_gaussian_float4CS", Protean::Graphics::ShaderStage::CS},
	{"bloomseparateCS", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_mainCS", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_neighborhoodMaxCOCCS", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_prepassCS", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_upsampleCS", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_tileMaxCOC_verticalCS", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_tileMaxCOC_horizontalCS", Protean::Graphics::ShaderStage::CS},
	{"vxgi_offsetprevCS", Protean::Graphics::ShaderStage::CS},
	{"vxgi_temporalCS", Protean::Graphics::ShaderStage::CS},
	{"vxgi_sdf_jumpfloodCS", Protean::Graphics::ShaderStage::CS},
	{"vxgi_resolve_diffuseCS", Protean::Graphics::ShaderStage::CS},
	{"vxgi_resolve_specularCS", Protean::Graphics::ShaderStage::CS},
	{"upsample_bilateral_float1CS", Protean::Graphics::ShaderStage::CS},
	{"upsample_bilateral_float4CS", Protean::Graphics::ShaderStage::CS},
	{"temporalaaCS", Protean::Graphics::ShaderStage::CS},
	{"tonemapCS", Protean::Graphics::ShaderStage::CS},
	{"underwaterCS", Protean::Graphics::ShaderStage::CS},
	{"mesh_blend_prepareCS", Protean::Graphics::ShaderStage::CS},
	{"mesh_blend_expandCS", Protean::Graphics::ShaderStage::CS},
	{"mesh_blendPS", Protean::Graphics::ShaderStage::PS},
	{"fsr_upscalingCS", Protean::Graphics::ShaderStage::CS},
	{"fsr_sharpenCS", Protean::Graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_autogen_reactive_pass", Protean::Graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_compute_luminance_pyramid_pass", Protean::Graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_prepare_input_color_pass", Protean::Graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_reconstruct_previous_depth_pass", Protean::Graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_depth_clip_pass", Protean::Graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_lock_pass", Protean::Graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_accumulate_pass", Protean::Graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_rcas_pass", Protean::Graphics::ShaderStage::CS},
	{"ssaoCS", Protean::Graphics::ShaderStage::CS},
	{"ssgi_deinterleaveCS", Protean::Graphics::ShaderStage::CS},
	{"ssgiCS", Protean::Graphics::ShaderStage::CS},
	{"ssgi_upsampleCS", Protean::Graphics::ShaderStage::CS},
	{"rtdiffuseCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_5},
	{"rtdiffuse_spatialCS", Protean::Graphics::ShaderStage::CS},
	{"rtdiffuse_temporalCS", Protean::Graphics::ShaderStage::CS},
	{"rtdiffuse_upsampleCS", Protean::Graphics::ShaderStage::CS},
	{"rtreflectionCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_5},
	{"ssr_tileMaxRoughness_horizontalCS", Protean::Graphics::ShaderStage::CS},
	{"ssr_tileMaxRoughness_verticalCS", Protean::Graphics::ShaderStage::CS},
	{"ssr_depthHierarchyCS", Protean::Graphics::ShaderStage::CS},
	{"ssr_resolveCS", Protean::Graphics::ShaderStage::CS},
	{"ssr_temporalCS", Protean::Graphics::ShaderStage::CS},
	{"ssr_upsampleCS", Protean::Graphics::ShaderStage::CS},
	{"ssr_raytraceCS", Protean::Graphics::ShaderStage::CS},
	{"ssr_raytraceCS_cheap", Protean::Graphics::ShaderStage::CS},
	{"ssr_raytraceCS_earlyexit", Protean::Graphics::ShaderStage::CS},
	{"sharpenCS", Protean::Graphics::ShaderStage::CS},
	{"crt_screenCS", Protean::Graphics::ShaderStage::CS},
	{"skinningCS", Protean::Graphics::ShaderStage::CS},
	{"resolveMSAADepthStencilCS", Protean::Graphics::ShaderStage::CS},
	{"raytraceCS", Protean::Graphics::ShaderStage::CS},
	{"raytraceCS_rtapi", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_5},
	{"paint_textureCS", Protean::Graphics::ShaderStage::CS},
	{"oceanUpdateDisplacementMapCS", Protean::Graphics::ShaderStage::CS},
	{"oceanUpdateGradientFoldingCS", Protean::Graphics::ShaderStage::CS},
	{"oceanSimulatorCS", Protean::Graphics::ShaderStage::CS},
	{"msao_interleaveCS", Protean::Graphics::ShaderStage::CS},
	{"msao_preparedepthbuffers1CS", Protean::Graphics::ShaderStage::CS},
	{"msao_preparedepthbuffers2CS", Protean::Graphics::ShaderStage::CS},
	{"msao_blurupsampleCS", Protean::Graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_blendout", Protean::Graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_premin", Protean::Graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_premin_blendout", Protean::Graphics::ShaderStage::CS},
	{"msaoCS", Protean::Graphics::ShaderStage::CS},
	{"motionblur_neighborhoodMaxVelocityCS", Protean::Graphics::ShaderStage::CS},
	{"motionblur_tileMaxVelocity_horizontalCS", Protean::Graphics::ShaderStage::CS},
	{"motionblur_tileMaxVelocity_verticalCS", Protean::Graphics::ShaderStage::CS},
	{"luminancePass2CS", Protean::Graphics::ShaderStage::CS},
	{"motionblurCS", Protean::Graphics::ShaderStage::CS},
	{"motionblurCS_cheap", Protean::Graphics::ShaderStage::CS},
	{"motionblurCS_earlyexit", Protean::Graphics::ShaderStage::CS},
	{"luminancePass1CS", Protean::Graphics::ShaderStage::CS},
	{"lightShaftsCS", Protean::Graphics::ShaderStage::CS},
	{"hbaoCS", Protean::Graphics::ShaderStage::CS},
	{"radix_sortCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_0, {{"FPS_COUNT"}}, "FPS_Count"},
	{"radix_sortCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_0, {{"FPS_COUNT_REDUCE"}}, "FPS_CountReduce"},
	{"radix_sortCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_0, {{"FPS_SCAN"}}, "FPS_Scan"},
	{"radix_sortCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_0, {{"FPS_SCAN_ADD"}}, "FPS_ScanAdd"},
	{"radix_sortCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_0, {{"FPS_SCATTER"}}, "FPS_Scatter"},
	{"radix_sortCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_0, {{"FPS_SCATTER", "kRS_ValueCopy"}}, "FPS_Scatter"},
	{"radix_sortCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_0, {{"FPS_INDIRECT"}}, "FPS_SetupIndirectParameters"},
	{"fxaaCS", Protean::Graphics::ShaderStage::CS},
	{"filterEnvMapCS", Protean::Graphics::ShaderStage::CS},
	{"fft_512x512_c2c_CS", Protean::Graphics::ShaderStage::CS},
	{"fft_512x512_c2c_v2_CS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_sphpartitionCS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_sphcellallocationCS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_sphbinningCS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_SORTING", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_sphdensityCS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_sphforceCS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_kickoffUpdateCS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_DEPTHCOLLISIONS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_emitCS", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_emitCS_FROMMESH", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_emitCS_volume", Protean::Graphics::ShaderStage::CS},
	{"emittedparticle_finishUpdateCS", Protean::Graphics::ShaderStage::CS},
	{"downsample4xCS", Protean::Graphics::ShaderStage::CS},
	{"lineardepthCS", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_prepassCS_earlyexit", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_mainCS_cheap", Protean::Graphics::ShaderStage::CS},
	{"depthoffield_mainCS_earlyexit", Protean::Graphics::ShaderStage::CS },
	{"depthoffield_postfilterCS", Protean::Graphics::ShaderStage::CS },
	{"copytexture2D_float4_borderexpandCS", Protean::Graphics::ShaderStage::CS },
	{"copytexture2D_float4CS", Protean::Graphics::ShaderStage::CS },
	{"chromatic_aberrationCS", Protean::Graphics::ShaderStage::CS },
	{"bvh_hierarchyCS", Protean::Graphics::ShaderStage::CS },
	{"bvh_primitivesCS", Protean::Graphics::ShaderStage::CS },
	{"bvh_propagateaabbCS", Protean::Graphics::ShaderStage::CS },
	{"blur_gaussian_wide_float1CS", Protean::Graphics::ShaderStage::CS },
	{"blur_gaussian_wide_float4CS", Protean::Graphics::ShaderStage::CS },
	{"blur_gaussian_float1CS", Protean::Graphics::ShaderStage::CS },
	{"blur_bilateral_wide_float1CS", Protean::Graphics::ShaderStage::CS },
	{"blur_bilateral_wide_float4CS", Protean::Graphics::ShaderStage::CS },
	{"blur_bilateral_float1CS", Protean::Graphics::ShaderStage::CS },
	{"blur_bilateral_float4CS", Protean::Graphics::ShaderStage::CS },
	{"normalsfromdepthCS", Protean::Graphics::ShaderStage::CS },
	{"volumetricCloud_curlnoiseCS", Protean::Graphics::ShaderStage::CS },
	{"volumetricCloud_detailnoiseCS", Protean::Graphics::ShaderStage::CS },
	{"volumetricCloud_renderCS", Protean::Graphics::ShaderStage::CS },
	{"volumetricCloud_renderCS_capture", Protean::Graphics::ShaderStage::CS },
	{"volumetricCloud_reprojectCS", Protean::Graphics::ShaderStage::CS },
	{"volumetricCloud_shadow_renderCS", Protean::Graphics::ShaderStage::CS },
	{"volumetricCloud_shapenoiseCS", Protean::Graphics::ShaderStage::CS },
	{"volumetricCloud_upsamplePS", Protean::Graphics::ShaderStage::PS },
	{"volumetricCloud_weathermapCS", Protean::Graphics::ShaderStage::CS },
	{"shadingRateClassificationCS", Protean::Graphics::ShaderStage::CS },
	{"shadingRateClassificationCS_DEBUG", Protean::Graphics::ShaderStage::CS },
	{"aerialPerspectiveCS", Protean::Graphics::ShaderStage::CS },
	{"skyAtmosphere_cameraVolumeLutCS", Protean::Graphics::ShaderStage::CS },
	{"skyAtmosphere_transmittanceLutCS", Protean::Graphics::ShaderStage::CS },
	{"skyAtmosphere_skyViewLutCS", Protean::Graphics::ShaderStage::CS },
	{"skyAtmosphere_multiScatteredLuminanceLutCS", Protean::Graphics::ShaderStage::CS },
	{"skyAtmosphere_skyLuminanceLutCS", Protean::Graphics::ShaderStage::CS },
	{"screenspaceshadowCS", Protean::Graphics::ShaderStage::CS },
	{"rtshadowCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_5 },
	{"rtshadow_denoise_tileclassificationCS", Protean::Graphics::ShaderStage::CS },
	{"rtshadow_denoise_filterCS", Protean::Graphics::ShaderStage::CS },
	{"rtshadow_denoise_temporalCS", Protean::Graphics::ShaderStage::CS },
	{"rtshadow_upsampleCS", Protean::Graphics::ShaderStage::CS },
	{"rtaoCS", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_5 },
	{"rtao_denoise_tileclassificationCS", Protean::Graphics::ShaderStage::CS },
	{"rtao_denoise_filterCS", Protean::Graphics::ShaderStage::CS },
	{"surfel_coverageCS", Protean::Graphics::ShaderStage::CS },
	{"surfel_indirectprepareCS", Protean::Graphics::ShaderStage::CS },
	{"surfel_updateCS", Protean::Graphics::ShaderStage::CS },
	{"surfel_gridoffsetsCS", Protean::Graphics::ShaderStage::CS },
	{"surfel_binningCS", Protean::Graphics::ShaderStage::CS },
	{"surfel_raytraceCS_rtapi", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_5 },
	{"surfel_raytraceCS", Protean::Graphics::ShaderStage::CS },
	{"surfel_integrateCS", Protean::Graphics::ShaderStage::CS },
	{"surfel_denoiseCS", Protean::Graphics::ShaderStage::CS },
	{"ddgi_rayallocationCS", Protean::Graphics::ShaderStage::CS },
	{"ddgi_indirectprepareCS", Protean::Graphics::ShaderStage::CS },
	{"ddgi_raytraceCS", Protean::Graphics::ShaderStage::CS },
	{"ddgi_raytraceCS_rtapi", Protean::Graphics::ShaderStage::CS, Protean::Graphics::ShaderModel::SM_6_5 },
	{"ddgi_updateCS", Protean::Graphics::ShaderStage::CS },
	{"ddgi_updateCS_depth", Protean::Graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS", Protean::Graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_normalmap", Protean::Graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_surfacemap", Protean::Graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_emissivemap", Protean::Graphics::ShaderStage::CS },
	{"meshlet_prepareCS", Protean::Graphics::ShaderStage::CS },
	{"impostor_prepareCS", Protean::Graphics::ShaderStage::CS },
	{"virtualTextureTileRequestsCS", Protean::Graphics::ShaderStage::CS },
	{"virtualTextureTileAllocateCS", Protean::Graphics::ShaderStage::CS },
	{"virtualTextureResidencyUpdateCS", Protean::Graphics::ShaderStage::CS },
	{"windCS", Protean::Graphics::ShaderStage::CS },
	{"yuv_to_rgbCS", Protean::Graphics::ShaderStage::CS },
	{"wetmap_updateCS", Protean::Graphics::ShaderStage::CS },
	{"depth_reprojectCS", Protean::Graphics::ShaderStage::CS },
	{"depth_pyramidCS", Protean::Graphics::ShaderStage::CS },
	{"lightmap_expandCS", Protean::Graphics::ShaderStage::CS },
	{"gaussian_splatCS", Protean::Graphics::ShaderStage::CS },
	{"gaussian_splat_indirectCS", Protean::Graphics::ShaderStage::CS },


	{"imagePS", Protean::Graphics::ShaderStage::PS },
	{"emittedparticlePS_soft", Protean::Graphics::ShaderStage::PS },
	{"emittedparticlePS_shadow", Protean::Graphics::ShaderStage::PS },
	{"emittedparticlePS_soft_lighting", Protean::Graphics::ShaderStage::PS },
	{"oceanSurfacePS", Protean::Graphics::ShaderStage::PS },
	{"oceanSurfacePS_envmap", Protean::Graphics::ShaderStage::PS },
	{"oceanSurfacePS_shadowmap", Protean::Graphics::ShaderStage::PS },
	{"hairparticlePS", Protean::Graphics::ShaderStage::PS },
	{"hairparticlePS_simple", Protean::Graphics::ShaderStage::PS },
	{"hairparticlePS_prepass", Protean::Graphics::ShaderStage::PS },
	{"hairparticlePS_prepass_depthonly", Protean::Graphics::ShaderStage::PS },
	{"hairparticlePS_shadow", Protean::Graphics::ShaderStage::PS },
	{"volumetricLight_SpotPS", Protean::Graphics::ShaderStage::PS },
	{"volumetricLight_PointPS", Protean::Graphics::ShaderStage::PS },
	{"volumetricLight_DirectionalPS", Protean::Graphics::ShaderStage::PS },
	{"volumetriclight_rectanglePS", Protean::Graphics::ShaderStage::PS },
	{"vertexcolorPS", Protean::Graphics::ShaderStage::PS },
	{"upsample_bilateralPS", Protean::Graphics::ShaderStage::PS },
	{"sunPS", Protean::Graphics::ShaderStage::PS },
	{"skyPS_dynamic", Protean::Graphics::ShaderStage::PS },
	{"skyPS_static", Protean::Graphics::ShaderStage::PS },
	{"shadowPS_transparent", Protean::Graphics::ShaderStage::PS },
	{"shadowPS_alphatest", Protean::Graphics::ShaderStage::PS },
	{"paintdecalPS", Protean::Graphics::ShaderStage::PS },
	{"renderlightmapPS", Protean::Graphics::ShaderStage::PS },
	{"renderlightmapPS_rtapi", Protean::Graphics::ShaderStage::PS, Protean::Graphics::ShaderModel::SM_6_5 },
	{"raytrace_debugbvhPS", Protean::Graphics::ShaderStage::PS },
	{"outlinePS", Protean::Graphics::ShaderStage::PS },
	{"oceanSurfaceSimplePS", Protean::Graphics::ShaderStage::PS },
	{"objectPS_voxelizer", Protean::Graphics::ShaderStage::PS },
	{"objectPS_hologram", Protean::Graphics::ShaderStage::PS },
	{"objectPS_paintradius", Protean::Graphics::ShaderStage::PS },
	{"objectPS_prepass", Protean::Graphics::ShaderStage::PS },
	{"objectPS_prepass_alphatest", Protean::Graphics::ShaderStage::PS },
	{"objectPS_prepass_depthonly_alphatest", Protean::Graphics::ShaderStage::PS },
	{"vRectLightPS", Protean::Graphics::ShaderStage::PS },
	{"lensFlarePS", Protean::Graphics::ShaderStage::PS },
	{"impostorPS", Protean::Graphics::ShaderStage::PS },
	{"impostorPS_simple", Protean::Graphics::ShaderStage::PS },
	{"impostorPS_prepass", Protean::Graphics::ShaderStage::PS },
	{"impostorPS_prepass_depthonly", Protean::Graphics::ShaderStage::PS },
	{"forceFieldVisualizerPS", Protean::Graphics::ShaderStage::PS },
	{"envMap_skyPS_static", Protean::Graphics::ShaderStage::PS },
	{"envMap_skyPS_dynamic", Protean::Graphics::ShaderStage::PS },
	{"envMapPS", Protean::Graphics::ShaderStage::PS },
	{"emittedparticlePS_soft_distortion", Protean::Graphics::ShaderStage::PS },
	{"downsampleDepthBuffer4xPS", Protean::Graphics::ShaderStage::PS },
	{"emittedparticlePS_simple", Protean::Graphics::ShaderStage::PS },
	{"cubeMapPS", Protean::Graphics::ShaderStage::PS },
	{"circlePS", Protean::Graphics::ShaderStage::PS },
	{"captureImpostorPS", Protean::Graphics::ShaderStage::PS },
	{"copyDepthPS", Protean::Graphics::ShaderStage::PS },
	{"copyStencilBitPS", Protean::Graphics::ShaderStage::PS },
	{"extractStencilBitPS", Protean::Graphics::ShaderStage::PS },
	{"trailPS", Protean::Graphics::ShaderStage::PS },
	{"waveeffectPS", Protean::Graphics::ShaderStage::PS },
	{"gaussian_splatPS", Protean::Graphics::ShaderStage::PS },
	{"voidPS", Protean::Graphics::ShaderStage::PS },


	{"hairparticleVS", Protean::Graphics::ShaderStage::VS },
	{"emittedparticleVS", Protean::Graphics::ShaderStage::VS },
	{"emittedparticleVS_shadow", Protean::Graphics::ShaderStage::VS },
	{"imageVS", Protean::Graphics::ShaderStage::VS },
	{"voxelVS", Protean::Graphics::ShaderStage::VS },
	{"vertexcolorVS", Protean::Graphics::ShaderStage::VS },
	{"volumetriclight_directionalVS", Protean::Graphics::ShaderStage::VS },
	{"volumetriclight_pointVS", Protean::Graphics::ShaderStage::VS },
	{"volumetriclight_spotVS", Protean::Graphics::ShaderStage::VS },
	{"volumetriclight_rectangleVS", Protean::Graphics::ShaderStage::VS },
	{"vSpotLightVS", Protean::Graphics::ShaderStage::VS },
	{"vPointLightVS", Protean::Graphics::ShaderStage::VS },
	{"vRectLightVS", Protean::Graphics::ShaderStage::VS },
	{"sphereVS", Protean::Graphics::ShaderStage::VS },
	{"skyVS", Protean::Graphics::ShaderStage::VS },
	{"postprocessVS", Protean::Graphics::ShaderStage::VS },
	{"paintdecalVS", Protean::Graphics::ShaderStage::VS },
	{"renderlightmapVS", Protean::Graphics::ShaderStage::VS },
	{"raytrace_screenVS", Protean::Graphics::ShaderStage::VS },
	{"oceanSurfaceVS", Protean::Graphics::ShaderStage::VS },
	{"objectVS_debug", Protean::Graphics::ShaderStage::VS },
	{"objectVS_voxelizer", Protean::Graphics::ShaderStage::VS },
	{"lensFlareVS", Protean::Graphics::ShaderStage::VS },
	{"impostorVS", Protean::Graphics::ShaderStage::VS },
	{"forceFieldPointVisualizerVS", Protean::Graphics::ShaderStage::VS },
	{"forceFieldPlaneVisualizerVS", Protean::Graphics::ShaderStage::VS },
	{"envMap_skyVS", Protean::Graphics::ShaderStage::VS },
	{"envMapVS", Protean::Graphics::ShaderStage::VS },
	{"occludeeVS", Protean::Graphics::ShaderStage::VS },
	{"ddgi_debugVS", Protean::Graphics::ShaderStage::VS },
	{"voxelGS", Protean::Graphics::ShaderStage::GS },
	{"objectGS_voxelizer", Protean::Graphics::ShaderStage::GS },
	{"objectVS_simple", Protean::Graphics::ShaderStage::VS },
	{"objectVS_common", Protean::Graphics::ShaderStage::VS },
	{"objectVS_common_tessellation", Protean::Graphics::ShaderStage::VS },
	{"objectVS_prepass", Protean::Graphics::ShaderStage::VS },
	{"objectVS_prepass_alphatest", Protean::Graphics::ShaderStage::VS },
	{"objectVS_prepass_tessellation", Protean::Graphics::ShaderStage::VS },
	{"objectVS_simple_tessellation", Protean::Graphics::ShaderStage::VS },
	{"shadowVS", Protean::Graphics::ShaderStage::VS },
	{"shadowVS_alphatest", Protean::Graphics::ShaderStage::VS },
	{"shadowVS_transparent", Protean::Graphics::ShaderStage::VS },
	{"screenVS", Protean::Graphics::ShaderStage::VS },
	{"voxelgridVS", Protean::Graphics::ShaderStage::VS },
	{"trailVS", Protean::Graphics::ShaderStage::VS },
	{"gaussian_splatVS", Protean::Graphics::ShaderStage::VS },



	{"objectDS", Protean::Graphics::ShaderStage::DS },
	{"objectDS_prepass", Protean::Graphics::ShaderStage::DS },
	{"objectDS_simple", Protean::Graphics::ShaderStage::DS },


	{"objectHS", Protean::Graphics::ShaderStage::HS },
	{"objectHS_prepass", Protean::Graphics::ShaderStage::HS },
	{"objectHS_simple", Protean::Graphics::ShaderStage::HS },

	{"emittedparticleMS", Protean::Graphics::ShaderStage::MS },

	{"objectMS", Protean::Graphics::ShaderStage::MS },
	{"objectMS_prepass", Protean::Graphics::ShaderStage::MS },
	{"objectMS_prepass_alphatest", Protean::Graphics::ShaderStage::MS },
	{"objectMS_simple", Protean::Graphics::ShaderStage::MS },
	{"shadowMS", Protean::Graphics::ShaderStage::MS },
	{"shadowMS_alphatest", Protean::Graphics::ShaderStage::MS },
	{"shadowMS_transparent", Protean::Graphics::ShaderStage::MS },

	{"objectAS", Protean::Graphics::ShaderStage::AS },


	//{"rtreflectionLIB", Protean::Graphics::ShaderStage::LIB },
};

struct Target
{
	Protean::Graphics::ShaderFormat format;
	std::string dir;
};
Protean::vector<Target> targets;
Protean::unordered_map<std::string, Protean::ShaderCompiler::CompilerOutput> results;
bool rebuild = false;
bool shaderdump_enabled = false;

using namespace Protean::Graphics;

std::ostream nullout(&nullbuf);

int main(int argc, char* argv[])
{
	Protean::ShaderCompiler::Flags compile_flags = Protean::ShaderCompiler::Flags::NONE;

	Protean::Arguments::Parse(argc, argv);
	std::ostream* out;

	if (Protean::Arguments::HasArgument("quiet"))
	{
		Protean::Backlog::SetLogLevel(Protean::Backlog::LogLevel::Error);
		out = &nullout;
	}
	else
	{
		out = &std::cout;
	}

	*out << "[Wicked Engine Offline Shader Compiler]\n";
	*out << "Available command arguments:\n";
	*out << "\thlsl5 : \t\tCompile shaders to hlsl5 (dx11) format (using d3dcompiler)\n";
	*out << "\thlsl6 : \t\tCompile shaders to hlsl6 (dx12) format (using dxcompiler)\n";
	*out << "\tspirv : \t\tCompile shaders to spirv (vulkan) format (using dxcompiler)\n";
	*out << "\tmetal : \t\tCompile shaders to Apple Metal format (using dxcompiler and metal shader converter)\n";
	*out << "\thlsl6_xs : \t\tCompile shaders to hlsl6 Xbox Series native (dx12) format (requires Xbox SDK)\n";
	*out << "\tps5 : \t\t\tCompile shaders to PlayStation 5 native format (requires PlayStation 5 SDK)\n";
	*out << "\trebuild : \t\tAll shaders will be rebuilt, regardless if they are outdated or not\n";
	*out << "\tdisable_optimization : \tShaders will be compiled without optimizations\n";
	*out << "\tstrip_reflection : \tReflection will be stripped from shader binary to reduce file size\n";
	*out << "\tshaderdump : \t\tShaders will be saved to wiShaderDump.h C++ header file (can be combined with \"rebuild\")\n";
	*out << "\tdebuginfo : \t\tKeep symbol data for shader debugging\n";
	*out << "\tquiet : \t\tOnly print errors\n";
	*out << "\tsm6.1 : \t\tIncrease all compilations to shader model 6.1\n";
	*out << "\tsm6.2 : \t\tIncrease all compilations to shader model 6.2\n";
	*out << "\tsm6.3 : \t\tIncrease all compilations to shader model 6.3\n";
	*out << "\tsm6.4 : \t\tIncrease all compilations to shader model 6.4\n";
	*out << "\tsm6.5 : \t\tIncrease all compilations to shader model 6.5\n";
	*out << "\tsm6.6 : \t\tIncrease all compilations to shader model 6.6\n";
	*out << "Command arguments used: ";

	if (Protean::Arguments::HasArgument("hlsl5"))
	{
		targets.push_back({ ShaderFormat::HLSL5, "shaders/hlsl5/" });
		*out << "hlsl5 ";
	}
	if (Protean::Arguments::HasArgument("hlsl6"))
	{
		targets.push_back({ ShaderFormat::HLSL6, "shaders/hlsl6/" });
		*out << "hlsl6 ";
	}
	if (Protean::Arguments::HasArgument("spirv"))
	{
		targets.push_back({ ShaderFormat::SPIRV, "shaders/spirv/" });
		*out << "spirv ";
	}
	if (Protean::Arguments::HasArgument("metal"))
	{
		targets.push_back({ ShaderFormat::METAL, "shaders/metal/" });
		*out << "metal ";
	}
	if (Protean::Arguments::HasArgument("hlsl6_xs"))
	{
		targets.push_back({ ShaderFormat::HLSL6_XS, "shaders/hlsl6_xs/" });
		*out << "hlsl6_xs ";
	}
	if (Protean::Arguments::HasArgument("ps5"))
	{
		targets.push_back({ ShaderFormat::PS5, "shaders/ps5/" });
		*out << "ps5 ";
	}

	if (Protean::Arguments::HasArgument("shaderdump"))
	{
		shaderdump_enabled = true;
		*out << "shaderdump ";
	}

	if (Protean::Arguments::HasArgument("rebuild"))
	{
		rebuild = true;
		*out << "rebuild ";
	}

	if (Protean::Arguments::HasArgument("disable_optimization"))
	{
		compile_flags |= Protean::ShaderCompiler::Flags::DISABLE_OPTIMIZATION;
		*out << "disable_optimization ";
	}

	if (Protean::Arguments::HasArgument("debuginfo"))
	{
		compile_flags |= Protean::ShaderCompiler::Flags::KEEP_DEBUG_INFORMATION;
		*out << "debuginfo ";
	}

	if (Protean::Arguments::HasArgument("strip_reflection"))
	{
		compile_flags |= Protean::ShaderCompiler::Flags::STRIP_REFLECTION;
		*out << "strip_reflection ";
	}

	ShaderModel minshadermodel_override = ShaderModel::SM_5_0;
	if (Protean::Arguments::HasArgument("sm6.1"))
	{
		minshadermodel_override = ShaderModel::SM_6_1;
		*out << "sm6.1 ";
	}
	if (Protean::Arguments::HasArgument("sm6.2"))
	{
		minshadermodel_override = ShaderModel::SM_6_2;
		*out << "sm6.2 ";
	}
	if (Protean::Arguments::HasArgument("sm6.3"))
	{
		minshadermodel_override = ShaderModel::SM_6_3;
		*out << "sm6.3 ";
	}
	if (Protean::Arguments::HasArgument("sm6.4"))
	{
		minshadermodel_override = ShaderModel::SM_6_4;
		*out << "sm6.4 ";
	}
	if (Protean::Arguments::HasArgument("sm6.5"))
	{
		minshadermodel_override = ShaderModel::SM_6_5;
		*out << "sm6.5 ";
	}
	if (Protean::Arguments::HasArgument("sm6.6"))
	{
		minshadermodel_override = ShaderModel::SM_6_6;
		*out << "sm6.6 ";
	}
	if (Protean::Arguments::HasArgument("sm6.7"))
	{
		minshadermodel_override = ShaderModel::SM_6_7;
		*out << "sm6.7 ";
	}

	*out << "\n";

	if (targets.empty())
	{
		targets = {
#ifdef __APPLE__
			{ ShaderFormat::METAL, "shaders/metal/" },
#else
			//{ ShaderFormat::HLSL5, "shaders/hlsl5/" },
			{ ShaderFormat::HLSL6, "shaders/hlsl6/" },
			{ ShaderFormat::SPIRV, "shaders/spirv/" },
#endif // __APPLE__
		};
		*out << "No shader formats were specified, assuming command arguments: spirv hlsl6\n";
	}

	// permutations for objectPS:
	shaders.push_back({ "objectPS", Protean::Graphics::ShaderStage::PS });
	for (auto& x : Protean::MaterialComponent::shaderTypeDefines)
	{
		shaders.back().permutations.emplace_back().defines = x;

		// same but with TRANSPARENT:
		shaders.back().permutations.emplace_back().defines = x;
		shaders.back().permutations.back().defines.push_back("TRANSPARENT");
	}

	// permutations for visibility_analyzeCS:
	shaders.push_back({ "visibility_analyzeCS", Protean::Graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back();
	shaders.back().permutations.emplace_back().defines = { "PRIMITIVEID_MSAA" };

	// permutations for visibility_resolveCS:
	shaders.push_back({ "visibility_resolveCS", Protean::Graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back().defines = { "PRIMITIVEID_UNIFORM" };
	shaders.back().permutations.emplace_back().defines = { "PRIMITIVEID_DIVERGENT" };
	shaders.back().permutations.emplace_back().defines = { "PRIMITIVEID_UNIFORM", "MATERIAL_BINNING" };
	shaders.back().permutations.emplace_back().defines = { "PRIMITIVEID_DIVERGENT", "MATERIAL_BINNING" };

	// permutations for visibility_velocityCS:
	shaders.push_back({ "visibility_velocityCS", Protean::Graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back().defines = { "PRIMITIVEID_UNIFORM" };
	shaders.back().permutations.emplace_back().defines = { "PRIMITIVEID_DIVERGENT" };

	// permutations for visibility_surfaceCS:
	shaders.push_back({ "visibility_surfaceCS", Protean::Graphics::ShaderStage::CS });
	for (auto& x : Protean::MaterialComponent::shaderTypeDefines)
	{
		shaders.back().permutations.emplace_back().defines = x;
		shaders.back().permutations.back().defines.push_back("PRIMITIVEID_UNIFORM");
	}
	for (auto& x : Protean::MaterialComponent::shaderTypeDefines)
	{
		shaders.back().permutations.emplace_back().defines = x;
		shaders.back().permutations.back().defines.push_back("PRIMITIVEID_DIVERGENT");
	}

	// permutations for visibility_shadeCS:
	shaders.push_back({ "visibility_shadeCS", Protean::Graphics::ShaderStage::CS });
	for (auto& x : Protean::MaterialComponent::shaderTypeDefines)
	{
		shaders.back().permutations.emplace_back().defines = x;
		shaders.back().permutations.back().defines.push_back("PRIMITIVEID_UNIFORM");
	}
	for (auto& x : Protean::MaterialComponent::shaderTypeDefines)
	{
		shaders.back().permutations.emplace_back().defines = x;
		shaders.back().permutations.back().defines.push_back("PRIMITIVEID_DIVERGENT");
	}

	// permutations for ssgiCS:
	shaders.push_back({ "ssgiCS", Protean::Graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back().defines = { "WIDE" };
	// permutations for ssgi_upsampleCS:
	shaders.push_back({ "ssgi_upsampleCS", Protean::Graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back().defines = { "WIDE" };

	// permutations for copyStencilBitPS:
	shaders.push_back({ "copyStencilBitPS", Protean::Graphics::ShaderStage::PS });
	shaders.back().permutations.emplace_back().defines = { "MSAA" };

	// permutations for yuv_to_rgbCS:
	shaders.push_back({ "yuv_to_rgbCS", Protean::Graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back().defines = { "ARRAY" };

	// permutations for lightCullingCS:
	shaders.push_back({ "lightCullingCS", Protean::Graphics::ShaderStage::CS });
	shaders.back().permutations.emplace_back().defines = {};
	shaders.back().permutations.emplace_back().defines = { "DEBUG" };
	shaders.back().permutations.emplace_back().defines = { "ADVANCED" };
	shaders.back().permutations.emplace_back().defines = { "ADVANCED", "DEBUG" };

	// Simplify permutation iteration:
	for (auto& shader : shaders)
	{
		if (shader.permutations.empty())
		{
			shader.permutations.emplace_back();
		}
	}

	Protean::JobSystem::Initialize();
	Protean::JobSystem::context ctx;

	static std::string SHADERSOURCEPATH = Protean::Renderer::GetShaderSourcePath();
	Protean::Helper::MakePathAbsolute(SHADERSOURCEPATH);

	*out << "[Wicked Engine Offline Shader Compiler] Searching for outdated shaders...\n";
	Protean::Timer timer;
	static int errors = 0;

	for (auto& target : targets)
	{
		std::string SHADERPATH = target.dir;
		Protean::Helper::DirectoryCreate(SHADERPATH);

		for (auto& shader : shaders)
		{
			if (target.format == ShaderFormat::HLSL5)
			{
				if (
					shader.stage == ShaderStage::MS ||
					shader.stage == ShaderStage::AS ||
					shader.stage == ShaderStage::LIB
					)
				{
					// shader stage not applicable to HLSL5
					continue;
				}
			}

			for (auto& permutation : shader.permutations)
			{
				Protean::JobSystem::Execute(ctx, [&target, &permutation, &shader, &out, SHADERPATH, compile_flags, minshadermodel_override](Protean::JobSystem::JobArgs args) {

					std::string shaderbinaryfilename = SHADERPATH + shader.name;
					for (auto& def : permutation.defines)
					{
						shaderbinaryfilename += "_" + def;
					}
					shaderbinaryfilename += ".cso";

					Protean::ShaderCompiler::CompilerOutput output;

					if (!Protean::ShaderCompiler::IsShaderOutdated(shaderbinaryfilename))
					{
						if (!rebuild)
						{
							if (shaderdump_enabled)
							{
								auto vec = Protean::Allocator::make_shared<std::vector<uint8_t>>();

								if (Protean::Helper::FileRead(shaderbinaryfilename, *vec))
								{
									output.internal_state = vec;
									output.shaderdata = vec->data();
									output.shadersize = vec->size();
									locker.lock();
									results[shaderbinaryfilename] = output;
									*out << "up-to-date: " << shaderbinaryfilename << std::endl;
									locker.unlock();
								}
								else
								{
									locker.lock();
									std::cerr << "ERROR reading binary shader: " << shaderbinaryfilename << std::endl;
									locker.unlock();
								}
							}
							return;
						}
					}

					Protean::ShaderCompiler::CompilerInput input;
					input.entrypoint = shader.entrypoint;
					input.flags = compile_flags;
					input.format = target.format;
					input.stage = shader.stage;
					input.shadersourcefilename = SHADERSOURCEPATH + shader.name + ".hlsl";
					input.include_directories.push_back(SHADERSOURCEPATH);
					input.include_directories.push_back(SHADERSOURCEPATH + Protean::Helper::GetDirectoryFromPath(shader.name));
					input.minshadermodel = std::max(shader.minshadermodel, minshadermodel_override);
					input.defines = permutation.defines;

					if (input.minshadermodel > ShaderModel::SM_5_0 && target.format == ShaderFormat::HLSL5)
					{
						// if shader format cannot support shader model, then we cancel the task without returning error
						return;
					}
					if (target.format == ShaderFormat::PS5 && (input.minshadermodel >= ShaderModel::SM_6_5 || input.stage == ShaderStage::MS || input.stage == ShaderStage::AS))
					{
						// TODO PS5 raytracing, mesh shader
						return;
					}
					if (target.format == ShaderFormat::HLSL6_XS && (input.stage == ShaderStage::MS || input.stage == ShaderStage::AS))
					{
						// TODO Xbox mesh shader
						return;
					}

					Protean::ShaderCompiler::Compile(input, output);

					if (output.IsValid())
					{
						Protean::ShaderCompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

						locker.lock();
						if (!output.error_message.empty())
						{
							std::cerr << output.error_message << "\n";
						}
						*out << "shader compiled: " << shaderbinaryfilename << "\n";
						if (shaderdump_enabled)
						{
							results[shaderbinaryfilename] = output;
						}
						locker.unlock();
					}
					else
					{
						locker.lock();
						std::cerr << "shader compile FAILED: " << shaderbinaryfilename << "\n" << output.error_message;
						errors++;
						locker.unlock();
					}

				});
			}
		}
	}
	Protean::JobSystem::Wait(ctx);

	*out << "[Wicked Engine Offline Shader Compiler] Finished in " << std::setprecision(4) << timer.elapsed_seconds() << " seconds with " << errors << " errors\n";

	if (shaderdump_enabled)
	{
		*out << "[Wicked Engine Offline Shader Compiler] Creating ShaderDump...\n";
		timer.record();
		size_t total_raw = 0;
		size_t total_compressed = 0;
		std::string ss;
		ss += "namespace wiShaderDump {\n";
		for (auto& x : results)
		{
			auto& name = x.first;
			auto& output = x.second;

			Protean::vector<uint8_t> compressed;
			bool success = Protean::Helper::Compress(output.shaderdata, output.shadersize, compressed, 9);
			if (success) {
				total_raw += output.shadersize;
				total_compressed += compressed.size();
			}
			else
			{
				Protean::Helper::DebugOut("Compression failed while creating shader dump!", Protean::Helper::DebugLevel::Error);
				continue;
			}

			std::string name_repl = name;
			std::replace(name_repl.begin(), name_repl.end(), '/', '_');
			std::replace(name_repl.begin(), name_repl.end(), '.', '_');
			std::replace(name_repl.begin(), name_repl.end(), '-', '_');
			ss += "static const uint8_t " + name_repl + "[] = {";
			for (size_t i = 0; i < compressed.size(); ++i)
			{
				ss += std::to_string((uint32_t)compressed[i]) + ",";
			}
			ss += "};\n";
		}
		*out << "[Wicked Engine Offline Shader Compiler] Compressed shaders: " << total_raw << " -> " << total_compressed << " (" << std::setprecision(3) << (100. * total_compressed / total_raw) << "%)" << std::endl;
		ss += "struct ShaderDumpEntry{const uint8_t* data; size_t size;};\n";
		ss += "static const Protean::unordered_map<std::string, ShaderDumpEntry> shaderdump = {\n";
		for (auto& x : results)
		{
			auto& name = x.first;

			std::string name_repl = name;
			std::replace(name_repl.begin(), name_repl.end(), '/', '_');
			std::replace(name_repl.begin(), name_repl.end(), '.', '_');
			std::replace(name_repl.begin(), name_repl.end(), '-', '_');
			ss += "{\"" + name + "\", {" + name_repl + ",sizeof(" + name_repl + ")}},\n";
		}
		ss += "};\n"; // map end
		ss += "}\n"; // namespace end
		Protean::Helper::FileWrite("wiShaderDump.h", (uint8_t*)ss.c_str(), ss.length());
		*out << "[Wicked Engine Offline Shader Compiler] ShaderDump written to wiShaderDump.h in " << std::setprecision(4) << timer.elapsed_seconds() << " seconds\n";
	}

	Protean::JobSystem::ShutDown();

	return errors;
}
