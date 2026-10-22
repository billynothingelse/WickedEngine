#pragma once
class EditorComponent;

class HierarchyWindow : public Protean::Gui::Window
{
private:
	Protean::unordered_set<Protean::Entity> entities;
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::ComboBox parentCombo;

	void ResizeLayout() override;
};

