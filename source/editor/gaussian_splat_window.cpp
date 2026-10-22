#include "stdafx.h"
#include "gaussian_splat_window.h"

using namespace Protean;
void GaussianSplatWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	Protean::Gui::Window::Create(ICON_GAUSSIAN_SPLAT " Gaussian Splats", Protean::Gui::Window::WindowControls_COLLAPSE | Protean::Gui::Window::WindowControls_CLOSE | Protean::Gui::Window::WindowControls_FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(600, 1000));

	closeButton.SetTooltip("Delete Gaussian Splats");
	OnClose([=](Protean::Gui::EventArgs args) {

		Protean::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().gaussian_splats.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	infoLabel.Create("");
	infoLabel.SetFitTextEnabled(true);
	infoLabel.SetSize(XMFLOAT2(300, 200));
	AddWidget(&infoLabel);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(entity);
}

void GaussianSplatWindow::SetEntity(Entity entity)
{
	bool changed = this->entity != entity;
	this->entity = entity;

	const Protean::GaussianSplatModel* splat = editor->GetCurrentScene().gaussian_splats.GetComponent(entity);

	if (splat != nullptr)
	{
		std::string str = "Gaussian Splats are a point cloud where each point has size, orientation and view dependent colors via Spherical Harmonics (up to L3 degree). They can be used to render scenes without triangles, with baked lighting, transparency and reflections.\n";
		str += "\nSplat count: " + std::to_string(splat->GetSplatCount());
		str += "\nSpherical Harmonics degree: L" + std::to_string(splat->GetSphericalHarmonicsDegree());
		str += "\nMemory usage: " + Protean::Helper::GetMemorySizeText(splat->GetMemorySizeCPU());
		str += "\nGPU memory usage: " + Protean::Helper::GetMemorySizeText(splat->GetMemorySizeGPU());
		infoLabel.SetText(str);

		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}

}

void GaussianSplatWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();
	layout.margin_left = 100;

	layout.add_fullwidth(infoLabel);

}
