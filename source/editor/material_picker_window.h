#pragma once
class EditorComponent;

class MaterialPickerWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	std::deque<Protean::Gui::Button> buttons;

	Protean::Gui::Slider zoomSlider;
	Protean::Gui::CheckBox showMeshMaterialCheckBox;
	Protean::Gui::CheckBox showInternalMaterialCheckBox;

	void ResizeLayout() override;

	// Recreating buttons shouldn't be done every frame because interaction states need multi-frame execution
	void RecreateButtons();

private:
	bool ShouldShowMaterial(const Protean::MaterialComponent& material, Protean::Entity entity) const;
	bool MaterialIsUsedInMesh(Protean::Entity materialEntity) const;
};

