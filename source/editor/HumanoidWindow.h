#pragma once
class EditorComponent;

class HumanoidWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);
	void RefreshBoneList();

	Protean::Gui::Label infoLabel;
	Protean::Gui::Button importAnimationsButton;
	Protean::Gui::Button activateAllRagdollsButton;
	Protean::Gui::ComboBox animationTesterCombo;
	Protean::Gui::CheckBox lookatMouseCheckBox;
	Protean::Gui::CheckBox lookatCheckBox;
	Protean::Gui::ComboBox lookatEntityCombo;
	Protean::Gui::CheckBox ragdollDisabledCheckBox;
	Protean::Gui::CheckBox ragdollCheckBox;
	Protean::Gui::CheckBox ragdoll2DCheckBox;
	Protean::Gui::CheckBox capsuleShadowCheckBox;
	Protean::Gui::Slider headRotMaxXSlider;
	Protean::Gui::Slider headRotMaxYSlider;
	Protean::Gui::Slider headRotSpeedSlider;
	Protean::Gui::Slider eyeRotMaxXSlider;
	Protean::Gui::Slider eyeRotMaxYSlider;
	Protean::Gui::Slider eyeRotSpeedSlider;
	Protean::Gui::Slider headSizeSlider;
	Protean::Gui::Slider ragdollFatnessSlider;
	Protean::Gui::Slider ragdollHeadSizeSlider;
	Protean::Gui::Slider armSpacingSlider;
	Protean::Gui::Slider legSpacingSlider;
	Protean::Gui::TreeList boneList;

	void ResizeLayout() override;

	void UpdateHumanoids();
};

