#pragma once
#include "../../engine/protean.h"


class Example_ImGuiRenderer : public Protean::RenderPath3D
{
	Protean::Gui::Label label;
public:
	void Load() override;
	void Update(float dt) override;
	void ResizeLayout() override;
	void Render() const override;
};

class Example_ImGui : public Protean::Application
{
	Example_ImGuiRenderer renderer;

public:
	~Example_ImGui() override;
	void Initialize() override;
	void Compose(Protean::Graphics::CommandList cmd) override;
};

