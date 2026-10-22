#pragma once
class EditorComponent;

class ForceFieldWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::ComboBox typeComboBox;
	Protean::Gui::Slider gravitySlider;
	Protean::Gui::Slider rangeSlider;

	void ResizeLayout() override;
};

