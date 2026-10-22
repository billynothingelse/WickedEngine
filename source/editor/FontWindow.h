#pragma once
class EditorComponent;

class FontWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::TextInputField textInput;
	Protean::Gui::Button fileButton;
	Protean::Gui::Button fontStyleButton;
	Protean::Gui::ComboBox fontSizeCombo;
	Protean::Gui::ComboBox hAlignCombo;
	Protean::Gui::ComboBox vAlignCombo;
	Protean::Gui::Slider rotationSlider;
	Protean::Gui::Slider spacingSlider;
	Protean::Gui::Slider softnessSlider;
	Protean::Gui::Slider boldenSlider;
	Protean::Gui::Slider shadowSoftnessSlider;
	Protean::Gui::Slider shadowBoldenSlider;
	Protean::Gui::Slider shadowOffsetXSlider;
	Protean::Gui::Slider shadowOffsetYSlider;
	Protean::Gui::Slider intensitySlider;
	Protean::Gui::Slider shadowIntensitySlider;
	Protean::Gui::CheckBox hiddenCheckBox;
	Protean::Gui::CheckBox cameraFacingCheckBox;
	Protean::Gui::CheckBox cameraScalingCheckBox;
	Protean::Gui::CheckBox depthTestCheckBox;
	Protean::Gui::CheckBox sdfCheckBox;
	Protean::Gui::ComboBox colorModeCombo;
	Protean::Gui::ColorPicker colorPicker;

	Protean::Gui::Label typewriterInfoLabel;
	Protean::Gui::Slider typewriterTimeSlider;
	Protean::Gui::CheckBox typewriterLoopedCheckBox;
	Protean::Gui::TextInputField typewriterStartInput;

	void ResizeLayout() override;
};
