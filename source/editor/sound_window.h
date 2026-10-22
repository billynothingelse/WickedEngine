#pragma once
class EditorComponent;

class WaveGraph : public Protean::Gui::Widget
{
public:
	Protean::SoundComponent* sound = nullptr;

	void Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const override;
};

class SoundWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Button openButton;
	Protean::Gui::ComboBox reverbComboBox;
	Protean::Gui::Label filenameLabel;
	Protean::Gui::Button playstopButton;
	Protean::Gui::CheckBox loopedCheckbox;
	Protean::Gui::CheckBox reverbCheckbox;
	Protean::Gui::CheckBox disable3dCheckbox;
	Protean::Gui::Slider volumeSlider;
	Protean::Gui::ComboBox submixComboBox;
	Protean::Gui::TextInputField beginInput;
	Protean::Gui::TextInputField lengthInput;
	Protean::Gui::TextInputField loopBeginInput;
	Protean::Gui::TextInputField loopLengthInput;
	WaveGraph waveGraph;

	void ResizeLayout() override;
};
