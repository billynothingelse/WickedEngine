#pragma once
class EditorComponent;

class ProjectCreatorWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	Protean::Gui::Label infoLabel;
	Protean::Gui::TextInputField projectNameInput;
	Protean::Gui::Button reloadButton;
	Protean::Gui::Button iconButton;
	Protean::Gui::Button thumbnailButton;
	Protean::Gui::Button splashScreenButton;

	Protean::Gui::Label cursorLabel;
	Protean::Gui::Button cursorButton;

	Protean::Gui::ColorPicker fontColorPicker;
	Protean::Gui::ColorPicker backgroundColorPicker;
	Protean::Gui::Button colorPreviewButton;

	Protean::Gui::Button createButton;

	Protean::Resource iconResource;
	Protean::Resource thumbnailResource;
	Protean::Resource splashScreenResource;
	Protean::Resource splashScreenResourceCroppedPreview;
	Protean::Resource cursorResource;

	std::string iconFilename;
	std::string thumbnailFilename;
	std::string splashScreenFilename;
	std::string cursorFilename;

	float hotspotX = 0.5f;
	float hotspotY = 0.5f;

	void Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const override;
	void ResizeLayout() override;
};

