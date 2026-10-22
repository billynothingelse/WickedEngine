#pragma once
class EditorComponent;

class MaterialWindow;

class EmitterWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	void UpdateData();

	Protean::EmittedParticleSystem* GetEmitter();

	Protean::Gui::Button restartButton;
	Protean::Gui::Button burstButton;
	Protean::Gui::TextInputField burstCountInput;
	Protean::Gui::ComboBox meshComboBox;
	Protean::Gui::ComboBox shaderTypeComboBox;
	Protean::Gui::Label infoLabel;
	Protean::Gui::Slider maxParticlesSlider;
	Protean::Gui::CheckBox sortCheckBox;
	Protean::Gui::CheckBox depthCollisionsCheckBox;
	Protean::Gui::CheckBox sphCheckBox;
	Protean::Gui::CheckBox pauseCheckBox;
	Protean::Gui::CheckBox debugCheckBox;
	Protean::Gui::CheckBox volumeCheckBox;
	Protean::Gui::CheckBox frameBlendingCheckBox;
	Protean::Gui::CheckBox collidersDisabledCheckBox;
	Protean::Gui::CheckBox takeColorCheckBox;
	Protean::Gui::Slider emitCountSlider;
	Protean::Gui::Slider burstOnCreateSlider;
	Protean::Gui::Slider emitSizeSlider;
	Protean::Gui::Slider emitRotationSlider;
	Protean::Gui::Slider emitNormalSlider;
	Protean::Gui::Slider emitScalingSlider;
	Protean::Gui::Slider emitLifeSlider;
	Protean::Gui::Slider emitOpacityCurveStartSlider;
	Protean::Gui::Slider emitOpacityCurveEndSlider;
	Protean::Gui::Slider emitRandomnessSlider;
	Protean::Gui::Slider emitLifeRandomnessSlider;
	Protean::Gui::Slider emitColorRandomnessSlider;
	Protean::Gui::Slider emitMotionBlurSlider;
	Protean::Gui::Slider emitMassSlider;
	Protean::Gui::Slider timestepSlider;
	Protean::Gui::Slider dragSlider;
	Protean::Gui::Slider restitutionSlider;
	Protean::Gui::TextInputField VelocityXInput;
	Protean::Gui::TextInputField VelocityYInput;
	Protean::Gui::TextInputField VelocityZInput;
	Protean::Gui::TextInputField GravityXInput;
	Protean::Gui::TextInputField GravityYInput;
	Protean::Gui::TextInputField GravityZInput;

	Protean::Gui::Slider sph_h_Slider;
	Protean::Gui::Slider sph_K_Slider;
	Protean::Gui::Slider sph_p0_Slider;
	Protean::Gui::Slider sph_e_Slider;

	Protean::Gui::TextInputField frameRateInput;
	Protean::Gui::TextInputField framesXInput;
	Protean::Gui::TextInputField framesYInput;
	Protean::Gui::TextInputField frameCountInput;
	Protean::Gui::TextInputField frameStartInput;

	void ResizeLayout() override;
};

