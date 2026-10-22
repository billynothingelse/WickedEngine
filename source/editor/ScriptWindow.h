#pragma once
class EditorComponent;

class ScriptWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Button fileButton;
	Protean::Gui::CheckBox playonceCheckBox;
	Protean::Gui::Button playstopButton;

	void Update(const Protean::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};

