#include "stdafx.h"
#include "ContentBrowserWindow.h"

using namespace Protean;
void ContentBrowserWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	control_size = 30;

	Protean::Gui::Window::Create("Content Browser");

	RemoveWidget(&scrollbar_horizontal);

	sortingComboBox.Create("");
	sortingComboBox.AddItem("By recent", SORTING_RECENT);
	sortingComboBox.AddItem("By name", SORTING_NAME);
	sortingComboBox.AddItem("By path", SORTING_PATH);
	sortingComboBox.AddItem("By size", SORTING_SIZE);
	sortingComboBox.SetTooltip("Choose how items are sorted in the content browser.\nBy recent: Most recently used items first\nBy name: Alphabetically by file name\nBy path: Alphabetically by full path\nBy size: Largest files first");
	sortingComboBox.SetLocalizationEnabled(false);
	if (editor->main->config.GetSection("options").Has("content_browser_sorting"))
	{
		current_sorting = (SORTING)editor->main->config.GetSection("options").GetInt("content_browser_sorting");
	}
	sortingComboBox.SetSelected((int)current_sorting);
	sortingComboBox.OnSelect([this](Protean::Gui::EventArgs args) {
		current_sorting = (SORTING)args.iValue;
		editor->main->config.GetSection("options").Set("content_browser_sorting", args.iValue);
		editor->main->config.Commit();
		if (current_selection != SELECTION_COUNT)
		{
			SetSelection(current_selection);
		}
	});
	AddWidget(&sortingComboBox);

	SetVisible(false);
}

static constexpr float separator = 140; // width of the left selection folder buttons and sorting combobox
void ContentBrowserWindow::Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const
{
	Protean::Gui::Window::Render(canvas, cmd);

	if (IsVisible() && !IsCollapsed())
	{
		ApplyScissor(canvas, scissorRect, cmd);
		Protean::Image::Params params;
		params.pos = XMFLOAT3(translation.x + separator, translation.y + control_size, 0);
		params.siz = XMFLOAT2(2, scale.y - control_size);
		params.color = shadow_color;
		Protean::Image::Draw(nullptr, params, cmd);
	}
}

void ContentBrowserWindow::Update(const Protean::Canvas& canvas, float dt)
{
	Protean::Gui::Window::Update(canvas, dt);

	SetShadowRadius(6);

	const bool gui_round_enabled = !editor->generalWnd.disableRoundCornersCheckBox.GetCheck();
	constexpr float radius = 15;

	for (int i = 0; i < arraysize(Protean::Gui::Widget::sprites); ++i)
	{
		if (gui_round_enabled)
		{
			sprites[i].params.enableCornerRounding();
			sprites[i].params.corners_rounding[0].radius = radius;
			sprites[i].params.corners_rounding[1].radius = radius;
			sprites[i].params.corners_rounding[2].radius = radius;
			sprites[i].params.corners_rounding[3].radius = radius;

			openFolderButton.sprites[i].params.enableCornerRounding();
			openFolderButton.sprites[i].params.corners_rounding[0].radius = radius;
			openFolderButton.sprites[i].params.corners_rounding[1].radius = radius;
			openFolderButton.sprites[i].params.corners_rounding[2].radius = radius;
			openFolderButton.sprites[i].params.corners_rounding[3].radius = radius;
		}
		else
		{
			sprites[i].params.disableCornerRounding();
			openFolderButton.sprites[i].params.disableCornerRounding();
		}
	}

	for (auto& x : folderButtons)
	{
		x.font.params.h_align = Protean::Font::WIFALIGN_LEFT;
		for (auto& y : x.sprites)
		{
			if (gui_round_enabled)
			{
				y.params.enableCornerRounding();
				y.params.corners_rounding[3].radius = radius;
			}
			else
			{
				y.params.disableCornerRounding();
			}
		}
		x.SetShadowRadius(0);
	}
	for (auto& x : itemButtons)
	{
		if (x.sprites[Protean::Gui::IDLE].textureResource.IsValid())
		{
			x.sprites[Protean::Gui::IDLE].params.color = Protean::Color::White();
		}
		for (auto& y : x.sprites)
		{
			if (gui_round_enabled)
			{
				y.params.enableCornerRounding();
				y.params.corners_rounding[0].radius = radius;
				y.params.corners_rounding[1].radius = radius;
				y.params.corners_rounding[2].radius = radius;
				y.params.corners_rounding[3].radius = radius;
			}
			else
			{
				y.params.disableCornerRounding();
			}
		}
		x.SetShadowRadius(4);
	}

	openFolderButton.SetShadowRadius(0);

	const XMFLOAT4 color_on = sprites[Protean::Gui::FOCUS].params.color;
	const XMFLOAT4 color_off = sprites[Protean::Gui::IDLE].params.color;

	for (auto& x : folderButtons)
	{
		x.sprites[Protean::Gui::IDLE].params.color = color_off;
	}
	if (current_selection < SELECTION_COUNT)
	{
		folderButtons[current_selection].sprites[Protean::Gui::IDLE].params.color = color_on;
	}
}

void ContentBrowserWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();
	constexpr float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;

	sortingComboBox.Detach();
	sortingComboBox.SetPos(XMFLOAT2(translation.x + separator + 10, translation.y + control_size + padding));
	sortingComboBox.SetSize(XMFLOAT2(separator, 25));
	sortingComboBox.AttachTo(this);

	openFolderButton.Detach();
	openFolderButton.SetPos(XMFLOAT2(translation.x + padding, translation.y + scale.y - openFolderButton.GetSize().y - padding));
	openFolderButton.SetSize(XMFLOAT2(separator - padding * 2, openFolderButton.GetSize().y));
	openFolderButton.AttachTo(this);

	size_t seli = 0;
	for (auto& x : folderButtons)
	{
		x.Detach();
		x.SetPos(XMFLOAT2(translation.x + padding, translation.y + control_size + y));
		x.SetSize(XMFLOAT2(separator - padding * 2, x.GetSize().y));
		y += x.GetSize().y + padding;
		x.AttachTo(this);
		seli++;
		if (seli == SELECTION_RECENTFOLDER_BEGIN)
			y += x.GetSize().y + padding;
	}

	y = padding + 10 + 35;

	float sep = separator + 10;
	float hoffset = sep;
	for (auto& x : itemButtons)
	{
		x.SetPos(XMFLOAT2(hoffset, y));
		hoffset += x.GetSize().x + 15;
		if (hoffset + x.GetSize().x >= width)
		{
			hoffset = sep;
			y += x.GetSize().y + 40;
		}
	}
}

void ContentBrowserWindow::RefreshContent()
{
	content_folder = Protean::Helper::GetCurrentPath() + "/Content/";
	Protean::Helper::MakePathAbsolute(content_folder);
	if (!Protean::Helper::DirectoryExists(content_folder))
	{
		// Source tree layout: working directory is data/editor, content is in data/content
		content_folder = Protean::Helper::GetCurrentPath() + "/../content/";
		Protean::Helper::MakePathAbsolute(content_folder);
	}

	float hei = 25;
	float wid = 120;

	for (auto& x : folderButtons)
	{
		RemoveWidget(&x);
	}

	if (!editor->recentFilenames.empty())
	{
		Protean::Gui::Button& button = folderButtons[SELECTION_ALL];
		button.Create("All");
		button.SetTooltip("List all recently used files, not grouped by folders.");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](Protean::Gui::EventArgs args) {
			SetSelection(SELECTION_ALL);
			});
		AddWidget(&button, Protean::Gui::Window::AttachmentOptions::NONE);
		if (current_selection == SELECTION_COUNT)
		{
			current_selection = SELECTION_ALL;
		}
	}
	if (Protean::Helper::DirectoryExists(content_folder + "models"))
	{
		Protean::Gui::Button& button = folderButtons[SELECTION_MODELS];
		button.Create("Models");
		button.SetTooltip(content_folder + "models");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](Protean::Gui::EventArgs args) {
			SetSelection(SELECTION_MODELS);
			});
		AddWidget(&button, Protean::Gui::Window::AttachmentOptions::NONE);
		if (current_selection == SELECTION_COUNT)
		{
			current_selection = SELECTION_MODELS;
		}
	}
	if (Protean::Helper::DirectoryExists(content_folder + "scripts"))
	{
		Protean::Gui::Button& button = folderButtons[SELECTION_SCRIPTS];
		button.Create("Scripts");
		button.SetTooltip(content_folder + "scripts");
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this](Protean::Gui::EventArgs args) {
			SetSelection(SELECTION_SCRIPTS);
		});
		AddWidget(&button, Protean::Gui::Window::AttachmentOptions::NONE);
		if (current_selection == SELECTION_COUNT)
		{
			current_selection = SELECTION_SCRIPTS;
		}
	}

	for (size_t i = 0; i < editor->recentFolders.size(); ++i)
	{
		Protean::Gui::Button& button = folderButtons[SELECTION_RECENTFOLDER_BEGIN + i];
		std::string folder = editor->recentFolders[editor->recentFolders.size() - 1 - i];
		while (!folder.empty() && (folder.back() == '/' || folder.back() == '\\'))
		{
			folder.pop_back();
		}

		const auto last_slash = folder.find_last_of("/\\");
		if (last_slash != folder.npos) {
			folder = folder.substr(last_slash);
		}
		button.Create(folder);
		button.SetTooltip(editor->recentFolders[editor->recentFolders.size() - 1 - i]); // full folder name!
		button.SetLocalizationEnabled(false);
		button.SetSize(XMFLOAT2(wid, hei));
		button.OnClick([this, i](Protean::Gui::EventArgs args) {
			SetSelection((SELECTION)(SELECTION_RECENTFOLDER_BEGIN + i));
		});
		AddWidget(&button, Protean::Gui::Window::AttachmentOptions::NONE);
		if (current_selection == SELECTION_COUNT)
		{
			current_selection = (SELECTION)(SELECTION_RECENTFOLDER_BEGIN + i);
		}
	}

	if (current_selection != SELECTION_COUNT)
	{
		SetSelection(current_selection);
	}
}

void ContentBrowserWindow::SetSelection(SELECTION selection)
{
	Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [this, selection](uint64_t userdata) {
		current_selection = selection;
		for (auto& x : itemButtons)
		{
			RemoveWidget(&x);
		}
		itemButtons.clear();
		added_items.clear();

		RemoveWidget(&openFolderButton);
		openFolderButton.Create(ICON_OPEN " Go to location");
		openFolderButton.SetSize(XMFLOAT2(60, 60));

		switch (selection)
		{
		case SELECTION_SCRIPTS:
			AddItems(content_folder + "scripts/", "lua", ICON_SCRIPT);
			openFolderButton.OnClick([this](Protean::Gui::EventArgs args) {
				Protean::Helper::OpenUrl(content_folder + "scripts/");
				});
			AddWidget(&openFolderButton, Protean::Gui::Window::AttachmentOptions::NONE);
			break;
		case SELECTION_MODELS:
			AddItems(content_folder + "models/", "wiscene", ICON_OBJECT);
			AddItems(content_folder + "models/", "vrma", ICON_ANIMATION);
			AddItems(content_folder + "models/", "vrm", ICON_HUMANOID);
			AddItems(content_folder + "models/", "gltf", ICON_OBJECT);
			AddItems(content_folder + "models/", "glb", ICON_OBJECT);
			AddItems(content_folder + "models/", "fbx", ICON_OBJECT);
			AddItems(content_folder + "models/", "obj", ICON_OBJECT);
			AddItems(content_folder + "models/", "ply", ICON_OBJECT);
			openFolderButton.OnClick([this](Protean::Gui::EventArgs args) {
				Protean::Helper::OpenUrl(content_folder + "models/");
				});
			AddWidget(&openFolderButton, Protean::Gui::Window::AttachmentOptions::NONE);
			break;
		case SELECTION_ALL:
			for (size_t i = 0; i < editor->recentFilenames.size(); ++i)
			{
				const std::string& fileName = editor->recentFilenames[editor->recentFilenames.size() - 1 - i];
				std::string ext = Protean::Helper::toUpper(Protean::Helper::GetExtensionFromFileName(fileName));
				if (!ext.compare("LUA"))
				{
					AddItem(fileName, ICON_SCRIPT);
				}
				else if (!ext.compare("WISCENE"))
				{
					AddItem(fileName, ICON_OBJECT);
				}
				else if (!ext.compare("VRMA"))
				{
					AddItem(fileName, ICON_ANIMATION);
				}
				else if (!ext.compare("VRM"))
				{
					AddItem(fileName, ICON_HUMANOID);
				}
				else if (!ext.compare("GLTF"))
				{
					AddItem(fileName, ICON_OBJECT);
				}
				else if (!ext.compare("GLB"))
				{
					AddItem(fileName, ICON_OBJECT);
				}
				else if (!ext.compare("FBX"))
				{
					AddItem(fileName, ICON_OBJECT);
				}
				else if (!ext.compare("OBJ"))
				{
					AddItem(fileName, ICON_OBJECT);
				}
				else if (!ext.compare("PLY"))
				{
					AddItem(fileName, ICON_OBJECT);
				}
			}
			break;
		default:
			{
				size_t i = selection - SELECTION_RECENTFOLDER_BEGIN;
				i = std::min(i, editor->recentFolders.size() - 1);
				const std::string& folder = editor->recentFolders[editor->recentFolders.size() - 1 - i];
				AddItems(folder, "wiscene", ICON_OBJECT);
				AddItems(folder, "vrma", ICON_ANIMATION);
				AddItems(folder, "vrm", ICON_HUMANOID);
				AddItems(folder, "gltf", ICON_OBJECT);
				AddItems(folder, "glb", ICON_OBJECT);
				AddItems(folder, "fbx", ICON_OBJECT);
				AddItems(folder, "obj", ICON_OBJECT);
				AddItems(folder, "ply", ICON_OBJECT);
				AddItems(folder, "lua", ICON_SCRIPT);
				openFolderButton.OnClick([folder](Protean::Gui::EventArgs args) {
					Protean::Helper::OpenUrl(folder);
				});
				AddWidget(&openFolderButton, Protean::Gui::Window::AttachmentOptions::NONE);
			}
			break;
		}

		SortAndCreateItemButtons();

		for (auto& x : itemButtons)
		{
			AddWidget(&x);
		}

		editor->generalWnd.RefreshTheme();

	});
}

void ContentBrowserWindow::AddItems(const std::string& folder, const std::string& extension, const std::string& icon)
{
	// Folders parse:
	Protean::Helper::GetFolderNamesInDirectory(folder, [&](const std::string& folderName) {
		const std::string itemName = folderName.substr(folder.size()) + "." + extension;
		const std::string fileName = folderName + "/" + itemName;
		if (Protean::Helper::FileExists(fileName))
		{
			AddItem(fileName, icon);
		}
	});

	// Individual items:
	Protean::Helper::GetFileNamesInDirectory(folder, [&](const std::string& fileName) {
		AddItem(fileName, icon);
		}, extension);
}

void ContentBrowserWindow::AddItem(const std::string& fileName, const std::string& icon)
{
	if (!Protean::Helper::FileExists(fileName))
		return;

	for (const auto& item : added_items)
	{
		if (item.fileName == fileName)
			return;
	}

	ItemInfo info;
	info.fileName = fileName;
	info.icon = icon;
	info.file_size = Protean::Helper::FileSize(fileName);

	// Find index in recent filenames list (lower - more recent)
	info.recent_index = SIZE_MAX;
	for (size_t i = 0; i < editor->recentFilenames.size(); ++i)
	{
		// Check from the end (most recent) to beginning
		const size_t idx = editor->recentFilenames.size() - 1 - i;
		if (editor->recentFilenames[idx] == fileName)
		{
			info.recent_index = i;
			break;
		}
	}

	added_items.push_back(info);
}

void ContentBrowserWindow::SortAndCreateItemButtons()
{
	switch (current_sorting)
	{
	case SORTING_RECENT:
		std::stable_sort(added_items.begin(), added_items.end(), [](const ItemInfo& a, const ItemInfo& b) {
			return a.recent_index < b.recent_index;
		});
		break;
	case SORTING_NAME:
		std::sort(added_items.begin(), added_items.end(), [](const ItemInfo& a, const ItemInfo& b) {
			return Protean::Helper::toUpper(Protean::Helper::GetFileNameFromPath(a.fileName)) < Protean::Helper::toUpper(Protean::Helper::GetFileNameFromPath(b.fileName));
		});
		break;
	case SORTING_PATH:
		std::sort(added_items.begin(), added_items.end(), [](const ItemInfo& a, const ItemInfo& b) {
			return Protean::Helper::toUpper(a.fileName) < Protean::Helper::toUpper(b.fileName);
		});
		break;
	case SORTING_SIZE:
		std::sort(added_items.begin(), added_items.end(), [](const ItemInfo& a, const ItemInfo& b) {
			return a.file_size > b.file_size;
		});
		break;
	default:
		break;
	}

	for (const auto& item : added_items)
	{
		CreateItemButton(item.fileName, item.icon);
	}
}

void ContentBrowserWindow::CreateItemButton(const std::string& fileName, const std::string& icon)
{
	static constexpr auto siz = XMFLOAT2(240, 120);

	if (!Protean::Helper::FileExists(fileName))
		return;

	const std::string itemName = Protean::Helper::GetFileNameFromPath(fileName);
	const std::string folderName = Protean::Helper::GetDirectoryFromPath(fileName);
	const std::string extension = Protean::Helper::toUpper(Protean::Helper::GetExtensionFromFileName(fileName));

	Protean::Gui::Button& button = itemButtons.emplace_back();
	button.Create(icon);
	button.SetSize(siz);
	button.SetLocalizationEnabled(false);
	button.SetDescription(itemName);
	button.OnClick([this, fileName](Protean::Gui::EventArgs args) {
		Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [this, fileName](uint64_t userdata) {
			editor->Open(fileName);
			});
		this->SetVisible(false);
		});
	button.OnRightClick([this, fileName](Protean::Gui::EventArgs args) {
		Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [this, fileName](uint64_t userdata) {
			editor->NewScene();
			editor->Open(fileName);
			});
		this->SetVisible(false);
		});
	button.font_description.params.h_align = Protean::Font::WIFALIGN_CENTER;
	button.font_description.params.v_align = Protean::Font::WIFALIGN_TOP;
	button.font.params.size = 42;
	button.SetTooltip(fileName + "\nSize: " + Protean::Helper::GetMemorySizeText(Protean::Helper::FileSize(fileName)));
	if (extension.compare("WISCENE") == 0)
	{
		Protean::Archive::Header archive_header;
		Protean::Graphics::Texture archiveThumbnail = Protean::Archive::PeekThumbnail(fileName, &archive_header);
		if (archiveThumbnail.IsValid())
		{
			for (int i = 0; i < arraysize(sprites); ++i)
			{
				button.sprites[i].textureResource.SetTexture(archiveThumbnail);
			}
			button.SetText("");
		}
		button.SetTooltip(button.GetTooltip() + "\nVersion: " + std::to_string(archive_header.version) + (archive_header.properties.bits.compressed ? "\nCompressed : true" : "\nCompressed : false"));
	}
	else
	{
		std::string thumbnailName = folderName + "/thumbnail.png";
		if (Protean::Helper::FileExists(thumbnailName))
		{
			Protean::Resource thumbnail = Protean::ResourceManager::Load(thumbnailName);
			if (thumbnail.IsValid())
			{
				for (int i = 0; i < arraysize(sprites); ++i)
				{
					button.sprites[i].textureResource = thumbnail;
				}
				button.SetText("");
			}
		}
	}
	button.SetTooltip(button.GetTooltip() + "\n\nLeft-click: Merge into current scene\nRight-click: Open in new scene tab");
}
