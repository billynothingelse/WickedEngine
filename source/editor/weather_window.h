#pragma once
class EditorComponent;

class WeatherWindow : public Protean::Gui::Window
{
	void UpdateWind();
public:
	void Create(EditorComponent* editor);

	void UpdateData();

	EditorComponent* editor = nullptr;

	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);
	Protean::WeatherComponent& GetWeather() const;
	void InvalidateProbes() const;

	XMFLOAT3 default_sky_horizon = XMFLOAT3(0, 0, 0);
	XMFLOAT3 default_sky_zenith = XMFLOAT3(0, 0, 0);

	Protean::Gui::Button primaryButton;
	Protean::Gui::CheckBox overrideFogColorCheckBox;
	Protean::Gui::CheckBox heightFogCheckBox;
	Protean::Gui::Slider fogStartSlider;
	Protean::Gui::Slider fogDensitySlider;
	Protean::Gui::Slider fogHeightStartSlider;
	Protean::Gui::Slider fogHeightEndSlider;
	Protean::Gui::Slider gravitySlider;
	Protean::Gui::Slider windSpeedSlider;
	Protean::Gui::Slider windMagnitudeSlider;
	Protean::Gui::Slider windDirectionSlider;
	Protean::Gui::Slider windWaveSizeSlider;
	Protean::Gui::Slider windRandomnessSlider;
	Protean::Gui::Slider skyExposureSlider;
	Protean::Gui::Slider starsSlider;
	Protean::Gui::Slider skyRotationSlider;
	Protean::Gui::Slider rainAmountSlider;
	Protean::Gui::Slider rainLengthSlider;
	Protean::Gui::Slider rainSpeedSlider;
	Protean::Gui::Slider rainScaleSlider;
	Protean::Gui::Slider rainSplashScaleSlider;
	Protean::Gui::CheckBox realisticskyCheckBox;
	Protean::Gui::CheckBox aerialperspectiveCheckBox;
	Protean::Gui::CheckBox realisticskyHighQualityCheckBox;
	Protean::Gui::CheckBox realisticskyReceiveShadowCheckBox;
	Protean::Gui::Button skyButton;
	Protean::Gui::Button colorgradingButton;

	// ocean params:
	Protean::Gui::CheckBox ocean_enabledCheckBox;
	Protean::Gui::Slider ocean_patchSizeSlider;
	Protean::Gui::Slider ocean_waveAmplitudeSlider;
	Protean::Gui::Slider ocean_choppyScaleSlider;
	Protean::Gui::Slider ocean_windDependencySlider;
	Protean::Gui::Slider ocean_timeScaleSlider;
	Protean::Gui::Slider ocean_heightSlider;
	Protean::Gui::Slider ocean_detailSlider;
	Protean::Gui::Slider ocean_toleranceSlider;
	Protean::Gui::Button ocean_resetButton;

	Protean::Gui::ComboBox colorComboBox;
	Protean::Gui::ColorPicker colorPicker;

	// volumetric clouds:
	Protean::Gui::CheckBox volumetricCloudsCheckBox;
	Protean::Gui::CheckBox volumetricCloudsReceiveShadowCheckBox;
	Protean::Gui::CheckBox volumetricCloudsCastShadowCheckBox;
	Protean::Gui::Slider cloudStartHeightSlider;
	Protean::Gui::Slider cloudThicknessSlider;
	Protean::Gui::Slider skewAlongWindDirectionFirstSlider;
	Protean::Gui::Slider totalNoiseScaleFirstSlider;
	Protean::Gui::Slider curlScaleFirstSlider;
	Protean::Gui::Slider curlNoiseHeightFractionFirstSlider;
	Protean::Gui::Slider curlNoiseModifierFirstSlider;
	Protean::Gui::Slider detailScaleFirstSlider;
	Protean::Gui::Slider detailNoiseHeightFractionFirstSlider;
	Protean::Gui::Slider detailNoiseModifierFirstSlider;
	Protean::Gui::Slider skewAlongCoverageWindDirectionFirstSlider;
	Protean::Gui::Slider weatherScaleFirstSlider;
	Protean::Gui::Slider coverageAmountFirstSlider;
	Protean::Gui::Slider coverageMinimumFirstSlider;
	Protean::Gui::Slider typeAmountFirstSlider;
	Protean::Gui::Slider typeMinimumFirstSlider;
	Protean::Gui::Slider rainAmountFirstSlider;
	Protean::Gui::Slider rainMinimumFirstSlider;
	Protean::Gui::TextInputField gradientSmallFirstTextFields[4];
	Protean::Gui::TextInputField gradientMediumFirstTextFields[4];
	Protean::Gui::TextInputField gradientLargeFirstTextFields[4];
	Protean::Gui::TextInputField anvilDeformationSmallFirstTextFields[4];
	Protean::Gui::TextInputField anvilDeformationMediumFirstTextFields[4];
	Protean::Gui::TextInputField anvilDeformationLargeFirstTextFields[4];
	Protean::Gui::Slider windSpeedFirstSlider;
	Protean::Gui::Slider windAngleFirstSlider;
	Protean::Gui::Slider windUpAmountFirstSlider;
	Protean::Gui::Slider coverageWindSpeedFirstSlider;
	Protean::Gui::Slider coverageWindAngleFirstSlider;
	Protean::Gui::Slider coverageAmountSecondSlider;
	Protean::Gui::Slider coverageMinimumSecondSlider;
	Protean::Gui::Button volumetricCloudsWeatherMapFirstButton;
	Protean::Gui::Button volumetricCloudsWeatherMapSecondButton;

	Protean::Gui::Button preset0Button;
	Protean::Gui::Button preset1Button;
	Protean::Gui::Button preset2Button;
	Protean::Gui::Button preset3Button;
	Protean::Gui::Button preset4Button;
	Protean::Gui::Button preset5Button;

	void ResizeLayout() override;
};

