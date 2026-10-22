#pragma once
#include "render/RenderPath2D.h"
#include "render/Renderer.h"
#include "render/GraphicsAPI/GraphicsDevice.h"
#include "resource/ResourceManager.h"
#include "scene/Scene.h"
#include "core/UnorderedMap.h"

namespace Protean
{

	class RenderPath3D : public RenderPath2D
	{
	public:
		enum AO
		{
			AO_DISABLED,	// no ambient occlusion
			AO_SSAO,		// simple brute force screen space ambient occlusion
			AO_HBAO,		// horizon based screen space ambient occlusion
			AO_MSAO,		// multi scale screen space ambient occlusion
			AO_RTAO,		// ray traced ambient occlusion
			// Don't alter order! (bound to lua manually)
		};
		enum class FSR2_Preset
		{
			// Guidelines: https://github.com/GPUOpen-Effects/FidelityFX-FSR2#scaling-modes
			Quality,
			Balanced,
			Performance,
			Ultra_Performance,
		};
	private:
		float exposure = 1.0f;
		float brightness = 0.0f;
		float contrast = 1.0f;
		float saturation = 1.0f;
		float bloomThreshold = 1.0f;
		float motionBlurStrength = 100.0f;
		float dofStrength = 10.0f;
		float sharpenFilterAmount = 0.28f;
		float outlineThreshold = 0.2f;
		float outlineThickness = 1.0f;
		XMFLOAT4 outlineColor = XMFLOAT4(0, 0, 0, 1);
		float aoRange = 1.0f;
		uint32_t aoSampleCount = 16;
		float aoPower = 1.0f;
		float chromaticAberrationAmount = 2.0f;
		uint32_t screenSpaceShadowSampleCount = 16;
		float screenSpaceShadowRange = 1;
		float eyeadaptionKey = 0.115f;
		float eyeadaptionRate = 1;
		float fsrSharpness = 1.0f;
		float fsr2Sharpness = 0.5f;
		float lightShaftsStrength = 0.5f;
		float lightShaftsFadeSpeed = 3.0f;
		float raytracedDiffuseRange = 10;
		float raytracedReflectionsRange = 10000.0f;
		float reflectionRoughnessCutoff = 0.6f;
		float ssgiDepthRejection = 8;
		Protean::Renderer::Tonemap tonemap = Protean::Renderer::Tonemap::ACES;
		float hdr_calibration = 1;
		Protean::Renderer::PostProcessQuality raytracedDiffuseQuality = Protean::Renderer::PostProcessQuality::Medium;
		Protean::Renderer::PostProcessQuality raytracedReflectionsQuality = Protean::Renderer::PostProcessQuality::Medium;
		Protean::Renderer::PostProcessQuality ssrQuality = Protean::Renderer::PostProcessQuality::Medium;
		uint32_t planarReflectionMSAASampleCount = 4;
		float planarReflectionResolutionScale = 0.25f;

		AO ao = AO_DISABLED;
		bool fxaaEnabled = false;
		bool ssrEnabled = false;
		bool ssgiEnabled = false;
		bool raytracedReflectionsEnabled = false;
		bool raytracedDiffuseEnabled = false;
		bool reflectionsEnabled = true;
		bool shadowsEnabled = true;
		bool bloomEnabled = true;
		bool colorGradingEnabled = true;
		bool volumeLightsEnabled = true;
		bool lightShaftsEnabled = false;
		bool lensFlareEnabled = true;
		bool motionBlurEnabled = false;
		bool depthOfFieldEnabled = true;
		bool eyeAdaptionEnabled = false;
		bool sharpenFilterEnabled = false;
		bool outlineEnabled = false;
		bool chromaticAberrationEnabled = false;
		bool ditherEnabled = true;
		bool occlusionCullingEnabled = true;
		bool sceneUpdateEnabled = true;
		bool fsrEnabled = false;
		bool fsr2Enabled = false;
		bool mesh_blend = true;
		bool crtFilterEnabled = false;

		mutable bool first_frame = true;
		mutable bool prerender_happened = false;

		void RenderCameraComponents(Protean::JobSystem::context& ctx) const;

	public:
		Protean::Graphics::Texture rtMain;
		Protean::Graphics::Texture rtMain_render; // can be MSAA
		Protean::Graphics::Texture rtPrimitiveID;
		Protean::Graphics::Texture rtPrimitiveID_render; // can be MSAA
		Protean::Graphics::Texture rtVelocity; // optional R16G16_FLOAT
		Protean::Graphics::Texture rtReflection; // contains the scene rendered for planar reflections
		Protean::Graphics::Texture rtReflection_render; // contains the scene rendered for planar reflections, can be MSAA
		Protean::Graphics::Texture rtRaytracedDiffuse; // raytraced diffuse screen space texture
		Protean::Graphics::Texture rtSSR; // standard screen-space reflection results
		Protean::Graphics::Texture rtSSGI; // standard screen-space GI results
		Protean::Graphics::Texture rtSceneCopy; // contains the rendered scene that can be fed into transparent pass for distortion effect
		Protean::Graphics::Texture rtSceneCopy_tmp; // temporary for gaussian mipchain
		Protean::Graphics::Texture rtWaterRipple; // water ripple sprite normal maps are rendered into this
		Protean::Graphics::Texture rtParticleDistortion_render; // contains distortive particles (can be MSAA)
		Protean::Graphics::Texture rtParticleDistortion; // contains distortive particles
		Protean::Graphics::Texture rtVolumetricLights; // contains the volumetric light results
		Protean::Graphics::Texture rtBloom; // contains the bright parts of the image + mipchain
		Protean::Graphics::Texture rtBloom_tmp; // temporary for bloom downsampling
		Protean::Graphics::Texture rtAO; // full res AO
		Protean::Graphics::Texture rtShadow; // raytraced shadows mask
		Protean::Graphics::Texture rtSun[3]; // 0: sun render target used for lightshafts (can be MSAA), 1: radial blurred lightshafts
		Protean::Graphics::Texture rtSun_resolved; // sun render target, but the resolved version if MSAA is enabled
		Protean::Graphics::Texture rtGUIBlurredBackground[3];	// downsampled, gaussian blurred scene for GUI
		Protean::Graphics::Texture rtShadingRate; // UINT8 shading rate per tile
		Protean::Graphics::Texture rtFSR[2]; // FSR upscaling result (full resolution LDR)
		Protean::Graphics::Texture rtOutlineSource; // linear depth but only the regions which have outline stencil
		Protean::Graphics::Texture rtOutlineSource_MSAA; // linear depth but only the regions which have outline stencil
		Protean::Graphics::Texture rtPostprocess; // ping-pong with main scene RT in post-process chain

		Protean::Graphics::Texture depthBuffer_Main; // used for depth-testing, can be MSAA
		Protean::Graphics::Texture depthBuffer_Copy; // used for shader resource, single sample
		Protean::Graphics::Texture depthBuffer_Copy1; // used for disocclusion check
		Protean::Graphics::Texture depthBuffer_Reflection; // used for reflection
		Protean::Graphics::Texture depthBuffer_Reflection_render; // used for reflection, can be MSAA
		Protean::Graphics::Texture reprojectedDepth; // prev frame depth reprojected into current, and downsampled for meshlet occlusion culling

		Protean::Graphics::GPUBuffer aliasingAllocation; // aliasing memory allocation for textures
		uint64_t offset_rtPostprocess = 0;
		uint64_t offset_rtPrimitiveID = 0;
		uint64_t offset_rtSceneCopy = 0;
		uint64_t offset_rtParticleDistortion = 0;
		uint64_t offset_rtAO = 0;
		uint64_t offset_rtWaterRipple = 0;

		Protean::Graphics::Texture debugUAV; // debug UAV can be used by some shaders...
		Protean::Renderer::TiledLightResources tiledLightResources;
		Protean::Renderer::TiledLightResources tiledLightResources_planarReflection;
		Protean::Renderer::LuminanceResources luminanceResources;
		Protean::Renderer::SSAOResources ssaoResources;
		Protean::Renderer::MSAOResources msaoResources;
		Protean::Renderer::RTAOResources rtaoResources;
		Protean::Renderer::RTDiffuseResources rtdiffuseResources;
		Protean::Renderer::RTReflectionResources rtreflectionResources;
		Protean::Renderer::SSRResources ssrResources;
		Protean::Renderer::SSGIResources ssgiResources;
		Protean::Renderer::RTShadowResources rtshadowResources;
		Protean::Renderer::ScreenSpaceShadowResources screenspaceshadowResources;
		Protean::Renderer::DepthOfFieldResources depthoffieldResources;
		Protean::Renderer::MotionBlurResources motionblurResources;
		Protean::Renderer::AerialPerspectiveResources aerialperspectiveResources;
		Protean::Renderer::AerialPerspectiveResources aerialperspectiveResources_reflection;
		Protean::Renderer::VolumetricCloudResources volumetriccloudResources;
		Protean::Renderer::BloomResources bloomResources;
		Protean::Renderer::SurfelGIResources surfelGIResources;
		Protean::Renderer::TemporalAAResources temporalAAResources;
		Protean::Renderer::VisibilityResources visibilityResources;
		Protean::Renderer::FSR2Resources fsr2Resources;
		Protean::Renderer::VXGIResources vxgiResources;
		Protean::Renderer::MeshBlendResources meshblendResources;

		Protean::Graphics::CommandList video_cmd;

		Protean::Graphics::Texture distortion_overlay; // optional full screen distortion from an asset

		mutable const Protean::Graphics::Texture* lastPostprocessRT = &rtPostprocess;
		// Post-processes are ping-ponged, this function helps to obtain the last postprocess render target that was written
		const Protean::Graphics::Texture* GetLastPostprocessRT() const
		{
			return lastPostprocessRT;
		}

		mutable float lightShaftsFadeFactor = 0.0f;

		virtual void RenderAO(Protean::Graphics::CommandList cmd) const;
		virtual void RenderSSR(Protean::Graphics::CommandList cmd) const;
		virtual void RenderSSGI(Protean::Graphics::CommandList cmd) const;
		virtual void RenderOutline(Protean::Graphics::CommandList cmd) const;
		virtual void RenderLightShafts(Protean::Graphics::CommandList cmd) const;
		virtual void RenderVolumetrics(Protean::Graphics::CommandList cmd) const;
		virtual void RenderSceneMIPChain(Protean::Graphics::CommandList cmd) const;
		virtual void RenderTransparents(Protean::Graphics::CommandList cmd) const;
		virtual void RenderPostprocessChain(Protean::Graphics::CommandList cmd) const;

		void DeleteGPUResources() override;
		void ResizeBuffers() override;

		Protean::CameraComponent* camera = &Protean::GetCamera();
		Protean::CameraComponent camera_previous;
		Protean::CameraComponent camera_reflection;
		Protean::CameraComponent camera_reflection_previous;

		Protean::Scene* scene = &Protean::GetScene();
		Protean::Renderer::Visibility visibility_main;
		Protean::Renderer::Visibility visibility_reflection;

		FrameCB frameCB = {};

		bool visibility_shading_in_compute = false;

		// Crop parameters in logical coordinates:
		float crop_left = 0;
		float crop_top = 0;
		float crop_right = 0;
		float crop_bottom = 0;
		Protean::Graphics::Rect GetScissorNativeResolution() const
		{
			Protean::Graphics::Rect scissor;
			scissor.left = int(LogicalToPhysical(crop_left));
			scissor.top = int(LogicalToPhysical(crop_top));
			scissor.right = int(GetPhysicalWidth() - LogicalToPhysical(crop_right));
			scissor.bottom = int(GetPhysicalHeight() - LogicalToPhysical(crop_bottom));
			return scissor;
		}
		Protean::Graphics::Rect GetScissorInternalResolution() const
		{
			Protean::Graphics::Rect scissor;
			scissor.left = int(LogicalToPhysical(crop_left) * resolutionScale);
			scissor.top = int(LogicalToPhysical(crop_top) * resolutionScale);
			scissor.right = int(GetInternalResolution().x - LogicalToPhysical(crop_right) * resolutionScale);
			scissor.bottom = int(GetInternalResolution().y - LogicalToPhysical(crop_bottom) * resolutionScale);
			return scissor;
		}

		// The 3D scene rendering without 2D overlay that is used in Compose() to combine the final image
		const Protean::Graphics::Texture& GetRenderResult3D() const { return *GetLastPostprocessRT(); }

		// The depth stencil of the 3D rendering (not texture compatible, only depth-stencil)
		const Protean::Graphics::Texture* GetDepthStencil() const override { return &depthBuffer_Main; }

		// The 3D render result's blurred version that can be used for GUI background blur
		const Protean::Graphics::Texture* GetGUIBlurredBackground() const override { return &rtGUIBlurredBackground[2]; }

		constexpr float getExposure() const { return exposure; }
		constexpr float getHDRCalibration() const { return hdr_calibration; }
		constexpr float getBrightness() const { return brightness; }
		constexpr float getContrast() const { return contrast; }
		constexpr float getSaturation() const { return saturation; }
		constexpr float getBloomThreshold() const { return bloomThreshold; }
		constexpr float getMotionBlurStrength() const { return motionBlurStrength; }
		constexpr float getDepthOfFieldStrength() const { return dofStrength; }
		constexpr float getSharpenFilterAmount() const { return sharpenFilterAmount; }
		constexpr float getOutlineThreshold() const { return outlineThreshold; }
		constexpr float getOutlineThickness() const { return outlineThickness; }
		constexpr XMFLOAT4 getOutlineColor() const { return outlineColor; }
		constexpr float getAORange() const { return aoRange; }
		constexpr uint32_t getAOSampleCount() const { return aoSampleCount; }
		constexpr float getAOPower() const { return aoPower; }
		constexpr float getChromaticAberrationAmount() const { return chromaticAberrationAmount; }
		constexpr uint32_t getScreenSpaceShadowSampleCount() const { return screenSpaceShadowSampleCount; }
		constexpr float getScreenSpaceShadowRange() const { return screenSpaceShadowRange; }
		constexpr float getEyeAdaptionKey() const { return eyeadaptionKey; }
		constexpr float getEyeAdaptionRate() const { return eyeadaptionRate; }
		constexpr float getFSRSharpness() const { return fsrSharpness; }
		constexpr float getFSR2Sharpness() const { return fsr2Sharpness; }
		constexpr float getLightShaftsStrength() const { return lightShaftsStrength; }
		constexpr float getLightShaftsFadeSpeed() const { return lightShaftsFadeSpeed; }
		constexpr float getRaytracedDiffuseRange() const { return raytracedDiffuseRange; }
		constexpr float getRaytracedReflectionsRange() const { return raytracedReflectionsRange; }
		constexpr float getReflectionRoughnessCutoff() const { return reflectionRoughnessCutoff; }
		constexpr float getSSGIDepthRejection() const { return ssgiDepthRejection; }
		constexpr Protean::Renderer::Tonemap getTonemap() const { return tonemap; }
		constexpr Protean::Renderer::PostProcessQuality getRaytracedDiffuseQuality() const { return raytracedDiffuseQuality; }
		constexpr Protean::Renderer::PostProcessQuality getRaytracedReflectionsQuality() const { return raytracedReflectionsQuality; }
		constexpr Protean::Renderer::PostProcessQuality getSSRQuality() const { return ssrQuality; }
		constexpr uint32_t getPlanarReflectionMSAASampleCount() const { return planarReflectionMSAASampleCount; }
		constexpr float getPlanarReflectionResolutionScale() const { return planarReflectionResolutionScale; }

		constexpr bool getAOEnabled() const { return ao != AO_DISABLED; }
		constexpr AO getAO() const { return ao; }
		constexpr bool getSSREnabled() const { return ssrEnabled; }
		constexpr bool getSSGIEnabled() const { return ssgiEnabled; }
		constexpr bool getRaytracedDiffuseEnabled() const { return raytracedDiffuseEnabled; }
		constexpr bool getRaytracedReflectionEnabled() const { return raytracedReflectionsEnabled; }
		constexpr bool getShadowsEnabled() const { return shadowsEnabled; }
		constexpr bool getReflectionsEnabled() const { return reflectionsEnabled; }
		constexpr bool getFXAAEnabled() const { return fxaaEnabled; }
		constexpr bool getBloomEnabled() const { return bloomEnabled; }
		constexpr bool getColorGradingEnabled() const { return colorGradingEnabled; }
		constexpr bool getVolumeLightsEnabled() const { return volumeLightsEnabled; }
		constexpr bool getLightShaftsEnabled() const { return lightShaftsEnabled; }
		constexpr bool getLensFlareEnabled() const { return lensFlareEnabled; }
		constexpr bool getMotionBlurEnabled() const { return motionBlurEnabled; }
		constexpr bool getDepthOfFieldEnabled() const { return depthOfFieldEnabled; }
		constexpr bool getEyeAdaptionEnabled() const { return eyeAdaptionEnabled; }
		constexpr bool getSharpenFilterEnabled() const { return sharpenFilterEnabled && getSharpenFilterAmount() > 0; }
		constexpr bool getCRTFilterEnabled() const { return crtFilterEnabled && getSharpenFilterAmount() > 0; }
		constexpr bool getOutlineEnabled() const { return outlineEnabled; }
		constexpr bool getChromaticAberrationEnabled() const { return chromaticAberrationEnabled; }
		constexpr bool getDitherEnabled() const { return ditherEnabled; }
		constexpr bool getOcclusionCullingEnabled() const { return occlusionCullingEnabled; }
		constexpr bool getSceneUpdateEnabled() const { return sceneUpdateEnabled; }
		constexpr bool getFSREnabled() const { return fsrEnabled; }
		constexpr bool getFSR2Enabled() const { return fsr2Enabled; }
		constexpr bool getVisibilityComputeShadingEnabled() const { return visibility_shading_in_compute; }
		constexpr bool getMeshBlendEnabled() const { return mesh_blend; }

		constexpr void setExposure(float value) { exposure = value; }
		constexpr void setHDRCalibration(float value) { hdr_calibration = value; }
		constexpr void setBrightness(float value) { brightness = value; }
		constexpr void setContrast(float value) { contrast = value; }
		constexpr void setSaturation(float value) { saturation = value; }
		constexpr void setBloomThreshold(float value) { bloomThreshold = value; }
		constexpr void setMotionBlurStrength(float value) { motionBlurStrength = value; }
		constexpr void setDepthOfFieldStrength(float value) { dofStrength = value; }
		constexpr void setSharpenFilterAmount(float value) { sharpenFilterAmount = value; }
		constexpr void setOutlineThreshold(float value) { outlineThreshold = value; }
		constexpr void setOutlineThickness(float value) { outlineThickness = value; }
		constexpr void setOutlineColor(const XMFLOAT4& value) { outlineColor = value; }
		constexpr void setAORange(float value) { aoRange = value; }
		constexpr void setAOSampleCount(uint32_t value) { aoSampleCount = value; }
		constexpr void setAOPower(float value) { aoPower = value; }
		constexpr void setChromaticAberrationAmount(float value) { chromaticAberrationAmount = value; }
		constexpr void setScreenSpaceShadowSampleCount(uint32_t value) { screenSpaceShadowSampleCount = value; }
		constexpr void setScreenSpaceShadowRange(float value) { screenSpaceShadowRange = value; }
		constexpr void setEyeAdaptionKey(float value) { eyeadaptionKey = value; }
		constexpr void setEyeAdaptionRate(float value) { eyeadaptionRate = value; }
		constexpr void setFSRSharpness(float value) { fsrSharpness = value; }
		constexpr void setFSR2Sharpness(float value) { fsr2Sharpness = value; }
		constexpr void setLightShaftsStrength(float value) { lightShaftsStrength = value; }
		constexpr void setLightShaftsFadeSpeed(float value) { lightShaftsFadeSpeed = value; }
		constexpr void setRaytracedDiffuseRange(float value) { raytracedDiffuseRange = value; }
		constexpr void setRaytracedReflectionsRange(float value) { raytracedReflectionsRange = value; }
		constexpr void setReflectionRoughnessCutoff(float value) { reflectionRoughnessCutoff = value; }
		constexpr void setSSGIDepthRejection(float value) { ssgiDepthRejection = value; }
		constexpr void setTonemap(Protean::Renderer::Tonemap value) { tonemap = value; }
		constexpr void setVisibilityComputeShadingEnabled(bool value) { visibility_shading_in_compute = value; }
		constexpr void setRaytracedDiffuseQuality(Protean::Renderer::PostProcessQuality value) { raytracedDiffuseQuality = value; }
		constexpr void setRaytracedReflectionsQuality(Protean::Renderer::PostProcessQuality value) { raytracedReflectionsQuality = value; }
		constexpr void setSSRQuality(Protean::Renderer::PostProcessQuality value) { ssrQuality = value; }

		void setAO(AO value);
		void setSSREnabled(bool value);
		void setSSGIEnabled(bool value);
		void setRaytracedReflectionsEnabled(bool value);
		void setRaytracedDiffuseEnabled(bool value);
		void setMotionBlurEnabled(bool value);
		void setDepthOfFieldEnabled(bool value);
		void setEyeAdaptionEnabled(bool value);
		void setReflectionsEnabled(bool value); // planar reflections
		void setPlanarReflectionQuality(float resolutionScale, uint32_t msaaSampleCount);
		void setBloomEnabled(bool value);
		void setVolumeLightsEnabled(bool value);
		void setLightShaftsEnabled(bool value);
		void setOutlineEnabled(bool value);
		constexpr void setShadowsEnabled(bool value) { shadowsEnabled = value; }
		constexpr void setFXAAEnabled(bool value) { fxaaEnabled = value; }
		constexpr void setColorGradingEnabled(bool value) { colorGradingEnabled = value; }
		constexpr void setLensFlareEnabled(bool value) { lensFlareEnabled = value; }
		constexpr void setSharpenFilterEnabled(bool value) { sharpenFilterEnabled = value; }
		constexpr void setCRTFilterEnabled(bool value) { crtFilterEnabled = value; }
		constexpr void setChromaticAberrationEnabled(bool value) { chromaticAberrationEnabled = value; }
		constexpr void setDitherEnabled(bool value) { ditherEnabled = value; }
		constexpr void setOcclusionCullingEnabled(bool value) { occlusionCullingEnabled = value; }
		constexpr void setSceneUpdateEnabled(bool value) { sceneUpdateEnabled = value; }
		constexpr void setMeshBlendEnabled(bool value) { mesh_blend = value; }
		void setFSREnabled(bool value);
		void setFSR2Enabled(bool value);
		void setFSR2Preset(FSR2_Preset preset); // this will modify resolution scaling and sampler lod bias

		struct CustomPostprocess
		{
			std::string name = "CustomPostprocess";
			Protean::Graphics::Shader computeshader;
			XMFLOAT4 params0;
			XMFLOAT4 params1;
			enum class Stage
			{
				BeforeTonemap, // Before tonemap and bloom in HDR color space
				AfterTonemap // After tonemap, in display color space
			} stage = Stage::AfterTonemap;
		};
		Protean::vector<CustomPostprocess> custom_post_processes;

		void PreUpdate() override;
		void Update(float dt) override;
		void PreRender() override;
		void Render() const override;
		void Compose(Protean::Graphics::CommandList cmd) const override;

		void Stop() override;
		void Start() override;

		// Creates screenshot of the render result and replaces background (sky) pixels with transparency
		//	The stencil parameters let you configure other stencil ops instead of making sky transparent which is default
		Protean::Graphics::Texture CreateScreenshotWithAlphaBackground(uint8_t stencilref = Protean::STENCILREF_EMPTY, Protean::Image::STENCILMODE stencilmode = Protean::Image::STENCILMODE_NOT, Protean::Image::STENCILREFMODE stencilrefmode = Protean::Image::STENCILREFMODE_ALL);

		// This is an identifier of RenderPath subtype that is used for lua binding.
		static constexpr const auto script_check_identifier = relative_path_storage(__FILE__);
		const char* GetScriptBindingID() const override { return script_check_identifier.c_str(); }
	};

}
