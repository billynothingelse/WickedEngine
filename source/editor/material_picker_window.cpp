#include "stdafx.h"
#include "material_window.h"

using namespace Protean::Graphics;
using namespace Protean;
void MaterialPickerWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	Protean::Gui::Window::Create("Materials", Protean::Gui::Window::WindowControls_CLOSE | Protean::Gui::Window::WindowControls_RESIZE_RIGHT);
	SetText("Materials " ICON_MATERIALBROWSER);
	SetSize(XMFLOAT2(300, 400));

	zoomSlider.Create(10, 100, 100, 100 - 10, "Zoom: ");
	AddWidget(&zoomSlider);

	showMeshMaterialCheckBox.Create("Mesh materials: ");
	showMeshMaterialCheckBox.SetCheckText(ICON_EYE);
	showMeshMaterialCheckBox.SetCheck(true);
	showMeshMaterialCheckBox.OnClick([this](Protean::Gui::EventArgs args) {
		RecreateButtons();
	});
	AddWidget(&showMeshMaterialCheckBox);

	showInternalMaterialCheckBox.Create("Internal materials: ");
	showInternalMaterialCheckBox.SetCheckText(ICON_EYE);
	showInternalMaterialCheckBox.SetCheck(false);
	showInternalMaterialCheckBox.OnClick([this](Protean::Gui::EventArgs args) {
		RecreateButtons();
	});
	AddWidget(&showInternalMaterialCheckBox);

	SetVisible(false);
}

bool MaterialPickerWindow::MaterialIsUsedInMesh(const Protean::Entity materialEntity) const
{
	if (editor == nullptr)
		return false;

	const Protean::Scene& scene = editor->GetCurrentScene();

	// Check all meshes to see if any subset uses this material
	for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
	{
		const MeshComponent& mesh = scene.meshes[i];
		for (const auto& subset : mesh.subsets)
		{
			if (subset.materialID == materialEntity)
				return true;
		}
	}

	return false;
}

bool MaterialPickerWindow::ShouldShowMaterial(const Protean::MaterialComponent& material, const Protean::Entity entity) const
{
	const bool isInternal = material.IsInternal();

	// Check if we should hide internal materials
	if (!showInternalMaterialCheckBox.GetCheck() && isInternal)
		return false;

	// Check if we should hide materials used in meshes
	// Note: Internal materials are always shown if showInternalCheckBox is true, regardless of mesh usage filter
	if (!showMeshMaterialCheckBox.GetCheck() && !isInternal && MaterialIsUsedInMesh(entity))
		return false;

	return true;
}

void MaterialPickerWindow::RecreateButtons()
{
	if (editor == nullptr)
		return;
	const Protean::Scene& scene = editor->GetCurrentScene();
	for (auto& x : buttons)
	{
		RemoveWidget(&x);
	}
	buttons.resize(scene.materials.GetCount());

	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		const MaterialComponent& material = scene.materials[i];
		Entity entity = scene.materials.GetEntity(i);

		// Apply filters
		if (!ShouldShowMaterial(material, entity))
			continue;

		Protean::Gui::Button& button = buttons[i];
		button.Create("");
		AddWidget(&button);
		button.SetVisible(false);

		button.OnClick([entity, this](Protean::Gui::EventArgs args) {

			Protean::Archive& archive = editor->AdvanceHistory(true);
			archive << EditorComponent::HISTORYOP_SELECTION;
			// record PREVIOUS selection state...
			editor->RecordSelection(archive);

			if (!editor->translator.selected.empty() && Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT))
			{
				// Union selection:
				Protean::vector<Protean::PickResult> saved = editor->translator.selected;
				editor->translator.selected.clear();
				for (const Protean::PickResult& picked : saved)
				{
					editor->AddSelected(picked);
				}
				editor->AddSelected(entity);
			}
			else
			{
				// Replace selection:
				editor->translator.selected.clear();
				editor->AddSelected(entity);
			}

			// record NEW selection state...
			editor->RecordSelection(archive);
			});
	}

	ResizeLayout();
}

void MaterialPickerWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();

	if (editor == nullptr || IsCollapsed() || !IsVisible())
	{
		return;
	}

	const Protean::Scene& scene = editor->GetCurrentScene();
	if (buttons.size() != scene.materials.GetCount())
	{
		RecreateButtons();
		return;
	}

	layout.margin_left = 55;

	layout.add(zoomSlider);

	constexpr float hei = 20;
	showMeshMaterialCheckBox.SetSize(XMFLOAT2(hei, hei));
	layout.add_right(showMeshMaterialCheckBox);

	showInternalMaterialCheckBox.SetSize(XMFLOAT2(hei, hei));
	layout.add_right(showInternalMaterialCheckBox);

	Protean::Gui::Theme theme;
	theme.image.CopyFrom(sprites[Protean::Gui::IDLE].params);
	theme.image.background = false;
	theme.image.blendFlag = Protean::BLENDMODE_ALPHA;
	theme.font.CopyFrom(font.params);
	theme.shadow_color = Protean::Color::lerp(theme.font.color, Protean::Color::Transparent(), 0.25f);
	theme.tooltipFont.CopyFrom(tooltipFont.params);
	theme.tooltipImage.CopyFrom(tooltipSprite.params);

	const float preview_size = zoomSlider.GetValue();
	const float border = 20 * preview_size / 100.0f;
	const int cells = std::max(1, int(GetWidgetAreaSize().x / (preview_size + border)));
	float offset_y = showInternalMaterialCheckBox.GetPos().y + showInternalMaterialCheckBox.GetSize().y + border;
	int visible_button_index = 0;

	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		const MaterialComponent& material = scene.materials[i];
		const Entity entity = scene.materials.GetEntity(i);

		// Apply filters
		if (!ShouldShowMaterial(material, entity))
			continue;

		Protean::Gui::Button& button = buttons[i];
		button.SetVisible(IsVisible() && !IsCollapsed());

		button.SetTheme(theme);
		button.SetColor(Protean::Color::White());
		button.SetColor(Protean::Color(255, 255, 255, 150), Protean::Gui::FOCUS);
		button.SetShadowRadius(0);

		for (const auto& picked : editor->translator.selected)
		{
			if (picked.entity == entity)
			{
				button.SetColor(Protean::Color::White());
				button.SetShadowRadius(3);
				break;
			}
		}

		// find first good texture that we can show:
		button.SetImage({});
		for (auto& slot : material.textures)
		{
			if (slot.resource.IsValid())
			{
				button.SetImage(slot.resource);
				break;
			}
		}

		const NameComponent* name = scene.names.GetComponent(entity);
		if (name != nullptr)
		{
			if (preview_size >= 75)
			{
				button.SetText(name->name);
			}
			else
			{
				button.SetText("");
			}
			button.SetTooltip(name->name);
		}
		button.font.params.h_align = Protean::Font::WIFALIGN_CENTER;
		button.font.params.v_align = Protean::Font::WIFALIGN_BOTTOM;

		button.SetSize(XMFLOAT2(preview_size, preview_size));
		button.SetPos(XMFLOAT2((visible_button_index % cells) * (preview_size + border) + border, offset_y));
		if ((visible_button_index % cells) == (cells - 1))
		{
			offset_y += preview_size + border;
		}

		visible_button_index++;
	}
}
