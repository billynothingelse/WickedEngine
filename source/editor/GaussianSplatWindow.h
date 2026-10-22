#pragma once

class EditorComponent;

class GaussianSplatWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;

	void ResizeLayout() override;
};

