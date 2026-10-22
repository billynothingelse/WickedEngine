#pragma once
class EditorComponent;

class CameraPreview : public Protean::Gui::Widget
{
public:
	Protean::RenderPath3D renderpath;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	EditorComponent* editor = nullptr;

	void RenderPreview();

	void Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const override;
};

class CameraComponentWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Slider farPlaneSlider;
	Protean::Gui::Slider nearPlaneSlider;
	Protean::Gui::Slider fovSlider;
	Protean::Gui::Slider focalLengthSlider;
	Protean::Gui::Slider apertureSizeSlider;
	Protean::Gui::Slider apertureShapeXSlider;
	Protean::Gui::Slider apertureShapeYSlider;

	Protean::Gui::Button renderButton;
	bool renderEnabled = false;
	Protean::Gui::Slider resolutionXSlider;
	Protean::Gui::Slider resolutionYSlider;
	Protean::Gui::Slider samplecountSlider;
	Protean::Gui::Slider updateIntervalSlider;
	Protean::Gui::CheckBox crtCheckbox;

	CameraPreview preview;

	void ResizeLayout() override;
};

