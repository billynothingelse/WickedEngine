#pragma once
class EditorComponent;

class ThemeEditorWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	Protean::Gui::TextInputField nameInput;

	Protean::Gui::Button idleButton;
	Protean::Gui::Button focusButton;
	Protean::Gui::Button backgroundButton;
	Protean::Gui::Button shadowButton;
	Protean::Gui::Button fontButton;
	Protean::Gui::Button fontShadowButton;
	Protean::Gui::Button gradientButton;
	Protean::Gui::Button waveButton;

	Protean::Gui::ColorPicker colorpicker;

	Protean::Gui::Button imageButton;
	Protean::Gui::Slider imageSlider;

	Protean::Gui::Button saveButton;

	Protean::Color idleColor;
	Protean::Color focusColor;
	Protean::Color backgroundColor;
	Protean::Color shadowColor;
	Protean::Color fontColor;
	Protean::Color fontShadowColor;
	Protean::Color gradientColor;
	Protean::Color waveColor;

	Protean::Resource imageResource;
	std::string imageResourceName;

	enum ColorPickerMode {
		ColorPickerMode_None,
		ColorPickerMode_Idle,
		ColorPickerMode_Focus,
		ColorPickerMode_Background,
		ColorPickerMode_Shadow,
		ColorPickerMode_Font,
		ColorPickerMode_FontShadow,
		ColorPickerMode_Gradient,
		ColorPickerMode_Wave,
	} mode;
	void UpdateColorPickerMode();

	void Update(const Protean::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};

