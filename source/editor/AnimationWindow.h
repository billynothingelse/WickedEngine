#pragma once
class EditorComponent;

class AnimationWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::ComboBox modeComboBox;
	Protean::Gui::Button loopTypeButton;
	Protean::Gui::Button	playButton;
	Protean::Gui::Button	playFromStartButton;
	Protean::Gui::Button	backwardsButton;
	Protean::Gui::Button	backwardsFromEndButton;
	Protean::Gui::Button	stopButton;
	Protean::Gui::Slider	timerSlider;
	Protean::Gui::Slider	amountSlider;
	Protean::Gui::Slider	speedSlider;
	Protean::Gui::TextInputField startInput;
	Protean::Gui::TextInputField endInput;

	Protean::Gui::ComboBox recordCombo;
	Protean::Gui::TreeList keyframesList;

	Protean::Gui::ComboBox retargetCombo;

	Protean::Gui::CheckBox rootMotionCheckBox;
	Protean::Gui::ComboBox rootBoneComboBox;

	void UpdateData();

	void RefreshKeyframesList();

	void ResizeLayout() override;
};
