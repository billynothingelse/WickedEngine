#pragma once
class EditorComponent;

class CameraWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	void ResetCam();

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);
	void UpdateData();


	Protean::Gui::Slider farPlaneSlider;
	Protean::Gui::Slider nearPlaneSlider;
	Protean::Gui::Slider fovSlider;
	Protean::Gui::Slider focalLengthSlider;
	Protean::Gui::Slider apertureSizeSlider;
	Protean::Gui::Slider apertureShapeXSlider;
	Protean::Gui::Slider apertureShapeYSlider;
	Protean::Gui::Slider movespeedSlider;
	Protean::Gui::Slider rotationspeedSlider;
	Protean::Gui::Slider accelerationSlider;
	Protean::Gui::Button resetButton;
	Protean::Gui::CheckBox fpsCheckBox;
	Protean::Gui::CheckBox orthoCheckBox;

	Protean::Gui::Button proxyButton;
	Protean::Gui::CheckBox followCheckBox;
	Protean::Gui::Slider followSlider;

	void ResizeLayout() override;
};

