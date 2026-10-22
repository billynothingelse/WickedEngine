#pragma once
class EditorComponent;

class MeshWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	int subset = -1;
	void SetEntity(Protean::Entity entity, int subset);
	void UpdateRecenterInputs(XMFLOAT3 center);

	Protean::Gui::Label meshInfoLabel;
	Protean::Gui::Button subsetRemoveButton;
	Protean::Gui::Button subsetLastButton;
	Protean::Gui::ComboBox subsetComboBox;
	Protean::Gui::ComboBox subsetMaterialComboBox;
	Protean::Gui::CheckBox doubleSidedCheckBox;
	Protean::Gui::CheckBox doubleSidedShadowCheckBox;
	Protean::Gui::CheckBox bvhCheckBox;
	Protean::Gui::CheckBox quantizeCheckBox;
	Protean::Gui::Button impostorCreateButton;
	Protean::Gui::Slider impostorDistanceSlider;
	Protean::Gui::Slider tessellationFactorSlider;
	Protean::Gui::Button instanceSelectButton;
	Protean::Gui::Button flipCullingButton;
	Protean::Gui::Button flipNormalsButton;
	Protean::Gui::Button computeNormalsSmoothButton;
	Protean::Gui::Button computeNormalsHardButton;
	Protean::Gui::Button recenterButton;
	Protean::Gui::Button recenterToTopButton;
	Protean::Gui::Button recenterToBottomButton;
	Protean::Gui::TextInputField recenterToXInput;
	Protean::Gui::TextInputField recenterToYInput;
	Protean::Gui::TextInputField recenterToZInput;
	Protean::Gui::Button mergeButton;
	Protean::Gui::Button optimizeButton;
	Protean::Gui::Button exportHeaderButton;

	Protean::Gui::ComboBox morphTargetCombo;
	Protean::Gui::Slider morphTargetSlider;

	Protean::Gui::Button lodgenButton;
	Protean::Gui::Button lodDeleteButton;
	Protean::Gui::Slider lodCountSlider;
	Protean::Gui::Slider lodQualitySlider;
	Protean::Gui::Slider lodErrorSlider;
	Protean::Gui::CheckBox lodSloppyCheckBox;

	void ResizeLayout() override;
};

