#pragma once

class TestsRenderer : public Protean::RenderPath3D
{
	Protean::Gui::Label label;
	Protean::Gui::ComboBox testSelector;
	Protean::Entity ik_entity = Protean::INVALID_ENTITY;
public:
	void Load() override;
	void Update(float dt) override;
	void ResizeLayout() override;

	void RunJobSystemTest();
	void RunFontTest();
	void RunSpriteTest();
	void RunNetworkTest();
	void ContainerTest();
};

class Tests : public Protean::Application
{
	TestsRenderer renderer;
public:
	void Initialize() override;
};

