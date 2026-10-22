#pragma once
class EditorComponent;

class MaterialWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::TextInputField materialNameField;
	Protean::Gui::Button presetLoadButton;
	Protean::Gui::Button presetSaveButton;
	Protean::Gui::CheckBox shadowReceiveCheckBox;
	Protean::Gui::CheckBox shadowCasterCheckBox;
	Protean::Gui::CheckBox useVertexColorsCheckBox;
	Protean::Gui::CheckBox specularGlossinessCheckBox;
	Protean::Gui::CheckBox occlusionPrimaryCheckBox;
	Protean::Gui::CheckBox occlusionSecondaryCheckBox;
	Protean::Gui::CheckBox vertexAOCheckBox;
	Protean::Gui::CheckBox windCheckBox;
	Protean::Gui::CheckBox doubleSidedCheckBox;
	Protean::Gui::CheckBox outlineCheckBox;
	Protean::Gui::CheckBox preferUncompressedCheckBox;
	Protean::Gui::CheckBox disableStreamingCheckBox;
	Protean::Gui::CheckBox coplanarCheckBox;
	Protean::Gui::CheckBox capsuleShadowCheckBox;
	Protean::Gui::ComboBox shaderTypeComboBox;
	Protean::Gui::ComboBox blendModeComboBox;
	Protean::Gui::ComboBox shadingRateComboBox;
	Protean::Gui::ComboBox cameraComboBox;
	Protean::Gui::Slider normalMapSlider;
	Protean::Gui::Slider roughnessSlider;
	Protean::Gui::Slider reflectanceSlider;
	Protean::Gui::Slider metalnessSlider;
	Protean::Gui::Slider emissiveSlider;
	Protean::Gui::Slider saturationSlider;
	Protean::Gui::Slider cloakSlider;
	Protean::Gui::Slider chromaticAberrationSlider;
	Protean::Gui::Slider transmissionSlider;
	Protean::Gui::Slider refractionSlider;
	Protean::Gui::Slider pomSlider;
	Protean::Gui::Slider anisotropyStrengthSlider;
	Protean::Gui::Slider anisotropyRotationSlider;
	Protean::Gui::Slider displacementMappingSlider;
	Protean::Gui::Slider subsurfaceScatteringSlider;
	Protean::Gui::Slider texAnimFrameRateSlider;
	Protean::Gui::Slider texAnimDirectionSliderU;
	Protean::Gui::Slider texAnimDirectionSliderV;
	Protean::Gui::Slider texMulSliderX;
	Protean::Gui::Slider texMulSliderY;
	Protean::Gui::Slider alphaRefSlider;
	Protean::Gui::Slider sheenRoughnessSlider;
	Protean::Gui::Slider clearcoatSlider;
	Protean::Gui::Slider clearcoatRoughnessSlider;
	Protean::Gui::Slider blendTerrainSlider;
	Protean::Gui::Slider meshblendSlider;
	Protean::Gui::Slider interiorScaleXSlider;
	Protean::Gui::Slider interiorScaleYSlider;
	Protean::Gui::Slider interiorScaleZSlider;
	Protean::Gui::Slider interiorOffsetXSlider;
	Protean::Gui::Slider interiorOffsetYSlider;
	Protean::Gui::Slider interiorOffsetZSlider;
	Protean::Gui::Slider interiorRotationSlider;

	Protean::Gui::ComboBox colorComboBox;
	Protean::Gui::ColorPicker colorPicker;

	Protean::Gui::ComboBox textureSlotComboBox;
	Protean::Gui::Image textureSlotImage;
	Protean::Gui::Button textureSlotLoadButton;
	Protean::Gui::Button textureSlotSelectButton;
	Protean::Gui::Button textureSlotClearButton;
	Protean::Gui::Label textureSlotLabel;
	Protean::Gui::TextInputField textureSlotUvsetField;

	Protean::unordered_map<std::string, Protean::Resource> uniqueTextures;
	Protean::Gui::Window texturePickerWindow;
	std::deque<Protean::Gui::Button> texturePickerButtons;

	void RecreateTexturePickerButtons();

	void ResizeLayout() override;
};

