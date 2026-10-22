#pragma once
class EditorComponent;

class ConstraintWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::CheckBox collisionCheckBox;
	Protean::Gui::CheckBox physicsDebugCheckBox;
	Protean::Gui::Slider constraintDebugSlider;
	Protean::Gui::Button rebindButton;
	Protean::Gui::ComboBox typeComboBox;
	Protean::Gui::ComboBox bodyAComboBox;
	Protean::Gui::ComboBox bodyBComboBox;
	Protean::Gui::Slider minSlider;
	Protean::Gui::Slider maxSlider;
	Protean::Gui::Slider breakSlider;

	Protean::Gui::Slider motorSlider1;
	Protean::Gui::Slider motorSlider2;

	Protean::Gui::Slider normalConeSlider;
	Protean::Gui::Slider planeConeSlider;

	Protean::Gui::Button fixedXButton;
	Protean::Gui::Button fixedYButton;
	Protean::Gui::Button fixedZButton;
	Protean::Gui::Button fixedXRotationButton;
	Protean::Gui::Button fixedYRotationButton;
	Protean::Gui::Button fixedZRotationButton;
	Protean::Gui::Slider minTranslationXSlider;
	Protean::Gui::Slider minTranslationYSlider;
	Protean::Gui::Slider minTranslationZSlider;
	Protean::Gui::Slider maxTranslationXSlider;
	Protean::Gui::Slider maxTranslationYSlider;
	Protean::Gui::Slider maxTranslationZSlider;
	Protean::Gui::Slider minRotationXSlider;
	Protean::Gui::Slider minRotationYSlider;
	Protean::Gui::Slider minRotationZSlider;
	Protean::Gui::Slider maxRotationXSlider;
	Protean::Gui::Slider maxRotationYSlider;
	Protean::Gui::Slider maxRotationZSlider;

	void ResizeLayout() override;
};

