#pragma once
class EditorComponent;

class ExpressionWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::CheckBox talkCheckBox;
	Protean::Gui::Slider blinkFrequencySlider;
	Protean::Gui::Slider blinkLengthSlider;
	Protean::Gui::Slider blinkCountSlider;
	Protean::Gui::Slider lookFrequencySlider;
	Protean::Gui::Slider lookLengthSlider;
	Protean::Gui::TreeList expressionList;
	Protean::Gui::Slider weightSlider;
	Protean::Gui::CheckBox binaryCheckBox;
	Protean::Gui::ComboBox overrideMouthCombo;
	Protean::Gui::ComboBox overrideBlinkCombo;
	Protean::Gui::ComboBox overrideLookCombo;

	void ResizeLayout() override;
};

