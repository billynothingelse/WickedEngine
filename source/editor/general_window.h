#pragma once
class EditorComponent;

class GeneralWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Gui::CheckBox versionCheckBox;
	Protean::Gui::CheckBox fpsCheckBox;
	Protean::Gui::CheckBox otherinfoCheckBox;
	Protean::Gui::Slider masterVolumeSlider;
	Protean::Gui::ComboBox themeCombo;
	Protean::Gui::Button themeEditorButton;
	Protean::Gui::ComboBox saveModeComboBox;
	Protean::Gui::CheckBox saveCompressionCheckBox;
	Protean::Gui::ComboBox entityTreeSortingComboBox;
	Protean::Gui::ComboBox languageCombo;

	Protean::Gui::CheckBox physicsDebugCheckBox;
	Protean::Gui::Slider physicsDebugMaxDistanceSlider;
	Protean::Gui::CheckBox nameDebugCheckBox;
	Protean::Gui::CheckBox gridHelperCheckBox;
	Protean::Gui::CheckBox aabbDebugCheckBox;
	Protean::Gui::CheckBox boneLinesCheckBox;
	Protean::Gui::CheckBox debugEmittersCheckBox;
	Protean::Gui::CheckBox debugForceFieldsCheckBox;
	Protean::Gui::CheckBox debugRaytraceBVHCheckBox;
	Protean::Gui::CheckBox placeInFrontOfCameraCheckBox;
	Protean::Gui::ComboBox wireFrameComboBox;
	Protean::Gui::CheckBox envProbesCheckBox;
	Protean::Gui::CheckBox cameraVisCheckBox;
	Protean::Gui::CheckBox colliderVisCheckBox;
	Protean::Gui::CheckBox springVisCheckBox;
	Protean::Gui::CheckBox splineVisCheckBox;
	Protean::Gui::CheckBox freezeCullingCameraCheckBox;
	Protean::Gui::CheckBox disableAlbedoMapsCheckBox;
	Protean::Gui::CheckBox forceDiffuseLightingCheckBox;
	Protean::Gui::CheckBox forceUnlitCheckBox;
	Protean::Gui::CheckBox focusModeCheckBox;
	Protean::Gui::CheckBox disableRoundCornersCheckBox;
	Protean::Gui::CheckBox disableGradientCheckBox;

	Protean::Gui::Label transformToolLabel;
	Protean::Gui::Slider transformToolOpacitySlider;
	Protean::Gui::Slider transformToolDarkenSlider;
	Protean::Gui::Slider transformToolScaleSlider;
	Protean::Gui::Slider transformToolThicknessSlider;

	Protean::Gui::Slider outlineOpacitySlider;
	Protean::Gui::Slider bonePickerOpacitySlider;
	Protean::Gui::CheckBox skeletonsVisibleCheckBox;

	Protean::Gui::Button localizationButton;
	Protean::Gui::Button eliminateCoarseCascadesButton;
	Protean::Gui::Button ddsConvButton;
	Protean::Gui::Button duplicateCollidersButton;

	std::string currentTheme;

	void ResizeLayout() override;

	void RefreshLanguageSelectionAfterWholeGUIWasInitialized();

	void RefreshTheme();
	void ReloadThemes();
};

