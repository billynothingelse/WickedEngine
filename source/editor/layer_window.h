#pragma once
class EditorComponent;

class LayerWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label label;
	Protean::Gui::CheckBox layers[32];
	Protean::Gui::Button enableAllButton;
	Protean::Gui::Button enableNoneButton;

	void ResizeLayout() override;
};

