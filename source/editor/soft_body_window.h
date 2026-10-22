#pragma once
class EditorComponent;

class SoftBodyWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::Button resetButton;
	Protean::Gui::Slider detailSlider;
	Protean::Gui::Slider massSlider;
	Protean::Gui::Slider frictionSlider;
	Protean::Gui::Slider pressureSlider;
	Protean::Gui::Slider restitutionSlider;
	Protean::Gui::Slider vertexRadiusSlider;
	Protean::Gui::CheckBox windCheckbox;

	void ResizeLayout() override;
};

