#pragma once
class EditorComponent;

class SpriteWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Button textureButton;
	Protean::Gui::Button maskButton;
	Protean::Gui::Slider pivotXSlider;
	Protean::Gui::Slider pivotYSlider;
	Protean::Gui::Slider intensitySlider;
	Protean::Gui::Slider rotationSlider;
	Protean::Gui::Slider saturationSlider;
	Protean::Gui::Slider alphaStartSlider;
	Protean::Gui::Slider alphaEndSlider;
	Protean::Gui::Slider borderSoftenSlider;
	Protean::Gui::Slider cornerRounding0Slider;
	Protean::Gui::Slider cornerRounding1Slider;
	Protean::Gui::Slider cornerRounding2Slider;
	Protean::Gui::Slider cornerRounding3Slider;
	Protean::Gui::ComboBox qualityCombo;
	Protean::Gui::ComboBox samplemodeCombo;
	Protean::Gui::ComboBox blendModeCombo;
	Protean::Gui::CheckBox mirrorCheckBox;
	Protean::Gui::CheckBox hiddenCheckBox;
	Protean::Gui::CheckBox cameraFacingCheckBox;
	Protean::Gui::CheckBox cameraScalingCheckBox;
	Protean::Gui::CheckBox depthTestCheckBox;
	Protean::Gui::CheckBox distortionCheckBox;
	Protean::Gui::ColorPicker colorPicker;

	Protean::Gui::Slider movingTexXSlider;
	Protean::Gui::Slider movingTexYSlider;

	Protean::Gui::Slider drawrectFrameRateSlider;
	Protean::Gui::TextInputField drawrectFrameCountInput;
	Protean::Gui::TextInputField drawrectHorizontalFrameCountInput;

	Protean::Gui::Slider wobbleXSlider;
	Protean::Gui::Slider wobbleYSlider;
	Protean::Gui::Slider wobbleSpeedSlider;

	void ResizeLayout() override;

	void UpdateSpriteDrawRectParams(Protean::Sprite* sprite);
};
