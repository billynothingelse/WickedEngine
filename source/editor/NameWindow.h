#pragma once
class EditorComponent;

class NameWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::TextInputField nameInput;

	void ResizeLayout() override;
};

