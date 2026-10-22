#include "stdafx.h"
#include "script_window.h"

using namespace Protean;

void ScriptWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	Protean::Gui::Window::Create(ICON_SCRIPT " Script", Protean::Gui::Window::WindowControls_COLLAPSE | Protean::Gui::Window::WindowControls_CLOSE | Protean::Gui::Window::WindowControls_FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(520, 80));

	closeButton.SetTooltip("Delete Script");
	OnClose([=](Protean::Gui::EventArgs args) {

		Protean::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().scripts.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float hei = 20;
	float wid = 100;

	fileButton.Create("Open File...");
	fileButton.SetDescription("File: ");
	fileButton.SetSize(XMFLOAT2(wid, hei));
	fileButton.OnClick([=](Protean::Gui::EventArgs args) {
		Protean::Scene& scene = editor->GetCurrentScene();
		Protean::ScriptComponent* script = scene.scripts.GetComponent(entity);
		if (script == nullptr)
			return;

		if (script->resource.IsValid())
		{
			script->resource = {};
			script->filename = {};
			script->script = {};
			fileButton.SetText("Open File...");
		}
		else
		{
			Protean::Helper::FileDialogParams params;
			params.type = Protean::Helper::FileDialogParams::OPEN;
			params.description = "Lua Script (*.lua)";
			params.extensions = Protean::ResourceManager::GetSupportedScriptExtensions();
			Protean::Helper::FileDialog(params, [=](std::string fileName) {
				Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					script->CreateFromFile(fileName);
					fileButton.SetText(Protean::Helper::GetFileNameFromPath(fileName));
					});
				});
		}
	});
	AddWidget(&fileButton);

	playonceCheckBox.Create("Once: ");
	playonceCheckBox.SetTooltip("Play the script only one time, and stop immediately.\nUseful for having custom update frequency logic in the script.");
	playonceCheckBox.SetSize(XMFLOAT2(hei, hei));
	playonceCheckBox.OnClick([=](Protean::Gui::EventArgs args) {
		Protean::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ScriptComponent* script = scene.scripts.GetComponent(x.entity);
			if (script == nullptr)
				continue;
			script->SetPlayOnce(args.bValue);
		}
	});
	AddWidget(&playonceCheckBox);

	playstopButton.Create("");
	playstopButton.SetTooltip("Play / Stop script");
	playstopButton.SetSize(XMFLOAT2(wid, hei));
	playstopButton.OnClick([=](Protean::Gui::EventArgs args) {
		Protean::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			ScriptComponent* script = scene.scripts.GetComponent(x.entity);
			if (script == nullptr)
				continue;
			if (script->IsPlaying())
			{
				script->Stop();
			}
			else
			{
				script->Play();
			}
		}
	});
	AddWidget(&playstopButton);

	SetMinimized(true);
	SetVisible(false);
}

void ScriptWindow::SetEntity(Protean::Entity entity)
{
	this->entity = entity;

	Protean::Scene& scene = editor->GetCurrentScene();
	Protean::ScriptComponent* script = scene.scripts.GetComponent(entity);

	if (script != nullptr)
	{
		if (script->resource.IsValid())
		{
			fileButton.SetText(Protean::Helper::GetFileNameFromPath(script->filename));
		}
		else
		{
			fileButton.SetText("Open File...");
		}
		playonceCheckBox.SetCheck(script->IsPlayingOnlyOnce());
	}
	else
	{
		fileButton.SetText("Open File...");
	}
}

void ScriptWindow::Update(const Protean::Canvas& canvas, float dt)
{
	Protean::Scene& scene = editor->GetCurrentScene();
	Protean::ScriptComponent* script = scene.scripts.GetComponent(entity);
	if (script != nullptr)
	{
		if (script->IsPlaying())
		{
			playstopButton.SetText(ICON_STOP);
		}
		else
		{
			playstopButton.SetText(ICON_PLAY);
		}
	}

	Protean::Gui::Window::Update(canvas, dt);
}
void ScriptWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();

	fileButton.SetPos(XMFLOAT2(60, 4));
	fileButton.SetSize(XMFLOAT2(GetSize().x - 65, fileButton.GetSize().y));

	Protean::Scene& scene = editor->GetCurrentScene();
	Protean::ScriptComponent* script = scene.scripts.GetComponent(entity);
	if (script != nullptr && script->resource.IsValid())
	{
		playstopButton.SetVisible(true);
		playstopButton.SetPos(XMFLOAT2(84, fileButton.GetPos().y + fileButton.GetSize().y + 4));
		playstopButton.SetSize(XMFLOAT2(GetSize().x - 90, playstopButton.GetSize().y));

		playonceCheckBox.SetVisible(true);
		playonceCheckBox.SetPos(XMFLOAT2(playstopButton.GetPos().x - playonceCheckBox.GetSize().x - 4, playstopButton.GetPos().y));
	}
	else
	{
		playstopButton.SetVisible(false);
		playonceCheckBox.SetVisible(false);
	}
}
