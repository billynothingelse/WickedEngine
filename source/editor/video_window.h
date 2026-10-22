#pragma once
class EditorComponent;

class VideoPreview : public Protean::Gui::Widget
{
public:
	Protean::VideoComponent* video = nullptr;

	void Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const override;
};

class VideoWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Button openButton;
	Protean::Gui::Label filenameLabel;
	Protean::Gui::Button playpauseButton;
	Protean::Gui::Button stopButton;
	Protean::Gui::Slider volumeSlider;
	Protean::Gui::CheckBox loopedCheckbox;
	VideoPreview preview;
	Protean::Gui::Slider timerSlider;
	Protean::Gui::Label infoLabel;

	void ResizeLayout() override;
};
