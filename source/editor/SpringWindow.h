#pragma once
class EditorComponent;

class SpringWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Button resetAllButton;
	Protean::Gui::CheckBox disabledCheckBox;
	Protean::Gui::CheckBox gravityCheckBox;
	Protean::Gui::Slider stiffnessSlider;
	Protean::Gui::Slider dragSlider;
	Protean::Gui::Slider windSlider;
	Protean::Gui::Slider gravitySlider;
	Protean::Gui::Slider hitradiusSlider;

	void ResizeLayout() override;
};

