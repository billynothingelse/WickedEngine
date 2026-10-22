#pragma once
class EditorComponent;

class RigidBodyWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	Protean::Entity driving_entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	void RefreshShapeType();

	Protean::Gui::ComboBox collisionShapeComboBox;
	Protean::Gui::Slider XSlider;
	Protean::Gui::Slider YSlider;
	Protean::Gui::Slider ZSlider;
	Protean::Gui::Slider massSlider;
	Protean::Gui::Slider frictionSlider;
	Protean::Gui::Slider restitutionSlider;
	Protean::Gui::Slider lineardampingSlider;
	Protean::Gui::Slider angulardampingSlider;
	Protean::Gui::Slider buoyancySlider;
	Protean::Gui::Slider physicsMeshLODSlider;
	Protean::Gui::CheckBox startDeactivatedCheckBox;
	Protean::Gui::CheckBox disabledeactivationCheckBox;
	Protean::Gui::CheckBox kinematicCheckBox;
	Protean::Gui::CheckBox locked2DCheckBox;
	Protean::Gui::Slider offsetXSlider;
	Protean::Gui::Slider offsetYSlider;
	Protean::Gui::Slider offsetZSlider;
	Protean::Gui::CheckBox physicsDebugCheckBox;

	Protean::Gui::Label characterLabel;
	Protean::Gui::CheckBox characterCheckBox;
	Protean::Gui::Slider characterSlopeSlider;
	Protean::Gui::Slider characterGravitySlider;

	Protean::Gui::Label vehicleLabel;
	Protean::Gui::ComboBox vehicleCombo;
	Protean::Gui::ComboBox vehicleCollisionCombo;
	Protean::Gui::Slider wheelRadiusSlider;
	Protean::Gui::Slider wheelWidthSlider;
	Protean::Gui::Slider chassisHalfWidthSlider;
	Protean::Gui::Slider chassisHalfHeightSlider;
	Protean::Gui::Slider chassisHalfLengthSlider;
	Protean::Gui::Slider frontWheelOffsetSlider;
	Protean::Gui::Slider rearWheelOffsetSlider;
	Protean::Gui::Slider maxTorqueSlider;
	Protean::Gui::Slider clutchStrengthSlider;
	Protean::Gui::Slider maxRollAngleSlider;
	Protean::Gui::Slider maxSteeringAngleSlider;

	Protean::Gui::Slider fSuspensionMinSlider;
	Protean::Gui::Slider fSuspensionMaxSlider;
	Protean::Gui::Slider fSuspensionFrequencySlider;
	Protean::Gui::Slider fSuspensionDampingSlider;

	Protean::Gui::Slider rSuspensionMinSlider;
	Protean::Gui::Slider rSuspensionMaxSlider;
	Protean::Gui::Slider rSuspensionFrequencySlider;
	Protean::Gui::Slider rSuspensionDampingSlider;

	Protean::Gui::Slider motorcycleFBrakeSuspensionAngleSlider;
	Protean::Gui::Slider motorcycleFBrakeTorqueSlider;
	Protean::Gui::Slider motorcycleRBrakeTorqueSlider;

	Protean::Gui::ComboBox wheelEntityFrontLeftCombo;
	Protean::Gui::ComboBox wheelEntityFrontRightCombo;
	Protean::Gui::ComboBox wheelEntityRearLeftCombo;
	Protean::Gui::ComboBox wheelEntityRearRightCombo;
	Protean::Gui::CheckBox fourwheelCheckbox;
	Protean::Gui::CheckBox motorleanCheckbox;
	Protean::Gui::CheckBox driveCheckbox;

	void ResizeLayout() override;
};

