#pragma once
class EditorComponent;

class GraphicsWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	Protean::Gui::CheckBox vsyncCheckBox;
	Protean::Gui::ComboBox swapchainComboBox;
	Protean::Gui::Slider hdrcalibrationSlider;
	Protean::Gui::Slider hdrScalingSlider;
	Protean::Gui::ComboBox renderPathComboBox;
	Protean::Gui::Slider pathTraceTargetSlider;
	Protean::Gui::Label pathTraceStatisticsLabel;
	Protean::Gui::CheckBox occlusionCullingCheckBox;
	Protean::Gui::CheckBox visibilityComputeShadingCheckBox;
	Protean::Gui::CheckBox meshShaderCheckBox;
	Protean::Gui::CheckBox meshletOcclusionCullingCheckBox;
	Protean::Gui::CheckBox shadowLODCheckBox;
	Protean::Gui::Slider resolutionScaleSlider;
	Protean::Gui::Slider streamingSlider;
	Protean::Gui::Slider GIBoostSlider;
	Protean::Gui::CheckBox surfelGICheckBox;
	Protean::Gui::ComboBox surfelGIDebugComboBox;
	Protean::Gui::CheckBox ddgiCheckBox;
	Protean::Gui::CheckBox ddgiDebugCheckBox;
	Protean::Gui::TextInputField ddgiX;
	Protean::Gui::TextInputField ddgiY;
	Protean::Gui::TextInputField ddgiZ;
	Protean::Gui::Slider ddgiRayCountSlider;
	Protean::Gui::Slider ddgiBlendSpeedSlider;
	Protean::Gui::Slider ddgiSmoothBackfaceSlider;
	Protean::Gui::CheckBox vxgiCheckBox;
	Protean::Gui::ComboBox vxgiDebugCombo;
	Protean::Gui::CheckBox vxgiReflectionsCheckBox;
	Protean::Gui::Slider vxgiVoxelSizeSlider;
	Protean::Gui::Slider vxgiRayStepSizeSlider;
	Protean::Gui::Slider vxgiMaxDistanceSlider;
	Protean::Gui::Slider speedMultiplierSlider;
	Protean::Gui::ComboBox shadowTypeComboBox;
	Protean::Gui::ComboBox shadowProps2DComboBox;
	Protean::Gui::ComboBox shadowPropsCubeComboBox;
	Protean::Gui::ComboBox MSAAComboBox;
	Protean::Gui::CheckBox temporalAACheckBox;
	Protean::Gui::CheckBox temporalAADebugCheckBox;
	Protean::Gui::ComboBox textureQualityComboBox;
	Protean::Gui::Slider mipLodBiasSlider;
	Protean::Gui::Slider raytraceBounceCountSlider;

	Protean::Gui::ComboBox tonemapCombo;
	Protean::Gui::Slider exposureSlider;
	Protean::Gui::Slider brightnessSlider;
	Protean::Gui::Slider contrastSlider;
	Protean::Gui::Slider saturationSlider;
	Protean::Gui::CheckBox lensFlareCheckBox;
	Protean::Gui::CheckBox lightShaftsCheckBox;
	Protean::Gui::Slider lightShaftsStrengthStrengthSlider;
	Protean::Gui::Slider lightShaftsFadeSpeedSlider;
	Protean::Gui::CheckBox capsuleshadowCheckbox;
	Protean::Gui::Slider capsuleshadowFadeSlider;
	Protean::Gui::Slider capsuleshadowAngleSlider;
	Protean::Gui::ComboBox aoComboBox;
	Protean::Gui::Slider aoPowerSlider;
	Protean::Gui::Slider aoRangeSlider;
	Protean::Gui::Slider aoSampleCountSlider;
	Protean::Gui::CheckBox reflectionsCheckBox;
	Protean::Gui::Slider planarReflectionResolutionScaleSlider;
	Protean::Gui::ComboBox planarReflectionSampleCountCombo;
	Protean::Gui::CheckBox ssrCheckBox;
	Protean::Gui::Slider ssrQualitySlider;
	Protean::Gui::CheckBox raytracedReflectionsCheckBox;
	Protean::Gui::Slider reflectionsRoughnessCutoffSlider;
	Protean::Gui::Slider raytracedReflectionsRangeSlider;
	Protean::Gui::Slider raytracedReflectionsQualitySlider;
	Protean::Gui::CheckBox raytracedDiffuseCheckBox;
	Protean::Gui::Slider raytracedDiffuseRangeSlider;
	Protean::Gui::Slider raytracedDiffuseQualitySlider;
	Protean::Gui::CheckBox ssgiCheckBox;
	Protean::Gui::Slider ssgiDepthRejectionSlider;
	Protean::Gui::CheckBox screenSpaceShadowsCheckBox;
	Protean::Gui::Slider screenSpaceShadowsStepCountSlider;
	Protean::Gui::Slider screenSpaceShadowsRangeSlider;
	Protean::Gui::CheckBox eyeAdaptionCheckBox;
	Protean::Gui::Slider eyeAdaptionKeySlider;
	Protean::Gui::Slider eyeAdaptionRateSlider;
	Protean::Gui::CheckBox motionBlurCheckBox;
	Protean::Gui::Slider motionBlurStrengthSlider;
	Protean::Gui::CheckBox depthOfFieldCheckBox;
	Protean::Gui::Slider depthOfFieldScaleSlider;
	Protean::Gui::CheckBox bloomCheckBox;
	Protean::Gui::Slider bloomStrengthSlider;
	Protean::Gui::CheckBox fxaaCheckBox;
	Protean::Gui::CheckBox colorGradingCheckBox;
	Protean::Gui::CheckBox ditherCheckBox;
	Protean::Gui::CheckBox crtFilterCheckBox;
	Protean::Gui::CheckBox sharpenFilterCheckBox;
	Protean::Gui::Slider sharpenFilterAmountSlider;
	Protean::Gui::CheckBox outlineCheckBox;
	Protean::Gui::Slider outlineThresholdSlider;
	Protean::Gui::Slider outlineThicknessSlider;
	Protean::Gui::CheckBox chromaticaberrationCheckBox;
	Protean::Gui::Slider chromaticaberrationSlider;
	Protean::Gui::CheckBox fsrCheckBox;
	Protean::Gui::Slider fsrSlider;
	Protean::Gui::CheckBox fsr2CheckBox;
	Protean::Gui::ComboBox fsr2Combo;
	Protean::Gui::Slider fsr2Slider;

	Protean::Gui::CheckBox variableRateShadingClassificationCheckBox;
	Protean::Gui::CheckBox variableRateShadingClassificationDebugCheckBox;
	Protean::Gui::CheckBox advancedLightCullingCheckBox;
	Protean::Gui::CheckBox debugLightCullingCheckBox;
	Protean::Gui::CheckBox tessellationCheckBox;

	Protean::Gui::CheckBox meshblendCheckBox;

	enum RENDERPATH
	{
		RENDERPATH_DEFAULT,
		RENDERPATH_PATHTRACING,
	};
	void ChangeRenderPath(RENDERPATH path);

	void UpdateSwapChainFormats(Protean::Graphics::SwapChain* swapChain);
	void UpdateData();

	void ResizeLayout() override;

	void ApplySamplerSettings();
};

