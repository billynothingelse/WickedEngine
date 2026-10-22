#pragma once
class EditorComponent;

class EnvProbeWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::Image preview;
	Protean::Gui::CheckBox realTimeCheckBox;
	Protean::Gui::CheckBox msaaCheckBox;
	Protean::Gui::Slider realtimeFrameIntervalSlider;
	Protean::Gui::Slider viewdistanceSlider;
	Protean::Gui::Button refreshButton;
	Protean::Gui::Button refreshAllButton;
	Protean::Gui::Button importButton;
	Protean::Gui::Button exportButton;
	Protean::Gui::ComboBox resolutionCombo;

	void ResizeLayout() override;
};

