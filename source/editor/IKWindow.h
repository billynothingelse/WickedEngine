#pragma once
class EditorComponent;

class IKWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::ComboBox targetCombo;
	Protean::Gui::CheckBox disabledCheckBox;
	Protean::Gui::Slider chainLengthSlider;
	Protean::Gui::Slider iterationCountSlider;

	void ResizeLayout() override;
};

