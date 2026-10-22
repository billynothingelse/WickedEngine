#include "stdafx.h"
#include "NameWindow.h"

using namespace Protean;
void NameWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	Protean::Gui::Window::Create(ICON_NAME " Name", Protean::Gui::Window::WindowControls_COLLAPSE | Protean::Gui::Window::WindowControls_CLOSE | Protean::Gui::Window::WindowControls_FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(360, 60));

	closeButton.SetTooltip("Delete NameComponent");
	OnClose([=](Protean::Gui::EventArgs args) {

		Protean::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().names.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 0;
	float step = 25;
	float siz = 250;
	float hei = 20;

	nameInput.Create("");
	nameInput.SetDescription("Name: ");
	nameInput.SetPos(XMFLOAT2(x, y));
	nameInput.SetSize(XMFLOAT2(siz, hei));
	nameInput.OnInputAccepted([=](Protean::Gui::EventArgs args) {
		Protean::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			NameComponent* name = scene.names.GetComponent(x.entity);
			if (name == nullptr)
			{
				name = &editor->GetCurrentScene().names.Create(x.entity);
			}
			name->name = args.sValue;
		}
		editor->componentsWnd.RefreshEntityTree();
	});
	AddWidget(&nameInput);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void NameWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	if (entity != INVALID_ENTITY)
	{
		SetEnabled(true);

		NameComponent* name = editor->GetCurrentScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			nameInput.SetValue(name->name);
		}
	}
	else
	{
		SetEnabled(false);
		nameInput.SetValue("Select entity to modify name...");
	}
}

void NameWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();

	nameInput.SetPos(XMFLOAT2(60, 4));
	nameInput.SetSize(XMFLOAT2(GetSize().x - 65, nameInput.GetSize().y));
}
