#pragma once
class EditorComponent;

class ObjectWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::ComboBox meshCombo;
	Protean::Gui::CheckBox renderableCheckBox;
	Protean::Gui::CheckBox shadowCheckBox;
	Protean::Gui::CheckBox navmeshCheckBox;
	Protean::Gui::CheckBox foregroundCheckBox;
	Protean::Gui::CheckBox notVisibleInMainCameraCheckBox;
	Protean::Gui::CheckBox notVisibleInReflectionsCheckBox;
	Protean::Gui::CheckBox wetmapCheckBox;
	Protean::Gui::Slider ditherSlider;
	Protean::Gui::Slider alphaRefSlider;
	Protean::Gui::Slider rimHighlightIntesitySlider;
	Protean::Gui::Slider rimHighlightFalloffSlider;
	Protean::Gui::Slider cascadeMaskSlider;
	Protean::Gui::Slider lodSlider;
	Protean::Gui::Slider drawdistanceSlider;
	Protean::Gui::Slider sortPrioritySlider;

	Protean::Gui::ComboBox colorComboBox;
	Protean::Gui::ColorPicker colorPicker;

	Protean::Gui::Slider lightmapResolutionSlider;
	Protean::Gui::CheckBox lightmapBlockCompressionCheckBox;
	Protean::Gui::ComboBox lightmapSourceUVSetComboBox;
	Protean::Gui::Button generateLightmapButton;
	Protean::Gui::Button stopLightmapGenButton;
	Protean::Gui::Button clearLightmapButton;
	Protean::Gui::Button lightmapPreviewButton;

	Protean::Gui::Button vertexAOButton;
	bool deleteAOMode = false;
	Protean::Gui::Slider vertexAORayCountSlider;
	Protean::Gui::Slider vertexAORayLengthSlider;

	void ResizeLayout() override;
};

