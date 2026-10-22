#include "stdafx.h"
#include "theme_editor_window.h"

using namespace Protean;
void ThemeEditorWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	control_size = 30;

	Protean::Gui::Window::Create("Theme Editor");

	nameInput.Create("themeName");
	nameInput.SetDescription("Theme name: ");
	nameInput.SetCancelInputEnabled(false);
	nameInput.SetText("");
	AddWidget(&nameInput);

	const float siz = 64;

	idleButton.Create("themeIdleButton");
	idleButton.SetSize(XMFLOAT2(siz, siz));
	idleButton.SetDescription("Base");
	idleButton.SetTooltip("The main color of widgets when they are not in interaction.");
	idleButton.SetText("");
	idleButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	idleButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	idleButton.OnClick([this](Protean::Gui::EventArgs args) {
		mode = mode == ColorPickerMode_Idle ? ColorPickerMode_None : ColorPickerMode_Idle;
		UpdateColorPickerMode();
		});
	AddWidget(&idleButton);

	focusButton.Create("themeFocusButton");
	focusButton.SetSize(XMFLOAT2(siz, siz));
	focusButton.SetDescription("Highlight");
	focusButton.SetTooltip("The main color of widgets when they are in focus, hovered by the mouse.");
	focusButton.SetText("");
	focusButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	focusButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	focusButton.OnClick([this](Protean::Gui::EventArgs args) {
		mode = mode == ColorPickerMode_Focus ? ColorPickerMode_None : ColorPickerMode_Focus;
		UpdateColorPickerMode();
		});
	AddWidget(&focusButton);

	backgroundButton.Create("themeBackgroundButton");
	backgroundButton.SetSize(XMFLOAT2(siz, siz));
	backgroundButton.SetDescription("Background");
	backgroundButton.SetTooltip("The color of the gui background.");
	backgroundButton.SetText("");
	backgroundButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	backgroundButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	backgroundButton.OnClick([this](Protean::Gui::EventArgs args) {
		mode = mode == ColorPickerMode_Background ? ColorPickerMode_None : ColorPickerMode_Background;
		UpdateColorPickerMode();
		});
	AddWidget(&backgroundButton);

	shadowButton.Create("themeShadowButton");
	shadowButton.SetSize(XMFLOAT2(siz, siz));
	shadowButton.SetDescription("Shadow");
	shadowButton.SetTooltip("The color of the gui shadow/border.");
	shadowButton.SetText("");
	shadowButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	shadowButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	shadowButton.OnClick([this](Protean::Gui::EventArgs args) {
		mode = mode == ColorPickerMode_Shadow ? ColorPickerMode_None : ColorPickerMode_Shadow;
		UpdateColorPickerMode();
		});
	AddWidget(&shadowButton);

	fontButton.Create("themeFontButton");
	fontButton.SetSize(XMFLOAT2(siz, siz));
	fontButton.SetDescription("Font");
	fontButton.SetTooltip("The main color of the text.");
	fontButton.SetText("");
	fontButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	fontButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	fontButton.OnClick([this](Protean::Gui::EventArgs args) {
		mode = mode == ColorPickerMode_Font ? ColorPickerMode_None : ColorPickerMode_Font;
		UpdateColorPickerMode();
		});
	AddWidget(&fontButton);

	fontShadowButton.Create("themeFontShadowButton");
	fontShadowButton.SetSize(XMFLOAT2(siz, siz));
	fontShadowButton.SetDescription("Font shadow");
	fontShadowButton.SetTooltip("The text shadow color.");
	fontShadowButton.SetText("");
	fontShadowButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	fontShadowButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	fontShadowButton.OnClick([this](Protean::Gui::EventArgs args) {
		mode = mode == ColorPickerMode_FontShadow ? ColorPickerMode_None : ColorPickerMode_FontShadow;
		UpdateColorPickerMode();
		});
	AddWidget(&fontShadowButton);

	gradientButton.Create("themeGradientButton");
	gradientButton.SetSize(XMFLOAT2(siz, siz));
	gradientButton.SetDescription("Gradient");
	gradientButton.SetTooltip("The gradient color.");
	gradientButton.SetText("");
	gradientButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	gradientButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	gradientButton.OnClick([this](Protean::Gui::EventArgs args) {
		mode = mode == ColorPickerMode_Gradient ? ColorPickerMode_None : ColorPickerMode_Gradient;
		UpdateColorPickerMode();
		});
	AddWidget(&gradientButton);

	waveButton.Create("themeWaveButton");
	waveButton.SetSize(XMFLOAT2(siz, siz));
	waveButton.SetDescription("Wave");
	waveButton.SetTooltip("The wave color.");
	waveButton.SetText("");
	waveButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	waveButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	waveButton.OnClick([this](Protean::Gui::EventArgs args) {
		mode = mode == ColorPickerMode_Wave ? ColorPickerMode_None : ColorPickerMode_Wave;
		UpdateColorPickerMode();
		});
	AddWidget(&waveButton);

	colorpicker.Create("themeColorPicker", Protean::Gui::Window::WindowControls_NONE);
	colorpicker.SetSize(XMFLOAT2(256, 256));
	colorpicker.OnColorChanged([this](Protean::Gui::EventArgs args) {
		switch (mode)
		{
		case ThemeEditorWindow::ColorPickerMode_Idle:
			idleColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode_Focus:
			focusColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode_Background:
			backgroundColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode_Shadow:
			shadowColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode_Font:
			fontColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode_FontShadow:
			fontShadowColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode_Gradient:
			gradientColor = args.color;
			break;
		case ThemeEditorWindow::ColorPickerMode_Wave:
			waveColor = args.color;
			break;
		default:
			break;
		}
		Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
			});
		});
	AddWidget(&colorpicker);

	imageButton.Create("themeImage");
	imageButton.SetText("");
	imageButton.SetDescription("Background image: ");
	imageButton.font_description.params.v_align = Protean::Font::WIFALIGN_BOTTOM;
	imageButton.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	imageButton.OnClick([this](Protean::Gui::EventArgs args) {
		if (imageResource.IsValid())
		{
			imageResource = {};
			imageResourceName = {};
			Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
				});
		}
		else
		{
			Protean::Helper::FileDialogParams params;
			params.type = Protean::Helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = Protean::ResourceManager::GetSupportedImageExtensions();
			Protean::Helper::FileDialog(params, [this](std::string fileName) {
				Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					Protean::Resource res = Protean::ResourceManager::Load(fileName, Protean::ResourceManager::Flags_IMPORT_RETAIN_FILEDATA);
					if (!res.IsValid())
						return;
					imageResource = res;
					imageResourceName = Protean::Helper::GetFileNameFromPath(fileName);
					Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
						editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
						});
					});
				});
		}
		});
	AddWidget(&imageButton);

	imageSlider.Create(0, 1, 0.5f, 100, "themeImageSlider");
	imageSlider.SetText("Background image tint: ");
	imageSlider.SetTooltip("Control how much the background color tints the background image.");
	imageSlider.OnSlide([this](Protean::Gui::EventArgs args) {
		Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			editor->generalWnd.themeCombo.SetSelectedByUserdata(~0ull);
			});
		});
	AddWidget(&imageSlider);

	saveButton.Create("themeSave");
	saveButton.SetText("Save theme");
	saveButton.OnClick([this](Protean::Gui::EventArgs args) {
		std::string directory = Protean::Helper::GetCurrentPath() + "/themes/";
		Protean::Helper::DirectoryCreate(directory);
		static const uint64_t version = 3;
		Protean::Archive archive;
		archive.SetReadModeAndResetPos(false);
		archive << version;
		archive << idleColor.rgba;
		archive << focusColor.rgba;
		archive << backgroundColor.rgba;
		archive << shadowColor.rgba;
		archive << fontColor.rgba;
		archive << fontShadowColor.rgba;
		archive << imageResourceName;
		if (imageResource.IsValid())
		{
			archive << imageResource.GetFileData();
		}
		else
		{
			Protean::vector<uint8_t> emptyimage;
			archive << emptyimage;
		}

		// New version handlers should be added at the end, so older versions can open newer themes!
		if (version >= 2)
		{
			archive << gradientColor.rgba;
		}
		if (version >= 3)
		{
			archive << waveColor.rgba;
		}

		std::string name = nameInput.GetCurrentInputValue();
		if (name.empty())
			name = "Unnamed";
		std::string filename = directory + name + ".witheme";
		if (archive.SaveFile(filename))
		{
			Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				editor->PostSaveText("Saved theme:", filename);
				editor->main->config.GetSection("options").Set("theme", name);
				editor->generalWnd.ReloadThemes();
			});
		}
	});
	saveButton.SetAngularHighlightWidth(6);
	saveButton.SetSize(XMFLOAT2(64, 64));
	saveButton.font.params.scaling = 1.5f;
	AddWidget(&saveButton);

	SetVisible(false);
}

void ThemeEditorWindow::UpdateColorPickerMode()
{
	switch (mode)
	{
	case ThemeEditorWindow::ColorPickerMode_None:
		break;
	case ThemeEditorWindow::ColorPickerMode_Idle:
		colorpicker.SetPickColor(idleColor);
		break;
	case ThemeEditorWindow::ColorPickerMode_Focus:
		colorpicker.SetPickColor(focusColor);
		break;
	case ThemeEditorWindow::ColorPickerMode_Background:
		colorpicker.SetPickColor(backgroundColor);
		break;
	case ThemeEditorWindow::ColorPickerMode_Shadow:
		colorpicker.SetPickColor(shadowColor);
		break;
	case ThemeEditorWindow::ColorPickerMode_Font:
		colorpicker.SetPickColor(fontColor);
		break;
	case ThemeEditorWindow::ColorPickerMode_FontShadow:
		colorpicker.SetPickColor(fontShadowColor);
		break;
	case ThemeEditorWindow::ColorPickerMode_Gradient:
		colorpicker.SetPickColor(gradientColor);
		break;
	case ThemeEditorWindow::ColorPickerMode_Wave:
		colorpicker.SetPickColor(waveColor);
		break;
	default:
		break;
	}
}

void ThemeEditorWindow::Update(const Protean::Canvas& canvas, float dt)
{
	const bool gui_round_enabled = !editor->generalWnd.disableRoundCornersCheckBox.GetCheck();
	if (mode == ColorPickerMode_None)
	{
		colorpicker.SetVisible(false);
	}
	else
	{
		colorpicker.SetVisible(true);
	}

	idleButton.SetShadowRadius(1);
	focusButton.SetShadowRadius(1);
	backgroundButton.SetShadowRadius(1);
	shadowButton.SetShadowRadius(1);
	fontButton.SetShadowRadius(1);
	fontShadowButton.SetShadowRadius(1);
	gradientButton.SetShadowRadius(1);
	waveButton.SetShadowRadius(1);

	const float rad = 6;
	switch (mode)
	{
	case ThemeEditorWindow::ColorPickerMode_Idle:
		idleButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Idle color");
		break;
	case ThemeEditorWindow::ColorPickerMode_Focus:
		focusButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Focus color");
		break;
	case ThemeEditorWindow::ColorPickerMode_Background:
		backgroundButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Background color");
		break;
	case ThemeEditorWindow::ColorPickerMode_Shadow:
		shadowButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Shadow color");
		break;
	case ThemeEditorWindow::ColorPickerMode_Font:
		fontButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Font color");
		break;
	case ThemeEditorWindow::ColorPickerMode_FontShadow:
		fontShadowButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Font shadow color");
		break;
	case ThemeEditorWindow::ColorPickerMode_Gradient:
		gradientButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Gradient color");
		break;
	case ThemeEditorWindow::ColorPickerMode_Wave:
		waveButton.SetShadowRadius(rad);
		colorpicker.label.SetText("Wave color");
		break;
	default:
		break;
	}

	idleButton.SetColor(idleColor);
	focusButton.SetColor(focusColor);
	backgroundButton.SetColor(backgroundColor);
	shadowButton.SetColor(shadowColor);
	fontButton.SetColor(fontColor);
	fontShadowButton.SetColor(fontShadowColor);
	gradientButton.SetColor(gradientColor);
	waveButton.SetColor(waveColor);

	for (int i = 0; i < arraysize(editor->newSceneButton.sprites); ++i)
	{
		auto apply_round_corners = [&](Protean::Gui::Button& b) {
			if (gui_round_enabled)
			{
				b.sprites[i].params.enableCornerRounding();
				b.sprites[i].params.corners_rounding[0].radius = 20;
				b.sprites[i].params.corners_rounding[1].radius = 20;
				b.sprites[i].params.corners_rounding[2].radius = 20;
				b.sprites[i].params.corners_rounding[3].radius = 20;
			}
			else
			{
				b.sprites[i].params.disableCornerRounding();
			}
		};
		apply_round_corners(idleButton);
		apply_round_corners(focusButton);
		apply_round_corners(backgroundButton);
		apply_round_corners(shadowButton);
		apply_round_corners(fontButton);
		apply_round_corners(fontShadowButton);
		apply_round_corners(gradientButton);
		apply_round_corners(waveButton);
		apply_round_corners(imageButton);
	}

	imageButton.SetShadowRadius(8);
	imageButton.SetColor(Protean::Color::White(), Protean::Gui::IDLE);
	if (imageButton.sprites[Protean::Gui::IDLE].textureResource.IsValid())
	{
		imageSlider.SetVisible(true);
	}
	else
	{
		imageButton.SetSize(XMFLOAT2(10, 1));
		imageSlider.SetVisible(false);
	}
	imageButton.SetImage(imageResource);

	Protean::Gui::Window::Update(canvas, dt);
}
void ThemeEditorWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();

	layout.add_fullwidth(nameInput);

	layout.jump(32);

	layout.padding = 20;
	layout.add_right(idleButton, focusButton, backgroundButton, shadowButton, fontButton, fontShadowButton, gradientButton, waveButton);
	layout.padding = 4;

	layout.add_fullwidth(colorpicker);

	layout.jump();

	layout.add_fullwidth_aspect(imageButton);
	layout.add_fullwidth(imageSlider);

	layout.jump();

	layout.add_fullwidth(saveButton);
}
