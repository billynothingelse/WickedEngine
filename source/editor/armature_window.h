#pragma once
class EditorComponent;

class ArmatureWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);
	void RefreshBoneList();

	Protean::Gui::Label infoLabel;
	Protean::Gui::Button resetPoseButton;
	Protean::Gui::Button createHumanoidButton;
	Protean::Gui::TreeList boneList;

	void ResizeLayout() override;
};

