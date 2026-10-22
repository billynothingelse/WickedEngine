#pragma once
class EditorComponent;

class DecalWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::CheckBox placementCheckBox;
	Protean::Gui::CheckBox onlyalphaCheckBox;
	Protean::Gui::Slider slopeBlendPowerSlider;

	void ResizeLayout() override;
};

