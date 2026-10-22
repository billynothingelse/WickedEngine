#pragma once
class EditorComponent;

class LightWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	void SetLightType(Protean::LightComponent::LightType type);

	Protean::Gui::Slider intensitySlider;
	Protean::Gui::Slider rangeSlider;
	Protean::Gui::Slider radiusSlider;
	Protean::Gui::Slider lengthSlider;
	Protean::Gui::Slider heightSlider;
	Protean::Gui::Slider outerConeAngleSlider;
	Protean::Gui::Slider innerConeAngleSlider;
	Protean::Gui::Slider volumetricBoostSlider;
	Protean::Gui::CheckBox shadowCheckBox;
	Protean::Gui::CheckBox haloCheckBox;
	Protean::Gui::CheckBox volumetricsCheckBox;
	Protean::Gui::CheckBox staticCheckBox;
	Protean::Gui::CheckBox volumetricCloudsCheckBox;
	Protean::Gui::ColorPicker colorPicker;
	Protean::Gui::ComboBox typeSelectorComboBox;
	Protean::Gui::ComboBox shadowResolutionComboBox;
	Protean::Gui::ComboBox cameraComboBox;

	Protean::Gui::Label tipLabel;

	Protean::Gui::Label lensflare_Label;
	Protean::Gui::Button lensflare_Button[7];

	struct CascadeConfig
	{
		Protean::Gui::Slider distanceSlider;
		Protean::Gui::Button removeButton;
	};
	std::deque<CascadeConfig> cascades;
	Protean::Gui::Button addCascadeButton;
	void RefreshCascades();

	void ResizeLayout() override;
};

