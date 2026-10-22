#pragma once
class EditorComponent;

class ColliderWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::CheckBox cpuCheckBox;
	Protean::Gui::CheckBox gpuCheckBox;
	Protean::Gui::ComboBox shapeCombo;
	Protean::Gui::Slider radiusSlider;
	Protean::Gui::Slider offsetX;
	Protean::Gui::Slider offsetY;
	Protean::Gui::Slider offsetZ;
	Protean::Gui::Slider tailX;
	Protean::Gui::Slider tailY;
	Protean::Gui::Slider tailZ;

	void ResizeLayout() override;
};

