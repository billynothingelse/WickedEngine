#pragma once
class EditorComponent;

class ProfilerWidget : public Protean::Gui::Widget
{
	void Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const override;
};

class ProfilerWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	ProfilerWidget profilerWidget;
	EditorComponent* editor = nullptr;

	void Update(const Protean::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};
