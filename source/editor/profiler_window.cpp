#include "stdafx.h"
#include "profiler_window.h"

using namespace Protean::Graphics;

void ProfilerWidget::Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const
{
	if (!IsVisible())
	{
		Protean::Profiler::DisableDrawForThisFrame();
		Protean::Profiler::SetBackgroundColor(Protean::Color::fromFloat4(sprites[Protean::Gui::IDLE].params.color));
		return;
	}

	GraphicsDevice* device = GetDevice();

	ApplyScissor(canvas, scissorRect, cmd);

	Protean::Profiler::SetBackgroundColor(Protean::Color::Transparent());
	Protean::Profiler::SetTextColor(font.params.color);
	Protean::Profiler::DrawData(canvas, translation.x + 5, translation.y + 5, cmd);
}

void ProfilerWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	Window::Create("Profiler", Protean::Gui::Window::WindowControls_ALL);

	OnClose([=](Protean::Gui::EventArgs args) {
		Protean::Profiler::SetEnabled(false);
	});

	profilerWidget.SetSize(XMFLOAT2(200, 1000));
	AddWidget(&profilerWidget);

	auto size = XMFLOAT2(300, 400);
	if (editor->main->config.GetSection("layout").Has("profiler.width"))
	{
		size.x = editor->main->config.GetSection("layout").GetFloat("profiler.width");
	}
	if (editor->main->config.GetSection("layout").Has("profiler.height"))
	{
		size.y = editor->main->config.GetSection("layout").GetFloat("profiler.height");
	}
	SetSize(size);

	auto pos = XMFLOAT2(5, 100);
	if (editor->main->config.GetSection("layout").Has("profiler.x"))
	{
		pos.x = editor->main->config.GetSection("layout").GetFloat("profiler.x");
	}
	if (editor->main->config.GetSection("layout").Has("profiler.y"))
	{
		pos.y = editor->main->config.GetSection("layout").GetFloat("profiler.y");
	}
	SetPos(pos);
	SetVisible(false);
	SetShadowRadius(2);
}

void ProfilerWindow::Update(const Protean::Canvas& canvas, float dt)
{
	Protean::Gui::Window::Update(canvas, dt);

	const bool gui_round_enabled = !editor->generalWnd.disableRoundCornersCheckBox.GetCheck();
	for (int i = 0; i < arraysize(Protean::Gui::Widget::sprites); ++i)
	{
		if (gui_round_enabled)
		{
			sprites[i].params.enableCornerRounding();
			sprites[i].params.corners_rounding[0].radius = 10;
			sprites[i].params.corners_rounding[1].radius = 10;
			sprites[i].params.corners_rounding[2].radius = 10;
			sprites[i].params.corners_rounding[3].radius = 10;
		}
		else
		{
			sprites[i].params.disableCornerRounding();
		}
	}
}
void ProfilerWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();

	editor->main->config.GetSection("layout").Set("profiler.width", GetSize().x);
	editor->main->config.GetSection("layout").Set("profiler.height", GetSize().y);
	editor->main->config.GetSection("layout").Set("profiler.x", translation.x);
	editor->main->config.GetSection("layout").Set("profiler.y", translation.y);

	profilerWidget.SetSize(XMFLOAT2(GetWidgetAreaSize().x - control_size, profilerWidget.GetSize().y));
}
