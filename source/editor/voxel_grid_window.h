#pragma once
class EditorComponent;

class VoxelGridWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::TextInputField dimXInput;
	Protean::Gui::TextInputField dimYInput;
	Protean::Gui::TextInputField dimZInput;
	Protean::Gui::Button	clearButton;
	Protean::Gui::Button	voxelizeObjectsButton;
	Protean::Gui::Button	voxelizeNavigationButton;
	Protean::Gui::Button	voxelizeCollidersButton;
	Protean::Gui::Button	floodfillButton;
	Protean::Gui::Button	fitToSceneButton;
	Protean::Gui::Button	generateMeshButton;
	Protean::Gui::Button	generateSimplifiedMeshButton;
	Protean::Gui::Button	generateNavMeshButton;
	Protean::Gui::CheckBox subtractCheckBox;
	Protean::Gui::CheckBox debugAllCheckBox;

	void ResizeLayout() override;
};

