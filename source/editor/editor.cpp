#include "stdafx.h"
#include "render/renderer.h"
#include "script/bindings/scene_bind_lua.h"

#include "model_importer.h"
#include "translator.h"
#include "dummy_visualizer.h"

// some application parameters can be overwritten in the executable by finding the 256 byte long pattern in the first member:
ApplicationExeCustomization exe_customization = {
	"Wicked Editor                                                                                                          ",
	Protean::Color(130, 210, 220, 255),
	Protean::Color(17, 30, 43, 255)
};

using namespace Protean::Graphics;
using namespace Protean;
enum FileType {
	FileType_INVALID,
	FileType_LUA,
	FileType_WISCENE,
	FileType_OBJ,
	FileType_GLTF,
	FileType_GLB,
	FileType_VRM,
	FileType_VRMA,
	FileType_FBX,
	FileType_PLY,
	FileType_IMAGE,
	FileType_VIDEO,
	FileType_SOUND,
	FileType_TEXT,
	FileType_HEADER,
	FileType_CPP,
};
static Protean::unordered_map<std::string, FileType> filetypes = {
	{"LUA", FileType_LUA},
	{"WISCENE", FileType_WISCENE},
	{"OBJ", FileType_OBJ},
	{"GLTF", FileType_GLTF},
	{"GLB", FileType_GLB},
	{"VRM", FileType_VRM},
	{"VRMA", FileType_VRMA},
	{"FBX", FileType_FBX},
	{"PLY", FileType_PLY},
	{"H", FileType_HEADER},
	{"CPP", FileType_CPP},
	{"TXT", FileType_TEXT},
};

enum EditorActions {
	// Camera movement
	EditorActions_MOVE_CAMERA_FORWARD,
	EditorActions_MOVE_CAMERA_BACKWARD,
	EditorActions_MOVE_CAMERA_LEFT,
	EditorActions_MOVE_CAMERA_RIGHT,
	EditorActions_MOVE_CAMERA_UP,
	EditorActions_MOVE_CAMERA_DOWN,

	// Entity actions
	EditorActions_DUPLICATE_ENTITY,

	// Selection actions
	EditorActions_SELECT_ALL_ENTITIES,
	EditorActions_DESELECT_ALL_ENTITIES,
	EditorActions_FOCUS_ON_SELECTION,
	EditorActions_RENAME_SELECTED,

	// Edit actions
	EditorActions_UNDO_ACTION,
	EditorActions_REDO_ACTION,
	EditorActions_COPY_ACTION,
	EditorActions_CUT_ACTION,
	EditorActions_PASTE_ACTION,
	EditorActions_DELETE_ACTION,

	// User actions
	EditorActions_MOVE_TOGGLE_ACTION,
	EditorActions_ROTATE_TOGGLE_ACTION,
	EditorActions_SCALE_TOGGLE_ACTION,
	EditorActions_LOCAL_GLOBAL_TOGGLE_ACTION,

	// Engine actions
	EditorActions_SCREENSHOT,
	EditorActions_SCREENSHOT_NOGUI,
	EditorActions_SCREENSHOT_ALPHA,
	EditorActions_SCREENSHOT_ALPHA_SELECTION,
	EditorActions_INSPECTOR_MODE,
	EditorActions_PLACE_INSTANCES,

	// Scene actions
	EditorActions_SAVE_SCENE_AS,
	EditorActions_SAVE_SCENE,

	// Transform actions
	EditorActions_ENABLE_TRANSFORM_TOOL,

	// View mode actions
	EditorActions_WIREFRAME_MODE,

	// Effect actions
	EditorActions_DEPTH_OF_FIELD_REFOCUS_TO_POINT,
	EditorActions_COLOR_GRADING_REFERENCE,

	// Other actions
	EditorActions_RAGDOLL_AND_PHYSICS_IMPULSE_TESTER,
	EditorActions_ORTHO_CAMERA,
	EditorActions_HIERARCHY_SELECT,
	EditorActions_ADD_TO_SPLINE,
	EditorActions_RELOAD_TERRAIN_PROPS,

	EditorActions_COUNT
};
struct HotkeyInfo
{
	Protean::Input::BUTTON button;
	bool press = false;
	bool control = false;
	bool shift = false;
};
HotkeyInfo hotkeyActions[size_t(EditorActions_COUNT)] = {
	{Protean::Input::BUTTON('W'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//MOVE_CAMERA_FORWARD,
	{Protean::Input::BUTTON('S'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//MOVE_CAMERA_BACKWARD,
	{Protean::Input::BUTTON('A'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//MOVE_CAMERA_LEFT,
	{Protean::Input::BUTTON('D'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//MOVE_CAMERA_RIGHT,
	{Protean::Input::BUTTON('E'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//MOVE_CAMERA_UP,
	{Protean::Input::BUTTON('Q'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//MOVE_CAMERA_DOWN,
	{Protean::Input::BUTTON('D'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//DUPLICATE_ENTITY,
	{Protean::Input::BUTTON('A'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//SELECT_ALL_ENTITIES,
	{Protean::Input::BUTTON::KEYBOARD_BUTTON_ESCAPE,	/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//DESELECT_ALL_ENTITIES,
	{Protean::Input::BUTTON('F'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//FOCUS_ON_SELECTION,
	{Protean::Input::BUTTON::KEYBOARD_BUTTON_F2,		/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//RENAME_SELECTED,
	{Protean::Input::BUTTON('Z'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//UNDO_ACTION,
	{Protean::Input::BUTTON('Y'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//REDO_ACTION,
	{Protean::Input::BUTTON('C'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//COPY_ACTION,
	{Protean::Input::BUTTON('X'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//CUT_ACTION,
	{Protean::Input::BUTTON('V'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//PASTE_ACTION,
	{Protean::Input::BUTTON::KEYBOARD_BUTTON_DELETE,	/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//DELETE_ACTION,
	{Protean::Input::BUTTON('1'),					/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//MOVE_TOGGLE_ACTION,
	{Protean::Input::BUTTON('2'),					/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//ROTATE_TOGGLE_ACTION,
	{Protean::Input::BUTTON('3'),					/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//SCALE_TOGGLE_ACTION,
	{Protean::Input::BUTTON('4'),					/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//LOCAL_GLOBAL_TOGGLE_ACTION,
	{Protean::Input::BUTTON::KEYBOARD_BUTTON_F3,		/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//SCREENSHOT,
	{Protean::Input::BUTTON::KEYBOARD_BUTTON_F3,		/*press=*/ true,		/*control=*/ false,		/*shift=*/ true},	//SCREENSHOT_NOGUI,
	{Protean::Input::BUTTON::KEYBOARD_BUTTON_F4,		/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//SCREENSHOT_ALPHA,
	{Protean::Input::BUTTON::KEYBOARD_BUTTON_F4,		/*press=*/ true,		/*control=*/ false,		/*shift=*/ true},	//SCREENSHOT_ALPHA_SELECTION,
	{Protean::Input::BUTTON('I'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//INSPECTOR_MODE,
	{Protean::Input::BUTTON::MOUSE_BUTTON_LEFT,		/*press=*/ true,		/*control=*/ true,		/*shift=*/ true},	//PLACE_INSTANCES,
	{Protean::Input::BUTTON('S'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ true},	//SAVE_SCENE_AS,
	{Protean::Input::BUTTON('S'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//SAVE_SCENE,
	{Protean::Input::BUTTON('T'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//ENABLE_TRANSFORM_TOOL,
	{Protean::Input::BUTTON('W'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//WIREFRAME_MODE,
	{Protean::Input::BUTTON('C'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//DEPTH_OF_FIELD_REFOCUS_TO_POINT,
	{Protean::Input::BUTTON('G'),					/*press=*/ false,		/*control=*/ true,		/*shift=*/ false},	//COLOR_GRADING_REFERENCE,
	{Protean::Input::BUTTON('P'),					/*press=*/ false,		/*control=*/ false,		/*shift=*/ false},	//RAGDOLL_AND_PHYSICS_IMPULSE_TESTER,
	{Protean::Input::BUTTON('O'),					/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//ORTHO_CAMERA,
	{Protean::Input::BUTTON('H'),					/*press=*/ true,		/*control=*/ false,		/*shift=*/ false},	//HIERARCHY_SELECT,
	{Protean::Input::BUTTON('E'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//ADD_TO_SPLINE,
	{Protean::Input::BUTTON('R'),					/*press=*/ true,		/*control=*/ true,		/*shift=*/ false},	//RELOAD_TERRAIN_PROPS,
};
static_assert(arraysize(hotkeyActions) == size_t(EditorActions_COUNT));
bool CheckInput(EditorActions action)
{
	const HotkeyInfo& hotkey = hotkeyActions[size_t(action)];
	bool ret = false;
	if (hotkey.press)
	{
		ret |= Protean::Input::Press(hotkey.button);
	}
	else
	{
		ret |= Protean::Input::Down(hotkey.button);
	}
	if (hotkey.control)
	{
		ret &= Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RCONTROL);
	}
	if (hotkey.shift)
	{
		ret &= Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RSHIFT);
	}
	return ret;
}
std::string GetInputString(EditorActions action)
{
	const HotkeyInfo& hotkey = hotkeyActions[size_t(action)];
	std::string ret = Protean::Input::ButtonToString(hotkey.button).chars;
	if (hotkey.shift)
	{
		ret = "Shift + " + ret;
	}
	if (hotkey.control)
	{
		ret = "Ctrl + " + ret;
	}
	return ret;
}
void HotkeyRemap(Editor* main)
{
	static const Protean::unordered_map<std::string, EditorActions> actionMap = {
		{"MOVE_CAMERA_FORWARD", EditorActions_MOVE_CAMERA_FORWARD},
		{"MOVE_CAMERA_BACKWARD", EditorActions_MOVE_CAMERA_BACKWARD},
		{"MOVE_CAMERA_LEFT", EditorActions_MOVE_CAMERA_LEFT},
		{"MOVE_CAMERA_RIGHT", EditorActions_MOVE_CAMERA_RIGHT},
		{"MOVE_CAMERA_UP", EditorActions_MOVE_CAMERA_UP},
		{"MOVE_CAMERA_DOWN", EditorActions_MOVE_CAMERA_DOWN},
		{"DUPLICATE_ENTITY", EditorActions_DUPLICATE_ENTITY},
		{"SELECT_ALL_ENTITIES", EditorActions_SELECT_ALL_ENTITIES},
		{"DESELECT_ALL_ENTITIES", EditorActions_DESELECT_ALL_ENTITIES},
		{"FOCUS_ON_SELECTION", EditorActions_FOCUS_ON_SELECTION},
		{"RENAME_SELECTED", EditorActions_RENAME_SELECTED},
		{"UNDO_ACTION", EditorActions_UNDO_ACTION},
		{"REDO_ACTION", EditorActions_REDO_ACTION},
		{"COPY_ACTION", EditorActions_COPY_ACTION},
		{"CUT_ACTION", EditorActions_CUT_ACTION},
		{"PASTE_ACTION", EditorActions_PASTE_ACTION},
		{"DELETE_ACTION", EditorActions_DELETE_ACTION},
		{"MOVE_TOGGLE_ACTION", EditorActions_MOVE_TOGGLE_ACTION},
		{"ROTATE_TOGGLE_ACTION", EditorActions_ROTATE_TOGGLE_ACTION},
		{"SCALE_TOGGLE_ACTION", EditorActions_SCALE_TOGGLE_ACTION},
		{"LOCAL_GLOBAL_TOGGLE_ACTION", EditorActions_LOCAL_GLOBAL_TOGGLE_ACTION},
		{"MAKE_NEW_SCREENSHOT", EditorActions_SCREENSHOT},
		{"MAKE_NEW_SCREENSHOT_NOGUI", EditorActions_SCREENSHOT_NOGUI},
		{"MAKE_NEW_SCREENSHOT_ALPHA", EditorActions_SCREENSHOT_ALPHA},
		{"MAKE_NEW_SCREENSHOT_ALPHA_SELECTION", EditorActions_SCREENSHOT_ALPHA_SELECTION},
		{"INSPECTOR_MODE", EditorActions_INSPECTOR_MODE},
		{"PLACE_INSTANCES", EditorActions_PLACE_INSTANCES},
		{"SAVE_SCENE_AS", EditorActions_SAVE_SCENE_AS},
		{"SAVE_SCENE", EditorActions_SAVE_SCENE},
		{"ENABLE_TRANSFORM_TOOL", EditorActions_ENABLE_TRANSFORM_TOOL},
		{"WIREFRAME_MODE", EditorActions_WIREFRAME_MODE},
		{"DEPTH_OF_FIELD_REFOCUS_TO_POINT", EditorActions_DEPTH_OF_FIELD_REFOCUS_TO_POINT},
		{"COLOR_GRADING_REFERENCE", EditorActions_COLOR_GRADING_REFERENCE},
		{"RAGDOLL_AND_PHYSICS_IMPULSE_TESTER", EditorActions_RAGDOLL_AND_PHYSICS_IMPULSE_TESTER},
		{"ORTHO_CAMERA", EditorActions_ORTHO_CAMERA},
		{"HIERARCHY_SELECT", EditorActions_HIERARCHY_SELECT},
		{"ADD_TO_SPLINE", EditorActions_ADD_TO_SPLINE},
		{"RELOAD_TERRAIN_PROPS", EditorActions_RELOAD_TERRAIN_PROPS},
	};
	static const Protean::unordered_map<std::string, Protean::Input::BUTTON> buttonMap = {
		{"ESC", Protean::Input::KEYBOARD_BUTTON_ESCAPE},
		{"INSERT", Protean::Input::KEYBOARD_BUTTON_INSERT},
		{"DELETE", Protean::Input::KEYBOARD_BUTTON_DELETE},
		{"HOME", Protean::Input::KEYBOARD_BUTTON_HOME},
		{"PAGEUP", Protean::Input::KEYBOARD_BUTTON_PAGEUP},
		{"PAGEDOWN", Protean::Input::KEYBOARD_BUTTON_PAGEDOWN},
		{"MOUSELEFT", Protean::Input::MOUSE_BUTTON_LEFT},
		{"MOUSERIGHT", Protean::Input::MOUSE_BUTTON_RIGHT},
		{"F1", Protean::Input::KEYBOARD_BUTTON_F1},
		{"F2", Protean::Input::KEYBOARD_BUTTON_F2},
		{"F3", Protean::Input::KEYBOARD_BUTTON_F3},
		{"F4", Protean::Input::KEYBOARD_BUTTON_F4},
		{"F5", Protean::Input::KEYBOARD_BUTTON_F5},
		{"F6", Protean::Input::KEYBOARD_BUTTON_F6},
		{"F7", Protean::Input::KEYBOARD_BUTTON_F7},
		{"F8", Protean::Input::KEYBOARD_BUTTON_F8},
		{"F9", Protean::Input::KEYBOARD_BUTTON_F9},
		{"F10", Protean::Input::KEYBOARD_BUTTON_F10},
		{"F11", Protean::Input::KEYBOARD_BUTTON_F11},
		{"F12", Protean::Input::KEYBOARD_BUTTON_F12},
		{"SPACE", Protean::Input::KEYBOARD_BUTTON_SPACE},
	};

	Protean::Config::Section hotkeyssection = main->config.GetSection("hotkeys");
	for (auto& x : hotkeyssection)
	{
		auto itAction = actionMap.find(Protean::Helper::toUpper(x.first));
		if (itAction == actionMap.end())
			continue;
		EditorActions action = itAction->second;
		std::string hotkeyString = Protean::Helper::toUpper(x.second);
		Protean::Input::BUTTON button = Protean::Input::BUTTON_NONE;

		// Find the main key from the whole hotkey string:
		std::string firstNonModifierKey;
		std::istringstream iss(hotkeyString);
		std::string token;
		while (std::getline(iss, token, '+'))
		{
			if (token != "CTRL" && token != "SHIFT" && !token.empty())
			{
				firstNonModifierKey = token;
			}
		}

		// Try to find the key in the map
		auto itButton = buttonMap.find(firstNonModifierKey);
		if (itButton != buttonMap.end())
		{
			button = itButton->second;
		}
		else
		{
			// Cast individual key to button
			if (firstNonModifierKey.length() == 1)
			{
				char c = firstNonModifierKey[0];
				if (std::isalnum(c))
				{
					button = Protean::Input::BUTTON(c);
				}
			}
		}

		// Remap hotkey if button is successfully found:
		if (button != Protean::Input::BUTTON_NONE)
		{
			hotkeyActions[size_t(action)] = HotkeyInfo{ button, hotkeyActions[size_t(action)].press, hotkeyString.find("CTRL") != std::string::npos, hotkeyString.find("SHIFT") != std::string::npos };
		}
	}
}

void Editor::Initialize()
{
	if (config.Has("font"))
	{
		// Replace default font from config before engine initialization:
		Protean::Font::AddFontStyle(config.GetText("font"));
	}

	Protean::Backlog::setFontColor(exe_customization.font_color);
	Protean::Backlog::setBackgroundColor(exe_customization.background_color);
	XMFLOAT4 clearcol = exe_customization.background_color;
	swapChain.desc.clear_color[0] = clearcol.x;
	swapChain.desc.clear_color[1] = clearcol.y;
	swapChain.desc.clear_color[2] = clearcol.z;

	Application::Initialize();

	infoDisplay.active = true;
	infoDisplay.watermark = false; // can be toggled instead on gui
	infoDisplay.pipeline_creation = true;
	//infoDisplay.fpsinfo = true;
	//infoDisplay.resolution = true;
	//infoDisplay.logical_size = true;
	//infoDisplay.colorspace = true;
	//infoDisplay.heap_allocation_counter = true;
	//infoDisplay.vram_usage = true;

	// Font icon is from #include "FontAwesomeV6.h"
	//	We will not directly use this font style, but let the font renderer fall back on it
	//	when an icon character is not found in the default font.
	Protean::Font::AddFontStyle("FontAwesomeV6", font_awesome_v6, font_awesome_v6_size);

	if (!IsScriptReplacement()) // we only activate editor functionality if this exe does not have a script replacement
	{
		renderComponent.main = this;
		uint32_t msaa = 4;
		if (config.Has("gui_antialiasing"))
		{
			msaa = std::max(1u, (uint32_t)config.GetInt("gui_antialiasing"));
		}
		renderComponent.setMSAASampleCount(msaa);
		renderComponent.Load();
		ActivatePath(&renderComponent, 0.5f, Protean::Color::Black(), Protean::FadeManager::FadeType_CrossFade);

		Protean::Lua::EnableEditorFunctionality(this, &renderComponent);

		auto ext_video = Protean::ResourceManager::GetSupportedVideoExtensions();
		for (auto& x : ext_video)
		{
			filetypes[x] = FileType_VIDEO;
		}
		auto ext_sound = Protean::ResourceManager::GetSupportedSoundExtensions();
		for (auto& x : ext_sound)
		{
			filetypes[x] = FileType_SOUND;
		}
		auto ext_image = Protean::ResourceManager::GetSupportedImageExtensions();
		for (auto& x : ext_image)
		{
			filetypes[x] = FileType_IMAGE;
		}
	}
}

bool Editor::KeepRunning() {
	return !exit_requested || renderComponent.save_in_progress;
}

void Editor::SaveWindowSize()
{
	if (window != nullptr)
	{
#ifdef _WIN32
		WINDOWPLACEMENT placement = {};
		placement.length = sizeof(WINDOWPLACEMENT);
		if (GetWindowPlacement(window, &placement))
		{
			if (placement.showCmd == SW_SHOWMAXIMIZED)
			{
				config.Set("window_maximized", true);
			}
			else
			{
				RECT rect;
				GetWindowRect(window, &rect);
				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;
				if (width > 0 && height > 0)
				{
					config.Set("width", width);
					config.Set("height", height);
				}
				config.Set("window_maximized", false);
			}
		}
#elif defined(SDL2) || defined(SDL3)
		if (SDL_GetWindowFlags(window) & SDL_WINDOW_MAXIMIZED)
		{
			config.Set("window_maximized", true);
		}
		else
		{
			int width = 0;
			int height = 0;
			SDL_GetWindowSize(window, &width, &height);
			if (width > 0 && height > 0)
			{
				config.Set("width", width);
				config.Set("height", height);
			}
			config.Set("window_maximized", false);
		}
#elif defined(__APPLE__)
		XMUINT2 size = Protean::Apple::GetWindowSizeNoScaling(window);
		config.Set("width", size.x);
		config.Set("height", size.y);
#endif
	}
}

void Editor::Exit()
{
	// Check all scenes for unsaved changes
	for (int i = 0; i < (int)renderComponent.scenes.size(); ++i)
	{
		if (!renderComponent.CheckUnsavedChanges(i))
		{
			return;
		}
	}

	// Save window size to config
	SaveWindowSize();

	// main loop will need to quit for us
	exit_requested = true;
}

void Editor::HotReload()
{
	if (IsScriptReplacement())
		return;
	if (!Protean::Initializer::IsInitializeFinished())
		return;

	static Protean::JobSystem::context hotreload_ctx;
	Protean::JobSystem::Wait(hotreload_ctx);
	hotreload_ctx.priority = Protean::JobSystem::Priority_Streaming;

	if (Protean::ShaderCompiler::GetRegisteredShaderCount() > 0 && !Protean::Renderer::IsPipelineCreationActive())
	{
		Protean::JobSystem::Execute(hotreload_ctx, [](Protean::JobSystem::JobArgs args) {
			Protean::Backlog::post("[Shader check] Started checking " + std::to_string(Protean::ShaderCompiler::GetRegisteredShaderCount()) + " registered shaders for changes...");
			if (Protean::ShaderCompiler::CheckRegisteredShadersOutdated())
			{
				Protean::Backlog::post("[Shader check] Changes detected, initiating reload...");
				Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [](uint64_t userdata) {
					Protean::Renderer::ReloadShaders();
				});
			}
			else
			{
				Protean::Backlog::post("[Shader check] All up to date");
			}
		});
	}

	Protean::JobSystem::Execute(hotreload_ctx, [](Protean::JobSystem::JobArgs args) {
		Protean::Backlog::post("[Resource check] Started checking resource manager for changes...");
		if (Protean::ResourceManager::CheckResourcesOutdated())
		{
			Protean::Backlog::post("[Resource check] Changes detected, initiating reload...");
			Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [](uint64_t userdata) {
				Protean::ResourceManager::ReloadOutdatedResources();
			});
		}
		else
		{
			Protean::Backlog::post("[Resource check] All up to date");
		}
	});

	renderComponent.ReloadLanguage();
}

void EditorComponent::ResizeBuffers()
{
	graphicsWnd.UpdateSwapChainFormats(&main->swapChain);

	init(main->canvas);
	RenderPath2D::ResizeBuffers();

	renderPath->width = 0; // force resize buffers
	renderPath->height = 0;// force resize buffers
	ResizeViewport3D();

	TextureDesc desc;
	desc.width = GetPhysicalWidth();
	desc.height = GetPhysicalHeight();
	desc.format = Format_R10G10B10A2_UNORM;
	desc.bind_flags = BindFlag_RENDER_TARGET | BindFlag_SHADER_RESOURCE;
	GetDevice()->CreateTexture(&desc, nullptr, &gui_background_effect);
	GetDevice()->SetName(&gui_background_effect, "editor.gui_background_effect");
}
void EditorComponent::ResizeLayout()
{
	RenderPath2D::ResizeLayout();

	// GUI elements scaling:

	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();

	topmenuWnd.SetSize(XMFLOAT2(screenW, 30));
	topmenuWnd.SetPos(XMFLOAT2(0, 0));

	componentsWnd.SetSize(XMFLOAT2(componentsWnd.GetSize().x, screenH - topmenuWnd.GetSize().y));
	componentsWnd.SetPos(XMFLOAT2(screenW - componentsWnd.scale_local.x, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));

	aboutWindow.SetSize(XMFLOAT2(screenW / 2.0f, screenH / 1.5f));
	aboutWindow.SetPos(XMFLOAT2(screenW / 2.0f - aboutWindow.scale.x / 2.0f, screenH / 2.0f - aboutWindow.scale.y / 2.0f));

	contentBrowserWnd.SetSize(XMFLOAT2(screenW / 1.6f, screenH / 1.2f));
	contentBrowserWnd.SetPos(XMFLOAT2(screenW / 2.0f - contentBrowserWnd.scale.x / 2.0f, screenH / 2.0f - contentBrowserWnd.scale.y / 2.0f));

	projectCreatorWnd.SetSize(XMFLOAT2(projectCreatorWnd.backgroundColorPicker.GetSize().x * 2 + 4 * 3, std::min(780.0f, screenH * 0.8f)));
	projectCreatorWnd.SetPos(XMFLOAT2(screenW / 2.0f - projectCreatorWnd.scale.x / 2.0f, screenH / 2.0f - projectCreatorWnd.scale.y / 2.0f));

	themeEditorWnd.SetSize(XMFLOAT2(740, std::min(780.0f, screenH * 0.8f)));
	themeEditorWnd.SetPos(XMFLOAT2(screenW / 2.0f - themeEditorWnd.scale.x / 2.0f, screenH / 2.0f - themeEditorWnd.scale.y / 2.0f));

}
void EditorComponent::Load()
{
	Protean::Timer timer;

	is_2D_mode = main->config.GetBool("2D_mode");

	//Load hotkeys here
	HotkeyRemap(main);

	Protean::Gui::CheckBox::SetCheckTextGlobal(ICON_CHECK);

	loadmodel_workload.priority = Protean::JobSystem::Priority_Low;

	loadmodel_font.SetText("Loading model...");
	loadmodel_font.anim.typewriter.time = 2;
	loadmodel_font.anim.typewriter.looped = true;
	loadmodel_font.anim.typewriter.character_start = 13;
	loadmodel_font.params.size = 22;
	loadmodel_font.params.h_align = Protean::Font::WIFALIGN_LEFT;
	loadmodel_font.params.v_align = Protean::Font::WIFALIGN_TOP;
	AddFont(&loadmodel_font);

	terrain_generation_font.SetText("Generating terrain...");
	terrain_generation_font.anim.typewriter.time = 2;
	terrain_generation_font.anim.typewriter.looped = true;
	terrain_generation_font.anim.typewriter.character_start = 19;
	terrain_generation_font.params.size = 22;
	terrain_generation_font.params.h_align = Protean::Font::WIFALIGN_LEFT;
	terrain_generation_font.params.v_align = Protean::Font::WIFALIGN_TOP;
	AddFont(&terrain_generation_font);

	topmenuWnd.Create("", Protean::Gui::Window::WindowControls_NONE);
	topmenuWnd.SetShadowRadius(2);
	GetGUI().AddWidget(&topmenuWnd);
	topmenuWnd.scrollbar_horizontal.Detach();
	topmenuWnd.scrollbar_vertical.Detach();

	generalButton.Create(ICON_GENERALOPTIONS);
	generalButton.OnClick([this](Protean::Gui::EventArgs args) {
		generalWnd.SetVisible(!generalWnd.IsVisible());
		graphicsWnd.SetVisible(false);
		cameraWnd.SetVisible(false);
		materialPickerWnd.SetVisible(false);
		paintToolWnd.SetVisible(false);
	});
	generalButton.SetShadowRadius(0);
	generalButton.SetTooltip("General options");
	generalButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	GetGUI().AddWidget(&generalButton);

	graphicsButton.Create(ICON_GRAPHICSOPTIONS);
	graphicsButton.OnClick([this](Protean::Gui::EventArgs args) {
		generalWnd.SetVisible(false);
		graphicsWnd.SetVisible(!graphicsWnd.IsVisible());
		cameraWnd.SetVisible(false);
		materialPickerWnd.SetVisible(false);
		paintToolWnd.SetVisible(false);
	});
	graphicsButton.SetShadowRadius(0);
	graphicsButton.SetTooltip("Graphics options");
	graphicsButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	GetGUI().AddWidget(&graphicsButton);

	cameraButton.Create(ICON_CAMERAOPTIONS);
	cameraButton.OnClick([this](Protean::Gui::EventArgs args) {
		generalWnd.SetVisible(false);
		graphicsWnd.SetVisible(false);
		cameraWnd.SetVisible(!cameraWnd.IsVisible());
		materialPickerWnd.SetVisible(false);
		paintToolWnd.SetVisible(false);
	});
	cameraButton.SetShadowRadius(0);
	cameraButton.SetTooltip("Camera options");
	cameraButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	GetGUI().AddWidget(&cameraButton);

	materialsButton.Create(ICON_MATERIALBROWSER);
	materialsButton.OnClick([this](Protean::Gui::EventArgs args) {
		generalWnd.SetVisible(false);
		graphicsWnd.SetVisible(false);
		cameraWnd.SetVisible(false);
		materialPickerWnd.SetVisible(!materialPickerWnd.IsVisible());
		paintToolWnd.SetVisible(false);
		if (materialPickerWnd.IsVisible())
		{
			materialPickerWnd.RecreateButtons();
		}
	});
	materialsButton.SetShadowRadius(0);
	materialsButton.SetTooltip("Material browser");
	materialsButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	GetGUI().AddWidget(&materialsButton);

	paintToolButton.Create(ICON_PAINTTOOL);
	paintToolButton.OnClick([this](Protean::Gui::EventArgs args) {
		generalWnd.SetVisible(false);
		graphicsWnd.SetVisible(false);
		cameraWnd.SetVisible(false);
		materialPickerWnd.SetVisible(false);
		paintToolWnd.SetVisible(!paintToolWnd.IsVisible());
	});
	paintToolButton.SetShadowRadius(0);
	paintToolButton.SetTooltip("Paint tool");
	paintToolButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	GetGUI().AddWidget(&paintToolButton);

	graphicsWnd.Create(this);
	GetGUI().AddWidget(&graphicsWnd);

	cameraWnd.Create(this);
	GetGUI().AddWidget(&cameraWnd);

	paintToolWnd.Create(this);
	GetGUI().AddWidget(&paintToolWnd);

	materialPickerWnd.Create(this);
	GetGUI().AddWidget(&materialPickerWnd);

	generalWnd.Create(this);
	GetGUI().AddWidget(&generalWnd);

	newSceneButton.Create("+");
	newSceneButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	newSceneButton.SetTooltip("New scene");
	newSceneButton.OnClick([this](Protean::Gui::EventArgs args) {
		NewScene();
		});
	topmenuWnd.AddWidget(&newSceneButton);

	enum NEW_THING
	{
		NEW_TRANSFORM,
		NEW_MATERIAL,
		NEW_POINTLIGHT,
		NEW_SPOTLIGHT,
		NEW_RECTLIGHT,
		NEW_DIRECTIONALLIGHT,
		NEW_ENVIRONMENTPROBE,
		NEW_FORCE,
		NEW_DECAL,
		NEW_SOUND,
		NEW_VIDEO,
		NEW_WEATHER,
		NEW_EMITTER,
		NEW_HAIR,
		NEW_CAMERA,
		NEW_CUBE,
		NEW_PLANE,
		NEW_SPHERE,
		NEW_ANIMATION,
		NEW_SCRIPT,
		NEW_COLLIDER,
		NEW_TERRAIN,
		NEW_SPRITE,
		NEW_FONT,
		NEW_VOXELGRID,
		NEW_METADATA,
		NEW_CONSTRAINT,
		NEW_SPLINE,
	};

	newEntityCombo.Create("New: ");
	newEntityCombo.SetShadowRadius(0);
	newEntityCombo.SetInvalidSelectionText("+");
	newEntityCombo.selected_font.params.size = 28;
	newEntityCombo.SetDropArrowEnabled(false);
	newEntityCombo.SetFixedDropWidth(200);
	newEntityCombo.SetMaxVisibleItemCount(16);
	newEntityCombo.AddItem("Transform " ICON_TRANSFORM, NEW_TRANSFORM);
	newEntityCombo.AddItem("Material " ICON_MATERIAL, NEW_MATERIAL);
	newEntityCombo.AddItem("Point Light " ICON_POINTLIGHT, NEW_POINTLIGHT);
	newEntityCombo.AddItem("Rectangle Light " ICON_RECTLIGHT, NEW_RECTLIGHT);
	newEntityCombo.AddItem("Spot Light " ICON_SPOTLIGHT, NEW_SPOTLIGHT);
	newEntityCombo.AddItem("Directional Light " ICON_DIRECTIONALLIGHT, NEW_DIRECTIONALLIGHT);
	newEntityCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, NEW_ENVIRONMENTPROBE);
	newEntityCombo.AddItem("Force " ICON_FORCE, NEW_FORCE);
	newEntityCombo.AddItem("Decal " ICON_DECAL, NEW_DECAL);
	newEntityCombo.AddItem("Sound " ICON_SOUND, NEW_SOUND);
	newEntityCombo.AddItem("Video " ICON_VIDEO, NEW_VIDEO);
	newEntityCombo.AddItem("Weather " ICON_WEATHER, NEW_WEATHER);
	newEntityCombo.AddItem("Emitter " ICON_EMITTER, NEW_EMITTER);
	newEntityCombo.AddItem("HairParticle " ICON_HAIR, NEW_HAIR);
	newEntityCombo.AddItem("Camera " ICON_CAMERA, NEW_CAMERA);
	newEntityCombo.AddItem("Cube " ICON_CUBE, NEW_CUBE);
	newEntityCombo.AddItem("Plane " ICON_SQUARE, NEW_PLANE);
	newEntityCombo.AddItem("Sphere " ICON_CIRCLE, NEW_SPHERE);
	newEntityCombo.AddItem("Animation " ICON_ANIMATION, NEW_ANIMATION);
	newEntityCombo.AddItem("Script " ICON_SCRIPT, NEW_SCRIPT);
	newEntityCombo.AddItem("Collider " ICON_COLLIDER, NEW_COLLIDER);
	newEntityCombo.AddItem("Terrain " ICON_TERRAIN, NEW_TERRAIN);
	newEntityCombo.AddItem("Sprite " ICON_SPRITE, NEW_SPRITE);
	newEntityCombo.AddItem("Font " ICON_FONT, NEW_FONT);
	newEntityCombo.AddItem("Voxel Grid " ICON_VOXELGRID, NEW_VOXELGRID);
	newEntityCombo.AddItem("Metadata " ICON_METADATA, NEW_METADATA);
	newEntityCombo.AddItem("Constraint " ICON_CONSTRAINT, NEW_CONSTRAINT);
	newEntityCombo.AddItem("Spline " ICON_SPLINE, NEW_SPLINE);
	newEntityCombo.OnSelect([this](Protean::Gui::EventArgs args) {
		newEntityCombo.SetSelectedWithoutCallback(-1);
		const EditorComponent::EditorScene& editorscene = GetCurrentEditorScene();
		const CameraComponent& camera = editorscene.camera;
		Scene& scene = GetCurrentScene();
		PickResult pick;

		XMFLOAT3 in_front_of_camera = GetPositionInFrontOfCamera();

		switch (args.userdata)
		{
		case NEW_TRANSFORM:
			pick.entity = scene.Entity_CreateTransform("transform");
			break;
		case NEW_MATERIAL:
			pick.entity = scene.Entity_CreateMaterial("material");
			break;
		case NEW_POINTLIGHT:
			pick.entity = scene.Entity_CreateLight("pointlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::POINT;
			scene.lights.GetComponent(pick.entity)->intensity = 20;
			break;
		case NEW_RECTLIGHT:
			pick.entity = scene.Entity_CreateLight("rectlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::RECTANGLE;
			scene.lights.GetComponent(pick.entity)->intensity = 20;
			scene.lights.GetComponent(pick.entity)->length = 2;
			scene.lights.GetComponent(pick.entity)->height = 2;
			scene.lights.GetComponent(pick.entity)->SetVisualizerEnabled(true);
			break;
		case NEW_SPOTLIGHT:
			pick.entity = scene.Entity_CreateLight("spotlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::SPOT;
			scene.lights.GetComponent(pick.entity)->intensity = 100;
			break;
		case NEW_DIRECTIONALLIGHT:
			pick.entity = scene.Entity_CreateLight("dirlight", XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::DIRECTIONAL;
			scene.lights.GetComponent(pick.entity)->intensity = 10;
			break;
		case NEW_ENVIRONMENTPROBE:
			pick.entity = scene.Entity_CreateEnvironmentProbe("envprobe", in_front_of_camera);
			break;
		case NEW_FORCE:
			pick.entity = scene.Entity_CreateForce("force");
			break;
		case NEW_DECAL:
			pick.entity = scene.Entity_CreateDecal("decal", "");
			if (scene.materials.Contains(pick.entity))
			{
				MaterialComponent* decal_material = scene.materials.GetComponent(pick.entity);
				decal_material->textures[MaterialComponent::BASECOLORMAP].resource.SetTexture(*Protean::TextureHelper::getLogo());
			}
			scene.transforms.GetComponent(pick.entity)->RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
			break;
		case NEW_SOUND:
		{
			const uint64_t target_scene_id = GetCurrentEditorScene().id;
			Protean::Helper::FileDialogParams params;
			params.type = Protean::Helper::FileDialogParams::OPEN;
			params.description = "Sound";
			params.extensions = Protean::ResourceManager::GetSupportedSoundExtensions();
			Protean::Helper::FileDialog(params, [=](const std::string& fileName) {
				Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					EditorScene& target_editorscene = GetOrCreateEditorSceneForLoading(target_scene_id);
					const Entity entity = target_editorscene.scene.Entity_CreateSound(Protean::Helper::GetFileNameFromPath(fileName), fileName);

					Protean::Archive& archive = AdvanceHistory();
					archive << EditorComponent::HISTORYOP_ADD;
					RecordSelection(archive);

					ClearSelected();
					AddSelected(entity);

					RecordSelection(archive);
					RecordEntity(archive, entity);

					componentsWnd.RefreshEntityTree();
					componentsWnd.soundWnd.SetEntity(entity);
					});
				});
			return;
		}
		break;
		case NEW_VIDEO:
		{
			const uint64_t target_scene_id = GetCurrentEditorScene().id;
			Protean::Helper::FileDialogParams params;
			params.type = Protean::Helper::FileDialogParams::OPEN;
			params.description = "Video";
			params.extensions = Protean::ResourceManager::GetSupportedVideoExtensions();
			Protean::Helper::FileDialog(params, [=](const std::string& fileName) {
				Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					EditorScene& target_editorscene = GetOrCreateEditorSceneForLoading(target_scene_id);
					const Entity entity = target_editorscene.scene.Entity_CreateVideo(Protean::Helper::GetFileNameFromPath(fileName), fileName);

					Protean::Archive& archive = AdvanceHistory();
					archive << EditorComponent::HISTORYOP_ADD;
					RecordSelection(archive);

					ClearSelected();
					AddSelected(entity);

					RecordSelection(archive);
					RecordEntity(archive, entity);

					componentsWnd.RefreshEntityTree();
					componentsWnd.videoWnd.SetEntity(entity);
					});
				});
			return;
		}
		break;
		case NEW_WEATHER:
			pick.entity = CreateEntity();
			scene.weathers.Create(pick.entity);
			scene.names.Create(pick.entity) = "weather";
			break;
		case NEW_EMITTER:
			pick.entity = scene.Entity_CreateEmitter("emitter");
			break;
		case NEW_HAIR:
			pick.entity = scene.Entity_CreateHair("hair");
			break;
		case NEW_CAMERA:
			pick.entity = scene.Entity_CreateCamera("camera", camera.width, camera.height);
			*scene.cameras.GetComponent(pick.entity) = camera;
			*scene.transforms.GetComponent(pick.entity) = editorscene.camera_transform;
			break;
		case NEW_CUBE:
			pick.entity = scene.Entity_CreateCube("cube");
			pick.subsetIndex = 0;
			break;
		case NEW_PLANE:
			pick.entity = scene.Entity_CreatePlane("plane");
			pick.subsetIndex = 0;
			break;
		case NEW_SPHERE:
			pick.entity = scene.Entity_CreateSphere("sphere");
			pick.subsetIndex = 0;
			break;
		case NEW_ANIMATION:
			pick.entity = CreateEntity();
			scene.animations.Create(pick.entity);
			scene.names.Create(pick.entity) = "animation";
			break;
		case NEW_SCRIPT:
			pick.entity = CreateEntity();
			scene.scripts.Create(pick.entity);
			scene.names.Create(pick.entity) = "script";
			break;
		case NEW_COLLIDER:
			pick.entity = CreateEntity();
			scene.colliders.Create(pick.entity);
			scene.transforms.Create(pick.entity);
			scene.names.Create(pick.entity) = "collider";
			break;
		case NEW_TERRAIN:
			componentsWnd.terrainWnd.SetupAssets();
			pick.entity = componentsWnd.terrainWnd.entity;
			main->CrossFade(1);
			break;
		case NEW_SPRITE:
		{
			pick.entity = CreateEntity();
			Protean::Sprite& sprite = scene.sprites.Create(pick.entity);
			sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite.anim.repeatable = true;
			scene.transforms.Create(pick.entity).Translate(XMFLOAT3(0, 2, 0));
			scene.names.Create(pick.entity) = "sprite";
		}
		break;
		case NEW_FONT:
		{
			pick.entity = CreateEntity();
			Protean::SpriteFont& font = scene.fonts.Create(pick.entity);
			font.SetText("Text");
			font.params.h_align = Protean::Font::Alignment::WIFALIGN_CENTER;
			font.params.v_align = Protean::Font::Alignment::WIFALIGN_CENTER;
			font.params.scaling = 0.1f;
			font.params.size = 26;
			font.anim.typewriter.looped = true;
			scene.transforms.Create(pick.entity).Translate(XMFLOAT3(0, 2, 0));
			scene.names.Create(pick.entity) = "font";
		}
		break;
		case NEW_VOXELGRID:
		{
			pick.entity = CreateEntity();
			scene.voxel_grids.Create(pick.entity).init(64, 64, 64);
			scene.transforms.Create(pick.entity).Scale(XMFLOAT3(0.25f, 0.25f, 0.25f));
			scene.names.Create(pick.entity) = "voxelgrid";
		}
		break;
		case NEW_METADATA:
		{
			pick.entity = CreateEntity();
			scene.metadatas.Create(pick.entity);
			scene.transforms.Create(pick.entity);
			scene.names.Create(pick.entity) = "metadata";
		}
		break;
		case NEW_CONSTRAINT:
		{
			pick.entity = CreateEntity();
			scene.constraints.Create(pick.entity);
			scene.transforms.Create(pick.entity);
			scene.names.Create(pick.entity) = "constraint";
		}
		break;
		case NEW_SPLINE:
		{
			pick.entity = CreateEntity();
			scene.splines.Create(pick.entity);
			scene.transforms.Create(pick.entity);
			scene.names.Create(pick.entity) = "spline";
		}
		break;
		default:
			break;
		}
		if (pick.entity != INVALID_ENTITY)
		{
			// Place the entity in front of camera, if enabled
			if (generalWnd.placeInFrontOfCameraCheckBox.GetCheck())
			{
				TransformComponent* transform = scene.transforms.GetComponent(pick.entity);
				if (transform != nullptr)
				{
					// Check if this entity type should spawn in front of camera
					bool should_reposition = true;
					if (args.userdata == NEW_DIRECTIONALLIGHT || args.userdata == NEW_CAMERA)
					{
						should_reposition = false;
					}

					if (should_reposition)
					{
						transform->translation_local = in_front_of_camera;
						transform->SetDirty();
					}
				}
			}

			Protean::Archive& archive = AdvanceHistory();
			archive << EditorComponent::HISTORYOP_ADD;
			RecordSelection(archive);

			ClearSelected();
			AddSelected(pick);

			RecordSelection(archive);
			RecordEntity(archive, pick.entity);
		}
		componentsWnd.RefreshEntityTree();
	});
	newEntityCombo.SetEnabled(true);
	newEntityCombo.SetTooltip("Create a new entity");
	GetGUI().AddWidget(&newEntityCombo);

	translateButton.Create(ICON_TRANSLATE);
	rotateButton.Create(ICON_ROTATE);
	scaleButton.Create(ICON_SCALE);
	{
		scaleButton.SetShadowRadius(2);
		scaleButton.SetTooltip("Scale\nHotkey: 3");
		scaleButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
		scaleButton.OnClick([this](Protean::Gui::EventArgs args) {
			translator.isScalator = true;
			translator.isTranslator = false;
			translator.isRotator = false;
		});
		GetGUI().AddWidget(&scaleButton);

		rotateButton.SetShadowRadius(2);
		rotateButton.SetTooltip("Rotate\nHotkey: 2");
		rotateButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
		rotateButton.OnClick([this](Protean::Gui::EventArgs args) {
			translator.isRotator = true;
			translator.isScalator = false;
			translator.isTranslator = false;
			});
		GetGUI().AddWidget(&rotateButton);

		translateButton.SetShadowRadius(2);
		translateButton.SetTooltip("Translate/Move (Ctrl + T)\nHotkey: 1");
		translateButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
		translateButton.OnClick([this](Protean::Gui::EventArgs args) {
			translator.isTranslator = true;
			translator.isScalator = false;
			translator.isRotator = false;
			});
		GetGUI().AddWidget(&translateButton);
	}

	localGlobalButton.Create(ICON_GLOBAL_SPACE);
	localGlobalButton.SetShadowRadius(2);
	localGlobalButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	UpdateLocalGlobalButton();
	localGlobalButton.OnClick([this](Protean::Gui::EventArgs args) {
		translator.isLocalSpace = !translator.isLocalSpace;
		UpdateLocalGlobalButton();
	});
	GetGUI().AddWidget(&localGlobalButton);

	physicsButton.Create(ICON_RIGIDBODY);
	physicsButton.SetShadowRadius(2);
	physicsButton.SetTooltip("Toggle Physics Simulation On/Off\n\tOr: press while holding left Ctrl to reset physics bodies");
	if (main->config.GetSection("options").Has("physics"))
	{
		Protean::Physics::SetSimulationEnabled(main->config.GetSection("options").GetBool("physics"));
	}
	physicsButton.OnClick([this](Protean::Gui::EventArgs args) {
		if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL))
		{
			Protean::Physics::ResetPhysicsObjects(GetCurrentScene());
		}
		else
		{
			Protean::Physics::SetSimulationEnabled(!Protean::Physics::IsSimulationEnabled());
			main->config.GetSection("options").Set("physics", Protean::Physics::IsSimulationEnabled());
		}
	});
	GetGUI().AddWidget(&physicsButton);

	dummyButton.Create(ICON_DUMMY);
	dummyButton.SetShadowRadius(2);
	dummyButton.SetTooltip("Toggle reference dummy visualizer\n - Use the reference dummy to get an idea about object sizes compared to a human character size.\n - Position the dummy by clicking on something with the middle mouse button while the dummy is active.\n - Pressing this button while Ctrl key is held down will reset dummy position to the origin.\n - Pressing this button while the Shift key is held down will switch between male and female dummies.");
	dummyButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	dummyButton.OnClick([this](Protean::Gui::EventArgs args) {
		if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RCONTROL))
		{
			dummy_pos = XMFLOAT3(0, 0, 0);
		}
		else if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RSHIFT))
		{
			dummy_male = !dummy_male;
		}
		else
		{
			dummy_enabled = !dummy_enabled;
		}
	});
	GetGUI().AddWidget(&dummyButton);

	navtestButton.Create(ICON_NAVIGATION);
	navtestButton.SetShadowRadius(2);
	navtestButton.SetTooltip("Toggle navigation testing. When enabled, you can visualize path finding results.\nYou can put down START and GOAL waypoints inside voxel grids to test path finding.\nControls:\n----------\nF5 + middle click: put START to surface\nF6 + middle click: put GOAL to surface\nF7 + middle click: put START to air\nF8 + middle click: put GOAL to air");
	navtestButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	navtestButton.OnClick([this](Protean::Gui::EventArgs args) {
		navtest_enabled = !navtest_enabled;
	});
	GetGUI().AddWidget(&navtestButton);

	playButton.Create(ICON_PLAY);
	playButton.font.params.shadowColor = Protean::Color::Transparent();
	playButton.SetShadowRadius(2);
	playButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	playButton.SetTooltip("Execute the last used (standalone) script.\nTo use a new script, use the Open button.");
	playButton.OnClick([this](Protean::Gui::EventArgs args) {
		if (last_script_path.empty() || !Protean::Helper::FileExists(last_script_path))
		{
			contentBrowserWnd.RefreshContent();
			contentBrowserWnd.SetVisible(true);
			contentBrowserWnd.SetEnabled(true);
		}
		else
		{
			Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				Protean::Lua::RunFile(last_script_path);
			});
		}
	});
	topmenuWnd.AddWidget(&playButton);

	if (main->config.Has("last_script_path"))
	{
		last_script_path = main->config.GetText("last_script_path");
	}
	playButton.SetScriptTip("dofile(\"" + last_script_path + "\")");


	stopButton.Create(ICON_STOP);
	stopButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	stopButton.font.params.shadowColor = Protean::Color::Transparent();
	stopButton.SetShadowRadius(2);
	stopButton.SetTooltip("Stops every script background processes that are still running.");
	stopButton.SetScriptTip("killProcesses()");
	stopButton.OnClick([](Protean::Gui::EventArgs args) {
		Protean::Lua::KillProcesses();
	});
	topmenuWnd.AddWidget(&stopButton);


	projectCreatorButton.Create(ICON_PROJECT_CREATE);
	projectCreatorButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	projectCreatorButton.font.params.shadowColor = Protean::Color::Transparent();
	projectCreatorButton.SetShadowRadius(2);
	projectCreatorButton.SetTooltip("Create a new project.");
	projectCreatorButton.OnClick([this](Protean::Gui::EventArgs args) {
		projectCreatorWnd.SetVisible(!projectCreatorWnd.IsVisible());
	});
	topmenuWnd.AddWidget(&projectCreatorButton);



	saveButton.Create("Save");
	saveButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	saveButton.font.params.shadowColor = Protean::Color::Transparent();
	saveButton.SetShadowRadius(2);
	saveButton.SetTooltip("Save the current scene to a new file (Ctrl + Shift + S)\nBy default, the scene will be saved into .wiscene, but you can specify .gltf or .glb extensions to export into GLTF.\nYou can also use Ctrl + S to quicksave, without browsing.");
	saveButton.SetColor(Protean::Color(50, 180, 100, 180), Protean::Gui::WIDGETSTATE::IDLE);
	saveButton.SetColor(Protean::Color(50, 220, 140, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	saveButton.OnClick([this](Protean::Gui::EventArgs args) {
		SaveAs();
		});
	topmenuWnd.AddWidget(&saveButton);


	openButton.Create("Open");
	openButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	openButton.SetShadowRadius(2);
	openButton.font.params.shadowColor = Protean::Color::Transparent();
	openButton.SetTooltip("Open a scene, import an asset or execute a Lua script...\nSupported file types: .wiscene, .obj, .gltf, .glb, .vrm, .fbx, .lua, images, videos, sounds, etc.");
#ifdef PLATFORM_WINDOWS_DESKTOP
	openButton.SetTooltip(openButton.GetTooltip() + "\nYou can also drag and drop a file onto the window to open it in the Editor.");
#endif // PLATFORM_WINDOWS_DESKTOP
	openButton.SetColor(Protean::Color(50, 100, 255, 180), Protean::Gui::WIDGETSTATE::IDLE);
	openButton.SetColor(Protean::Color(120, 160, 255, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	openButton.OnClick([this](Protean::Gui::EventArgs args) {
		const uint64_t target_scene_id = GetCurrentEditorScene().id;
		Protean::Helper::FileDialogParams params;
		params.type = Protean::Helper::FileDialogParams::OPEN;
		params.description = ".wiscene, .obj, .gltf, .glb, .vrm, .fbx, .ply, .lua, .mp4, .png, ...";
		params.extensions.push_back("wiscene");
		params.extensions.push_back("obj");
		params.extensions.push_back("gltf");
		params.extensions.push_back("glb");
		params.extensions.push_back("vrm");
		params.extensions.push_back("vrma");
		params.extensions.push_back("fbx");
		params.extensions.push_back("ply");
		params.extensions.push_back("lua");
		params.extensions.push_back("txt");
		const auto ext_video = Protean::ResourceManager::GetSupportedVideoExtensions();
		for (auto& x : ext_video)
		{
			params.extensions.push_back(x);
		}
		const auto ext_sound = Protean::ResourceManager::GetSupportedSoundExtensions();
		for (auto& x : ext_sound)
		{
			params.extensions.push_back(x);
		}
		const auto ext_image = Protean::ResourceManager::GetSupportedImageExtensions();
		for (auto& x : ext_image)
		{
			params.extensions.push_back(x);
		}
		Protean::Helper::FileDialog(params, [=](const std::string& fileName) {
			Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				// Switch to the target scene before opening the file
				const EditorScene* target = FindEditorSceneByID(target_scene_id);
				if (target != nullptr)
				{
					// Find index and switch to that scene
					for (int i = 0; i < (int)scenes.size(); ++i)
					{
						if (scenes[i].get() == target)
						{
							SetCurrentScene(i);
							break;
						}
					}
				}
				Open(fileName);
				});
			});
		});
	topmenuWnd.AddWidget(&openButton);


	contentBrowserButton.Create("Content Browser");
	contentBrowserButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	contentBrowserButton.SetShadowRadius(2);
	contentBrowserButton.font.params.shadowColor = Protean::Color::Transparent();
	contentBrowserButton.SetTooltip("Browse content.");
	contentBrowserButton.SetColor(Protean::Color(50, 100, 255, 180), Protean::Gui::WIDGETSTATE::IDLE);
	contentBrowserButton.SetColor(Protean::Color(120, 160, 255, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	contentBrowserButton.OnClick([this](Protean::Gui::EventArgs args) {
		contentBrowserWnd.SetVisible(!contentBrowserWnd.IsVisible());
		if (contentBrowserWnd.IsVisible())
		{
			contentBrowserWnd.RefreshContent();
		}
	});
	topmenuWnd.AddWidget(&contentBrowserButton);


	logButton.Create("Backlog");
	logButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	logButton.SetShadowRadius(2);
	logButton.font.params.shadowColor = Protean::Color::Transparent();
	logButton.SetTooltip("Open the backlog (toggle with HOME button)");
	logButton.SetColor(Protean::Color(50, 160, 200, 180), Protean::Gui::WIDGETSTATE::IDLE);
	logButton.SetColor(Protean::Color(120, 200, 200, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	logButton.OnClick([](Protean::Gui::EventArgs args) {
		Protean::Backlog::Toggle();
		});
	topmenuWnd.AddWidget(&logButton);


	profilerButton.Create("Profiler");
	profilerButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	profilerButton.SetShadowRadius(2);
	profilerButton.font.params.shadowColor = Protean::Color::Transparent();
	profilerButton.SetTooltip("View the profiler frame timings");
	profilerButton.SetColor(Protean::Color(50, 160, 200, 180), Protean::Gui::WIDGETSTATE::IDLE);
	profilerButton.SetColor(Protean::Color(120, 200, 200, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	profilerButton.OnClick([this](Protean::Gui::EventArgs args) {
		profilerWnd.SetVisible(!Protean::Profiler::IsEnabled());
		Protean::Profiler::SetEnabled(!Protean::Profiler::IsEnabled());
	});
	topmenuWnd.AddWidget(&profilerButton);


	cinemaButton.Create(ICON_CINEMA_MODE);
	cinemaButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	cinemaButton.SetShadowRadius(2);
	cinemaButton.font.params.shadowColor = Protean::Color::Transparent();
	cinemaButton.SetTooltip("Enter cinema mode (all HUD disabled). Press ESC to return to normal.");
	cinemaButton.SetColor(Protean::Color(50, 160, 200, 180), Protean::Gui::WIDGETSTATE::IDLE);
	cinemaButton.SetColor(Protean::Color(120, 200, 200, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	cinemaButton.OnClick([this](Protean::Gui::EventArgs args) {
		cinema_mode_saved_debugEnvProbes = Protean::Renderer::GetToDrawDebugEnvProbes();
		cinema_mode_saved_debugCameras = Protean::Renderer::GetToDrawDebugCameras();
		cinema_mode_saved_debugColliders = Protean::Renderer::GetToDrawDebugColliders();
		cinema_mode_saved_debugEmitters = Protean::Renderer::GetToDrawDebugEmitters();
		cinema_mode_saved_debugForceFields = Protean::Renderer::GetToDrawDebugForceFields();
		cinema_mode_saved_debugBoneLines = Protean::Renderer::GetToDrawDebugBoneLines();
		cinema_mode_saved_debugPartitionTree = Protean::Renderer::GetToDrawDebugPartitionTree();
		cinema_mode_saved_debugSprings = Protean::Renderer::GetToDrawDebugSprings();
		cinema_mode_saved_gridHelper = Protean::Renderer::GetToDrawGridHelper();

		Protean::Renderer::SetToDrawDebugEnvProbes(false);
		Protean::Renderer::SetToDrawDebugCameras(false);
		Protean::Renderer::SetToDrawDebugColliders(false);
		Protean::Renderer::SetToDrawDebugEmitters(false);
		Protean::Renderer::SetToDrawDebugForceFields(false);
		Protean::Renderer::SetToDrawDebugBoneLines(false);
		Protean::Renderer::SetToDrawDebugPartitionTree(false);
		Protean::Renderer::SetToDrawDebugSprings(false);
		Protean::Renderer::SetToDrawGridHelper(false);

		if (renderPath != nullptr)
		{
			renderPath->GetGUI().SetVisible(false);
		}
		GetGUI().SetVisible(false);
		Protean::Profiler::SetEnabled(false);
		profilerWnd.SetVisible(false);
	});
	GetGUI().AddWidget(&cinemaButton);

	fullscreenButton.Create("Full screen");
	fullscreenButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	fullscreenButton.SetShadowRadius(2);
	fullscreenButton.font.params.shadowColor = Protean::Color::Transparent();
	fullscreenButton.SetTooltip("Toggle full screen");
	fullscreenButton.SetColor(Protean::Color(50, 160, 200, 180), Protean::Gui::WIDGETSTATE::IDLE);
	fullscreenButton.SetColor(Protean::Color(120, 200, 200, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	fullscreenButton.OnClick([this](Protean::Gui::EventArgs args) {
		bool fullscreen = main->config.GetBool("fullscreen");
		fullscreen = !fullscreen;
		main->config.Set("fullscreen", fullscreen);

		Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t) {

			main->SetFullScreen(fullscreen);

		});
	});
	topmenuWnd.AddWidget(&fullscreenButton);


	bugButton.Create("Bug report");
	bugButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	bugButton.SetShadowRadius(2);
	bugButton.font.params.shadowColor = Protean::Color::Transparent();
	bugButton.SetTooltip("Opens a browser window where you can report a bug or an issue.\nURL: https://github.com/turanszkij/WickedEngine/issues/new");
	bugButton.SetColor(Protean::Color(50, 160, 200, 180), Protean::Gui::WIDGETSTATE::IDLE);
	bugButton.SetColor(Protean::Color(120, 200, 200, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	bugButton.OnClick([](Protean::Gui::EventArgs args) {
		Protean::Helper::OpenUrl("https://github.com/turanszkij/WickedEngine/issues/new");
	});
	topmenuWnd.AddWidget(&bugButton);


	aboutButton.Create("About");
	aboutButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	aboutButton.SetShadowRadius(2);
	aboutButton.font.params.shadowColor = Protean::Color::Transparent();
	aboutButton.SetTooltip("About...");
	aboutButton.SetColor(Protean::Color(50, 160, 200, 180), Protean::Gui::WIDGETSTATE::IDLE);
	aboutButton.SetColor(Protean::Color(120, 200, 200, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	aboutButton.OnClick([this](Protean::Gui::EventArgs args) {
		aboutWindow.SetVisible(!aboutWindow.IsVisible());
		});
	topmenuWnd.AddWidget(&aboutButton);

	{
		std::string ss;
		ss += "Wicked Editor v";
		ss += Protean::Version::GetVersionString();
		ss += "\n\nWebsite: https://wickedengine.net/";
		ss += "\nSource code: https://github.com/turanszkij/WickedEngine";
		ss += "\nDiscord chat: https://discord.gg/CFjRYmE";
		ss += "\n\nControls\n";
		ss += "------------\n";
		ss += "Move camera: " + GetInputString(EditorActions_MOVE_CAMERA_FORWARD) + ", " + GetInputString(EditorActions_MOVE_CAMERA_LEFT) + ", " + GetInputString(EditorActions_MOVE_CAMERA_BACKWARD) + ", " + GetInputString(EditorActions_MOVE_CAMERA_RIGHT) + ", or Controller left stick or D - pad\n";
		ss += "Look: Right mouse button / arrow keys / controller right stick\n";
		ss += "Select: Left mouse button\n";
		ss += "Select top-level parent: Alt + Left mouse button\n";
		ss += "Interact with physics/water/grass: Middle mouse button\n";
		ss += "Faster camera: Left Shift button or controller R2/RT\n";
		ss += "Snap to grid transform: Ctrl (hold while transforming)\n";
		ss += "Snap to surface transform: Shift (hold while transforming)\n";
		ss += "Camera up: " + GetInputString(EditorActions_MOVE_CAMERA_UP) + "\n";
		ss += "Camera down: " + GetInputString(EditorActions_MOVE_CAMERA_DOWN) + "\n";
		ss += "Orthographic camera: " + GetInputString(EditorActions_ORTHO_CAMERA) + "\n";
		ss += "Select whole hierarchy of selected: " + GetInputString(EditorActions_HIERARCHY_SELECT) + "\n";
		ss += "Duplicate entity: " + GetInputString(EditorActions_DUPLICATE_ENTITY) + "\n";
		ss += "Select All: " + GetInputString(EditorActions_SELECT_ALL_ENTITIES) + "\n";
		ss += "Deselect All: " + GetInputString(EditorActions_DESELECT_ALL_ENTITIES) + "\n";
		ss += "Rename Selected: " + GetInputString(EditorActions_RENAME_SELECTED) + " (this hotkey is temporary, will be F2 in the future)" + "\n";
		ss += "Undo: " + GetInputString(EditorActions_UNDO_ACTION) + "\n";
		ss += "Redo: " + GetInputString(EditorActions_REDO_ACTION) + "\n";
		ss += "Copy: " + GetInputString(EditorActions_COPY_ACTION) + "\n";
		ss += "Cut: " + GetInputString(EditorActions_CUT_ACTION) + "\n";
		ss += "Paste: " + GetInputString(EditorActions_PASTE_ACTION) + "\n";
		ss += "Delete: Delete button\n";
		ss += "Save As: " + GetInputString(EditorActions_SAVE_SCENE_AS) + "\n";
		ss += "Save: " + GetInputString(EditorActions_SAVE_SCENE) + "\n";
		ss += "Transform: " + GetInputString(EditorActions_ENABLE_TRANSFORM_TOOL) + "\n";
		ss += "Move Toggle: " + GetInputString(EditorActions_MOVE_TOGGLE_ACTION) + "\n";
		ss += "Rotate Toggle: " + GetInputString(EditorActions_ROTATE_TOGGLE_ACTION) + "\n";
		ss += "Scale Toggle: " + GetInputString(EditorActions_SCALE_TOGGLE_ACTION) + "\n";
		ss += "Local/Global Toggle: " + GetInputString(EditorActions_LOCAL_GLOBAL_TOGGLE_ACTION) + "\n";
		ss += "Reload terrain props: " + GetInputString(EditorActions_RELOAD_TERRAIN_PROPS) + "\n";
		ss += "Wireframe mode: " + GetInputString(EditorActions_WIREFRAME_MODE) + "\n";
		ss += "Screenshot (saved into Editor's screenshots folder): " + GetInputString(EditorActions_SCREENSHOT) + "\n";
		ss += "Screenshot without GUI (saved into Editor's screenshots folder): " + GetInputString(EditorActions_SCREENSHOT_NOGUI) + "\n";
		ss += "Screenshot with background as transparency (saved into Editor's screenshots folder): " + GetInputString(EditorActions_SCREENSHOT_ALPHA) + "\n";
		ss += "Screenshot selection with everything else as transparency (saved into Editor's screenshots folder): " + GetInputString(EditorActions_SCREENSHOT_ALPHA_SELECTION) + "\n";
		ss += "Depth of field refocus to point: " + GetInputString(EditorActions_DEPTH_OF_FIELD_REFOCUS_TO_POINT) + " + left mouse button" + "\n";
		ss += "Color grading reference: " + GetInputString(EditorActions_COLOR_GRADING_REFERENCE) + " (color grading palette reference will be displayed in top left corner)\n";
		ss += "Focus on selected: " + GetInputString(EditorActions_FOCUS_ON_SELECTION) + " button, this will make the camera jump to selection.\n";
		ss += "Inspector mode: " + GetInputString(EditorActions_INSPECTOR_MODE) + " button (hold), hovered entity information will be displayed near mouse position.\n";
		ss += "Place Instances: " + GetInputString(EditorActions_PLACE_INSTANCES) + " (place clipboard onto clicked surface)\n";
		ss += "Ragdoll and physics impulse tester: Hold " + GetInputString(EditorActions_RAGDOLL_AND_PHYSICS_IMPULSE_TESTER) + " and click on ragdoll or rigid body physics entity with middle mouse.\n";
		ss += "Add new node to spline: " + GetInputString(EditorActions_ADD_TO_SPLINE) + " while a spline is selected.\n";
		ss += "Script Console / backlog: HOME button\n";
		ss += "Bone picking: First select an armature, only then bone picking becomes available. As long as you have an armature or bone selected, bone picking will remain active.\n";
		ss += "\n";
		ss += "\nTips\n";
		ss += "-------\n";
		ss += "You can find sample scenes in the Content/models directory. Try to load one.\n";
		ss += "You can also import models from .OBJ, .GLTF, .GLB, .VRM, .FBX, .PLY files.\n";
		ss += "You can find a program configuration file at Editor/config.ini\n";
		ss += "You can find sample LUA scripts in the Content/scripts directory. Try to load one.\n";
		ss += "You can find a startup script in startup.lua (this will be executed on program start, if exists)\n";
		ss += "You can use some command line arguments (without any prefix):\n";
#ifdef PLATFORM_WINDOWS_DESKTOP
		ss += "\t- Select Vulkan graphics device instead of DX12: vulkan\n";
#endif // PLATFORM_WINDOWS_DESKTOP
		ss += "\t- Prefer integrated GPU: igpu\n";
		ss += "\t- Prefer AMD GPU: amdgpu\n";
		ss += "\t- Prefer Nvidia GPU: nvidiagpu\n";
		ss += "\t- Prefer Intel GPU: intelgpu\n";
		ss += "\t- Enable graphics device debug mode: debugdevice\n";
		ss += "\t- Enable graphics device GPU-based validation: gpuvalidation\n";
		ss += "\t- Make window always active, even when in background: alwaysactive\n";
		ss += "\nFor questions, bug reports, feedback, requests, please open an issue at:\n";
		ss += "https://github.com/turanszkij/WickedEngine/issues\n";
		ss += "\n\n";
		ss += Protean::Version::GetCreditsString();

		aboutLabel.Create("AboutLabel");
		aboutLabel.SetText(ss);
		aboutLabel.SetSize(XMFLOAT2(600,4000));
		aboutLabel.SetColor(Protean::Color(113, 183, 214, 100));
		aboutLabel.SetLocalizationEnabled(false);
		aboutWindow.AddWidget(&aboutLabel);

		auto wctrl = Protean::Gui::Window::WindowControls_ALL;
		wctrl &= ~Protean::Gui::Window::WindowControls_RESIZE_BOTTOMLEFT;
		aboutWindow.Create("About", wctrl);
		aboutWindow.SetVisible(false);
		aboutWindow.SetPos(XMFLOAT2(100, 100));
		aboutWindow.SetSize(XMFLOAT2(640, 480));
		aboutWindow.OnResize([this]() {
			aboutLabel.SetSize(XMFLOAT2(aboutWindow.GetWidgetAreaSize().x - 20, aboutLabel.GetSize().y));
		});
		aboutWindow.OnCollapse([this](Protean::Gui::EventArgs args) {
			const bool gui_round_enabled = !generalWnd.disableRoundCornersCheckBox.GetCheck();
			for (int i = 0; i < arraysize(Protean::Gui::Widget::sprites); ++i)
			{
				if (gui_round_enabled)
				{
					aboutWindow.sprites[i].params.enableCornerRounding();
					aboutWindow.sprites[i].params.corners_rounding[0].radius = 10;
					aboutWindow.sprites[i].params.corners_rounding[1].radius = 10;
					aboutWindow.sprites[i].params.corners_rounding[2].radius = 10;
					aboutWindow.sprites[i].params.corners_rounding[3].radius = 10;
				}
				else
				{
					aboutWindow.sprites[i].params.disableCornerRounding();
				}
			}
		});
		GetGUI().AddWidget(&aboutWindow);
	}

	exitButton.Create("Exit");
	exitButton.SetLocalizationEnabled(Protean::Gui::LocalizationEnabled_Tooltip);
	exitButton.SetShadowRadius(2);
	exitButton.font.params.shadowColor = Protean::Color::Transparent();
	exitButton.SetTooltip("Exit");
	exitButton.SetColor(Protean::Color(160, 50, 50, 180), Protean::Gui::WIDGETSTATE::IDLE);
	exitButton.SetColor(Protean::Color(200, 50, 50, 255), Protean::Gui::WIDGETSTATE::FOCUS);
	exitButton.OnClick([this](Protean::Gui::EventArgs args) {
		main->Exit();
		});
	topmenuWnd.AddWidget(&exitButton);

	guiScalingCombo.Create("GuiScaling");
	guiScalingCombo.SetDropArrowEnabled(false);
	guiScalingCombo.SetFixedDropWidth(60);
	guiScalingCombo.SetText("");
	guiScalingCombo.SetTooltip("Set the custom scaling factor for the GUI.\nNote that this is in addition to the operating system's DPI scaling for the monitor.");
	guiScalingCombo.AddItem("50%", 50);
	guiScalingCombo.AddItem("75%", 75);
	guiScalingCombo.AddItem("100%", 100);
	guiScalingCombo.AddItem("125%", 125);
	guiScalingCombo.AddItem("150%", 150);
	guiScalingCombo.AddItem("175%", 175);
	guiScalingCombo.AddItem("200%", 200);
	guiScalingCombo.AddItem("225%", 225);
	guiScalingCombo.AddItem("250%", 250);
	if (main->config.Has("scaling"))
	{
		guiScalingCombo.SetSelectedByUserdata((uint64_t)main->config.GetInt("scaling"));
	}
	else
	{
		guiScalingCombo.SetSelectedByUserdataWithoutCallback(100);
	}
	guiScalingCombo.OnSelect([this](Protean::Gui::EventArgs args) {
		this->main->config.Set("scaling", (int)args.userdata);
	});
	GetGUI().AddWidget(&guiScalingCombo);

	is2DModeButton.Create("is2DModeButton");
	is2DModeButton.SetTooltip("Switch between 3D and 2D editor workflows. 2D mode will lock the camera and controls to the XY axes.");
	is2DModeButton.SetShadowRadius(2);
	is2DModeButton.OnClick([this](Protean::Gui::EventArgs args) {
		Set2DMode(!is_2D_mode);
	});
	GetGUI().AddWidget(&is2DModeButton);

	componentsWnd.Create(this);
	GetGUI().AddWidget(&componentsWnd);

	profilerWnd.Create(this);
	GetGUI().AddWidget(&profilerWnd);

	contentBrowserWnd.Create(this);
	GetGUI().AddWidget(&contentBrowserWnd);

	projectCreatorWnd.Create(this);
	GetGUI().AddWidget(&projectCreatorWnd);

	themeEditorWnd.Create(this);
	GetGUI().AddWidget(&themeEditorWnd);

	SetDefaultLocalization();
	generalWnd.RefreshLanguageSelectionAfterWholeGUIWasInitialized();

	generalWnd.RefreshTheme();

	auto load_font = [this](std::string filename) {
		font_datas.emplace_back().name = filename;
		Protean::Helper::FileRead(filename, font_datas.back().filedata);
	};
	Protean::Helper::GetFileNamesInDirectory(Protean::Helper::GetCurrentPath() + "/fonts/", load_font, "TTF");

	{
		size_t current_recent = 0;
		auto& recent = main->config.GetSection("recent");
		while (recent.Has(std::to_string(current_recent).c_str()))
		{
			recentFilenames.push_back(recent.GetText(std::to_string(current_recent).c_str()));
			current_recent++;
		}
	}
	{
		size_t current_recent = 0;
		auto& recent = main->config.GetSection("recent_folders");
		while (recent.Has(std::to_string(current_recent).c_str()))
		{
			recentFolders.push_back(recent.GetText(std::to_string(current_recent).c_str()));
			current_recent++;
		}
	}

	GraphicsDevice* device = GetDevice();

	// Set up gradients for the spline visualizer:
	uint8_t spline_gradient[4096];
	for (int i = 0; i < arraysize(spline_gradient); ++i)
	{
		float u = float(i) / float(arraysize(spline_gradient) - 1);
		float p = u * 2 - 1;
		float c = smoothstep(0.0f, 0.1f, 1.0f - saturate(std::abs(p)));
		spline_gradient[i] = uint8_t(c * 255);
	}
	bool success = Protean::TextureHelper::CreateTexture(
		spline_renderer.texture,
		spline_gradient,
		arraysize(spline_gradient),
		1,
		Format_R8_UNORM,
		SwizzleFromString("111r")
	);
	assert(success);
	device->SetName(&spline_renderer.texture, "editor.spline_renderer.texture");

	for (int i = 0; i < arraysize(spline_gradient); ++i)
	{
		float u = float(i) / float(arraysize(spline_gradient) - 1);
		float p = u * 2 - 1;
		float c = lerp(0.2f, 1.0f, smoothstep(0.0f, 0.6f, std::abs(p)));
		spline_gradient[i] = uint8_t(c * 255);
	}
	success = Protean::TextureHelper::CreateTexture(
		spline_renderer.texture2,
		spline_gradient,
		arraysize(spline_gradient),
		1,
		Format_R8_UNORM,
		SwizzleFromString("111r")
	);
	assert(success);
	device->SetName(&spline_renderer.texture2, "editor.spline_renderer.texture2");

	RenderPath2D::Load();

	wilog_success("Editor loaded in %d ms", (int)std::round(timer.elapsed()));
}
void EditorComponent::Start()
{
	// Add other fonts that were loaded from fonts directory as fallback fonts:
	for (auto& x : font_datas)
	{
		Protean::Font::AddFontStyle(x.name, x.filedata.data(), x.filedata.size());
	}

	graphicsWnd.ApplySamplerSettings();

	componentsWnd.RefreshEntityTree(); // called at Start() for updating the tree list when returning from scripts too

	RenderPath2D::Start();
}
void EditorComponent::PreUpdate()
{
	RenderPath2D::PreUpdate();

	renderPath->PreUpdate();
}
void EditorComponent::FixedUpdate()
{
	RenderPath2D::FixedUpdate();

	renderPath->FixedUpdate();
}
void EditorComponent::Update(float dt)
{
	Protean::Profiler::range_id profrange = Protean::Profiler::BeginRangeCPU("Editor Update");

	main->canvas.scaling = float(guiScalingCombo.GetSelectedUserdata()) / 100.0f;

	Protean::Renderer::SetDebugDrawEnabled(true);
	if (screenshot)
	{
		screenshot = false;
		std::string filename = Protean::Helper::screenshot(main->swapChain);
		PostSaveText(filename);
		if (filename.empty())
		{
			PostSaveText("Error! Screenshot was not successful!");
		}
		else
		{
			PostSaveText("Screenshot saved: ", filename);
		}
	}
	if (screenshot_nogui)
	{
		screenshot_nogui = false;
		std::string filename = Protean::Helper::screenshot(renderPath->GetRenderResult3D());
		PostSaveText(filename);
		if (filename.empty())
		{
			PostSaveText("Error! Screenshot was not successful!");
		}
		else
		{
			PostSaveText("Screenshot saved: ", filename);
		}
	}
	if (screenshot_alpha)
	{
		screenshot_alpha = false;
		std::string filename = Protean::Helper::screenshot(renderPath->CreateScreenshotWithAlphaBackground());
		PostSaveText(filename);
		if (filename.empty())
		{
			PostSaveText("Error! Screenshot was not successful!");
		}
		else
		{
			PostSaveText("Screenshot saved: ", filename);
		}
	}
	if (screenshot_alpha_selection)
	{
		screenshot_alpha_selection = false;
		std::string filename = Protean::Helper::screenshot(renderPath->CreateScreenshotWithAlphaBackground(EDITORSTENCILREF_HIGHLIGHT_OBJECT, Protean::Image::STENCILMODE_EQUAL, Protean::Image::STENCILREFMODE_USER));
		PostSaveText(filename);
		if (filename.empty())
		{
			PostSaveText("Error! Screenshot was not successful!");
		}
		else
		{
			PostSaveText("Screenshot saved: ", filename);
		}
	}

	// screenshot is done next frame update to capture current frame render and the alpha modes also disable debug draw for just this frame
	if (CheckInput(EditorActions_SCREENSHOT_NOGUI)) // shift priority
	{
		screenshot_nogui = true;
		Protean::Renderer::SetDebugDrawEnabled(false);
	}
	else if (CheckInput(EditorActions_SCREENSHOT_ALPHA_SELECTION)) // shift priority
	{
		screenshot_alpha_selection = true;
		Protean::Renderer::SetDebugDrawEnabled(false);
	}
	else if (CheckInput(EditorActions_SCREENSHOT))
	{
		screenshot = true;
	}
	else if (CheckInput(EditorActions_SCREENSHOT_ALPHA))
	{
		screenshot_alpha = true;
		Protean::Renderer::SetDebugDrawEnabled(false);
	}

	loadmodel_font.SetHidden(!Protean::JobSystem::IsBusy(loadmodel_workload));

	bool terrain_generating = false;
	for (size_t i = 0; i < GetCurrentScene().terrains.GetCount(); ++i)
	{
		if (GetCurrentScene().terrains[i].IsGenerationBusy())
		{
			terrain_generating = true;
			break;
		}
	}
	if (terrain_generating)
	{
		terrain_generation_font_timeout = 0.5f;
	}
	else
	{
		terrain_generation_font_timeout -= dt;
	}
	terrain_generation_font.SetHidden(terrain_generation_font_timeout <= 0);

	Scene& scene = GetCurrentScene();
	EditorScene& editorscene = GetCurrentEditorScene();
	CameraComponent& camera = editorscene.camera;

	translator.scene = &scene;
	translator.is2D = is_2D_mode;
	Protean::Renderer::SetGridHelper2D(is_2D_mode);

	if (scene.forces.Contains(grass_interaction_entity))
	{
		scene.Entity_Remove(grass_interaction_entity);
	}

	cameraWnd.UpdateData();
	paintToolWnd.UpdateData(dt);
	graphicsWnd.UpdateData();
	componentsWnd.UpdateData(dt);

	// Pulsating selection color update:
	outlineTimer += dt;

	CheckBonePickingEnabled();

	save_text_alpha = std::max(0.0f, save_text_alpha - std::min(dt, 0.033f)); // after saving, dt can become huge

	bool clear_selected = false;
	if (CheckInput(EditorActions_DESELECT_ALL_ENTITIES))
	{
		if (!GetGUI().IsVisible())
		{
			// Exit cinema mode:
			Protean::Renderer::SetToDrawDebugEnvProbes(cinema_mode_saved_debugEnvProbes);
			Protean::Renderer::SetToDrawDebugCameras(cinema_mode_saved_debugCameras);
			Protean::Renderer::SetToDrawDebugColliders(cinema_mode_saved_debugColliders);
			Protean::Renderer::SetToDrawDebugEmitters(cinema_mode_saved_debugEmitters);
			Protean::Renderer::SetToDrawDebugForceFields(cinema_mode_saved_debugForceFields);
			Protean::Renderer::SetToDrawDebugBoneLines(cinema_mode_saved_debugBoneLines);
			Protean::Renderer::SetToDrawDebugPartitionTree(cinema_mode_saved_debugPartitionTree);
			Protean::Renderer::SetToDrawDebugSprings(cinema_mode_saved_debugSprings);
			Protean::Renderer::SetToDrawGridHelper(cinema_mode_saved_gridHelper);

			if (renderPath != nullptr)
			{
				renderPath->GetGUI().SetVisible(true);
			}
			GetGUI().SetVisible(true);
		}
		else
		{
			clear_selected = true;
		}
	}

	if (!Protean::Backlog::isActive() && paintToolWnd.GetMode() == PaintToolWindow::MODE::MODE_DISABLED)
	{
		translator.Update(camera, currentMouse, *renderPath);
	}

	// Vehicle driving controls:
	if (!Protean::Backlog::isActive())
	{
		if (scene.rigidbodies.GetCount() == 0)
		{
			componentsWnd.rigidWnd.driveCheckbox.SetCheck(false);
		}
		if (componentsWnd.rigidWnd.driveCheckbox.GetCheck())
		{
			RigidBodyPhysicsComponent* rigidbody = scene.rigidbodies.GetComponent(componentsWnd.rigidWnd.driving_entity);
			if (rigidbody != nullptr && rigidbody->IsVehicle())
			{
				//Protean::Physics::SetDebugDrawEnabled(true);
				float forward = 0;
				float brake = 0;
				float handbrake = 0;

				float velocityAmount = Protean::Physics::GetVehicleForwardVelocity(*rigidbody);

				if (std::abs(Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_TRIGGER_R).x) > 0.1f)
				{
					if (velocityAmount >= 0)
					{
						forward = Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_TRIGGER_R).x;
					}
					else
					{
						brake = Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_TRIGGER_R).x;
					}
				}
				if (Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_TRIGGER_L).x > 0.1f)
				{
					if (velocityAmount > 0.001f)
					{
						brake = Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_TRIGGER_L).x;
					}
					else
					{
						forward = -Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_TRIGGER_L).x;
					}
				}
				if (std::abs(Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_THUMBSTICK_L).x) > 0.1f)
				{
					drive_steering_smoothed = lerp(drive_steering_smoothed, Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_THUMBSTICK_L).x, dt * 2);
				}
				if (Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_PLAYSTATION_SQUARE))
				{
					handbrake = 1;
				}
				drive_orbit_horizontal += Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_THUMBSTICK_R).x * XM_PI * dt;

				if (CheckInput(EditorActions_MOVE_CAMERA_FORWARD))
				{
					if (velocityAmount >= 0)
					{
						forward = 1;
					}
					else
					{
						brake = 1;
					}
				}
				else if (CheckInput(EditorActions_MOVE_CAMERA_BACKWARD))
				{
					if (velocityAmount > 0.001f)
					{
						brake = 1;
					}
					else
					{
						forward = -1;
					}
				}
				if (CheckInput(EditorActions_MOVE_CAMERA_LEFT))
				{
					drive_steering_smoothed -= dt * 2;
				}
				else if (CheckInput(EditorActions_MOVE_CAMERA_RIGHT))
				{
					drive_steering_smoothed += dt * 2;
				}
				else if (std::abs(Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_THUMBSTICK_L).x) > 0.1f)
				{
					// nothing
				}
				else
				{
					drive_steering_smoothed = lerp(drive_steering_smoothed, 0.0f, 4 * dt);
				}
				if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT))
				{
					brake = 1;
				}
				if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_SPACE))
				{
					handbrake = 1;
				}
				if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LEFT))
				{
					drive_orbit_horizontal += XM_PI * dt;
				}
				if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RIGHT))
				{
					drive_orbit_horizontal -= XM_PI * dt;
				}

				if (!GetGUI().HasFocus())
				{
					drive_cam_dist_next -= Protean::Input::GetPointer().z;
				}
				drive_cam_dist = lerp(drive_cam_dist, drive_cam_dist_next, dt * 2);

				drive_steering_smoothed = clamp(drive_steering_smoothed, -1.0f, 1.0f);

				if (rigidbody->IsMotorcycle())
				{
					// break presses front and rear breaks:
					handbrake = std::max(handbrake, brake);
				}

				Protean::Physics::DriveVehicle(*rigidbody, forward, drive_steering_smoothed, brake, handbrake);
				drive_mode = true;
			}
			else
			{
				drive_mode = false;
			}
		}
		else
		{
			drive_mode = false;
		}
		drive_orbit_horizontal = lerp(drive_orbit_horizontal, 0.0f, dt);
	}

	if (!GetGUI().IsTyping() && !translator.selected.empty() && CheckInput(EditorActions_RENAME_SELECTED))
	{
		for (auto& x : translator.selected)
		{
			if (!scene.names.Contains(x.entity))
			{
				scene.names.Create(x.entity);
			}
		}
		componentsWnd.nameWnd.SetEntity(translator.selected.back().entity);
		componentsWnd.nameWnd.SetCollapsed(false);
		componentsWnd.nameWnd.nameInput.SetAsActive(true);
	}

	// Duplicate Entity
	if (!GetGUI().IsTyping() && CheckInput(EditorActions_DUPLICATE_ENTITY))
	{
		Protean::Archive& archive = AdvanceHistory();
		archive << HISTORYOP_ADD;
		RecordSelection(archive);

		auto& prevSel = translator.selectedEntitiesNonRecursive;
		Protean::vector<Entity> addedEntities;
		for (auto& x : prevSel)
		{
			Protean::PickResult picked;
			picked.entity = scene.Entity_Duplicate(x);
			addedEntities.push_back(picked.entity);
		}

		ClearSelected();

		for (auto& x : addedEntities)
		{
			AddSelected(x);
		}

		RecordSelection(archive);
		RecordEntity(archive, addedEntities);

		componentsWnd.RefreshEntityTree();
	}

	// Ensure pointer is visible when right mouse button is not held:
	if (!Protean::Input::Down(Protean::Input::MOUSE_BUTTON_RIGHT))
	{
		Protean::Input::HidePointer(false);
	}

	// Camera control:
	if (!drive_mode && !Protean::Backlog::isActive() && !GetGUI().HasFocus())
	{
		deleting = CheckInput(EditorActions_DELETE_ACTION);
		currentMouse = Protean::Input::GetPointer();
		if (camControlStart)
		{
			originalMouse = currentMouse;
		}

		// This check whether pressing left mouse can modify selection:
		const bool leftmouse_select =
			!translator.IsInteracting() && // translator shouldn't be active
			paintToolWnd.GetMode() == PaintToolWindow::MODE::MODE_DISABLED && // paint tool shouldn't be active
			(!componentsWnd.decalWnd.IsEnabled() || !componentsWnd.decalWnd.placementCheckBox.GetCheck()) && // decal placement shouldn't be active
			!(Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL) && Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT)) && // instance placement shouldn't be active
			!Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_ALT) &&
			viewport3D_hitbox.intersects(XMFLOAT2(currentMouse.x, currentMouse.y)) &&
			Protean::Input::Press(Protean::Input::MOUSE_BUTTON_LEFT);

		// Alt+Click to select parent entity:
		const bool altmouse_select_parent =
			!translator.IsInteracting() &&
			paintToolWnd.GetMode() == PaintToolWindow::MODE::MODE_DISABLED &&
			(!componentsWnd.decalWnd.IsEnabled() || !componentsWnd.decalWnd.placementCheckBox.GetCheck()) &&
			Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_ALT) &&
			viewport3D_hitbox.intersects(XMFLOAT2(currentMouse.x, currentMouse.y)) &&
			Protean::Input::Press(Protean::Input::MOUSE_BUTTON_LEFT);

		// After originalMouse is saved, currentMouse is remapped inside 3D viewport:
		//	originalMouse shouldn't be remapped, because that will be used to reset mouse position
		currentMouse.x -= renderPath->PhysicalToLogical((uint32_t)viewport3D.top_left_x);
		currentMouse.y -= renderPath->PhysicalToLogical((uint32_t)viewport3D.top_left_y);

		static constexpr float tweak60 = 0.1f / 60.0f;
		float xDif = 0, yDif = 0;
		const Protean::Input::MouseState& mouse = Protean::Input::GetMouseState();
		const Protean::Input::Pinch pinch = Protean::Input::GetTouchPinch();
		const XMFLOAT2& pan = Protean::Input::GetTouchPan();

		if (Protean::Input::Down(Protean::Input::MOUSE_BUTTON_RIGHT))
		{
			camControlStart = false;
			xDif = mouse.delta_position.x;
			yDif = mouse.delta_position.y;
			xDif *= tweak60;
			yDif *= tweak60;
			if (!is_2D_mode)
			{
				Protean::Input::SetPointer(originalMouse);
				Protean::Input::HidePointer(true);
			}
		}
		else
		{
			camControlStart = true;
			Protean::Input::HidePointer(false);
		}

		const float buttonrotSpeed = 2.0f * dt;
		if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LEFT) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_NUMPAD4))
		{
			xDif -= buttonrotSpeed;
		}
		if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RIGHT) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_NUMPAD6))
		{
			xDif += buttonrotSpeed;
		}
		if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_UP) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_NUMPAD8))
		{
			yDif -= buttonrotSpeed;
		}
		if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_DOWN) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_NUMPAD2))
		{
			yDif += buttonrotSpeed;
		}

		const XMFLOAT4 leftStick = Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_THUMBSTICK_L, 0);
		const XMFLOAT4 rightStick = Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_THUMBSTICK_R, 0);
		const XMFLOAT4 rightTrigger = Protean::Input::GetAnalog(Protean::Input::GAMEPAD_ANALOG_TRIGGER_R, 0);

		const float joystickrotspeed = 0.05f;
		xDif += rightStick.x * joystickrotspeed;
		yDif += rightStick.y * joystickrotspeed;
		
		if (translator.state == Translator::TRANSLATOR_IDLE)
		{
			xDif += pan.x * tweak60;
			yDif += pan.y * tweak60;
		}

		xDif *= cameraWnd.rotationspeedSlider.GetValue();
		yDif *= cameraWnd.rotationspeedSlider.GetValue();


		if (CheckInput(EditorActions_ORTHO_CAMERA))
		{
			camera.SetOrtho(!camera.IsOrtho());
			camera.ortho_vertical_size = camera.ComputeOrthoVerticalSizeFromPerspective(Protean::Math::Length(camera.Eye)); // camera distance from origin
			cameraWnd.orthoCheckBox.SetCheck(camera.IsOrtho());
		}

		if (CheckInput(EditorActions_HIERARCHY_SELECT))
		{
			Protean::vector<Entity> children;
			for (auto& x : translator.selectedEntitiesNonRecursive)
			{
				scene.GatherChildren(x, children);
			}
			for (auto& x : children)
			{
				AddSelected(x);
			}
		}

		if (CheckInput(EditorActions_ADD_TO_SPLINE) && componentsWnd.splineWnd.entity != INVALID_ENTITY)
		{
			componentsWnd.splineWnd.NewNode();
		}

		if (CheckInput(EditorActions_RELOAD_TERRAIN_PROPS))
		{
			ReloadTerrainProps();
		}

		if (is_2D_mode)
		{
			// 2D camera
			const float clampedDT = std::min(dt, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...

			const float speed = ((Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT) ? 10.0f : 1.0f) + rightTrigger.x * 10.0f) * cameraWnd.movespeedSlider.GetValue() * clampedDT;
			XMVECTOR move = XMLoadFloat3(&editorscene.cam_move);
			XMVECTOR moveNew = XMVectorSet(rightStick.x, rightStick.y, 0, 0);

			if (!Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL))
			{
				// Only move camera if control not pressed
				if (CheckInput(EditorActions_MOVE_CAMERA_LEFT) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_LEFT)) { moveNew += XMVectorSet(-1, 0, 0, 0); }
				if (CheckInput(EditorActions_MOVE_CAMERA_RIGHT) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_RIGHT)) { moveNew += XMVectorSet(1, 0, 0, 0); }
				if (CheckInput(EditorActions_MOVE_CAMERA_FORWARD) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_UP)) { moveNew += XMVectorSet(0, 1, 0, 0); }
				if (CheckInput(EditorActions_MOVE_CAMERA_BACKWARD) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_DOWN)) { moveNew += XMVectorSet(0, -1, 0, 0); }
				if (CheckInput(EditorActions_MOVE_CAMERA_UP) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_2)) { moveNew += XMVectorSet(0, 1, 0, 0); }
				if (CheckInput(EditorActions_MOVE_CAMERA_DOWN) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_1)) { moveNew += XMVectorSet(0, -1, 0, 0); }
				moveNew = XMVector3Normalize(moveNew);
			}
			moveNew += XMVectorSet(leftStick.x, 0, leftStick.y, 0);
			moveNew *= speed;

			move = XMVectorLerp(move, moveNew, std::min(1.0f, cameraWnd.accelerationSlider.GetValue() * clampedDT / 0.0166f)); // smooth the movement a bit
			float moveLength = XMVectorGetX(XMVector3Length(move));
			float moveNewLength = XMVectorGetX(XMVector3Length(moveNew));

			// Only zero out movement when there is no input
			if (moveLength < 0.0001f && moveNewLength < 0.0001f)
			{
				move = XMVectorSet(0, 0, 0, 0);
			}

			// Mouse pan works completely differently in 2D mode, it "grabs canvas"
			XMFLOAT2 grabdiff = XMFLOAT2(0, 0);
			const float units_per_pixel = camera.ortho_vertical_size / viewport3D.height * 0.5f;
			if (Protean::Input::Down(Protean::Input::MOUSE_BUTTON_RIGHT))
			{
				Protean::Input::SetCursor(Protean::Input::CURSOR_RESIZEALL);
				grabdiff = XMFLOAT2(-mouse.delta_position.x * units_per_pixel, mouse.delta_position.y * units_per_pixel);
				move = XMVectorZero();
			}
			else if(std::abs(pan.x) > 0.0001f || std::abs(pan.y) > 0.0001f)
			{
				grabdiff = XMFLOAT2(pan.x * units_per_pixel, -pan.y * units_per_pixel);
				move = XMVectorZero();
			}

			if (std::abs(grabdiff.x) > 0.0001f || std::abs(grabdiff.y) > 0.0001f || moveLength > 0.0001f)
			{
				editorscene.camera_transform.Translate(move + XMVectorSet(grabdiff.x, grabdiff.y, 0, 0));
				camera.SetDirty();
			}

			editorscene.camera_transform.rotation_local = XMFLOAT4(0, 0, 0, 1);
			editorscene.camera_transform.UpdateTransform();
			XMStoreFloat3(&editorscene.cam_move, move);

			if (!paintToolWnd.IsVisible() && std::abs(currentMouse.z) > 0.25f)
			{
				camera.ortho_vertical_size -= currentMouse.z;
			}
			camera.ortho_vertical_size *= 1.0f + pinch.delta_scale;

			camera.ortho_vertical_size = std::max(0.01f, camera.ortho_vertical_size);
		}
		else if (cameraWnd.fpsCheckBox.GetCheck())
		{
			// FPS Camera
			const float clampedDT = std::min(dt, 0.1f); // if dt > 100 millisec, don't allow the camera to jump too far...

			const float speed = ((Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT) ? 10.0f : 1.0f) + rightTrigger.x * 10.0f) * cameraWnd.movespeedSlider.GetValue() * clampedDT;
			XMVECTOR move = XMLoadFloat3(&editorscene.cam_move);
			XMVECTOR moveNew = XMVectorSet(0, 0, 0, 0);

			if (!Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL))
			{
				// Only move camera if control not pressed
				if (CheckInput(EditorActions_MOVE_CAMERA_LEFT) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_LEFT)) { moveNew += XMVectorSet(-1, 0, 0, 0); }
				if (CheckInput(EditorActions_MOVE_CAMERA_RIGHT) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_RIGHT)) { moveNew += XMVectorSet(1, 0, 0, 0); }
				if (CheckInput(EditorActions_MOVE_CAMERA_FORWARD) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_UP)) { moveNew += XMVectorSet(0, 0, 1, 0); camera.ortho_vertical_size -= 0.1f; }
				if (CheckInput(EditorActions_MOVE_CAMERA_BACKWARD) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_DOWN)) { moveNew += XMVectorSet(0, 0, -1, 0); camera.ortho_vertical_size += 0.1f; }
				if (CheckInput(EditorActions_MOVE_CAMERA_UP) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_2)) { moveNew += XMVectorSet(0, 1, 0, 0); }
				if (CheckInput(EditorActions_MOVE_CAMERA_DOWN) || Protean::Input::Down(Protean::Input::GAMEPAD_BUTTON_1)) { moveNew += XMVectorSet(0, -1, 0, 0); }
				moveNew += XMVectorSet(0, 0, pinch.delta_scale, 0);
				moveNew = XMVector3Normalize(moveNew);
			}
			moveNew += XMVectorSet(leftStick.x, 0, leftStick.y, 0);
			moveNew *= speed;

			move = XMVectorLerp(move, moveNew, std::min(1.0f, cameraWnd.accelerationSlider.GetValue() * clampedDT / 0.0166f)); // smooth the movement a bit
			float moveLength = XMVectorGetX(XMVector3Length(move));
			float moveNewLength = XMVectorGetX(XMVector3Length(moveNew));

			// Only zero out movement when there is no input
			if (moveLength < 0.0001f && moveNewLength < 0.0001f)
			{
				move = XMVectorSet(0, 0, 0, 0);
			}

			if (abs(xDif) + abs(yDif) > 0 || moveLength > 0.0001f)
			{
				XMMATRIX camRot = XMMatrixRotationQuaternion(XMLoadFloat4(&editorscene.camera_transform.rotation_local));
				XMVECTOR move_rot = XMVector3TransformNormal(move, camRot);
				XMFLOAT3 _move;
				XMStoreFloat3(&_move, move_rot);
				editorscene.camera_transform.Translate(_move);
				editorscene.camera_transform.RotateRollPitchYaw(XMFLOAT3(yDif, xDif, 0));
				camera.SetDirty();
			}

			editorscene.camera_transform.UpdateTransform();
			XMStoreFloat3(&editorscene.cam_move, move);

			// Modify camera speed with mouse scroll in FPS mode:
			//	Note: in Paint tool window the scroll is used to modify the brush size
			if (!paintToolWnd.IsVisible() && std::abs(currentMouse.z) > 0.1f)
			{
				float current = cameraWnd.movespeedSlider.GetValue();
				bool scrollingDown = currentMouse.z < 0;
				float newValue;

				if (current < 1.0f || (current == 1.0f && scrollingDown))
				{
					// Below 1.0 threshold: use 0.1 increments
					float add = scrollingDown ? -0.1f : 0.1f;
					newValue = std::max(0.1f, current + add);
				}
				else
				{
					// Above 1.0 threshold
					float increment = current > 10 ? 2.0f : 1.0f;
					float add = scrollingDown ? -increment : increment;
					newValue = std::max(0.1f, std::ceil(current + add));
				}

				cameraWnd.movespeedSlider.SetValue(newValue);
				char txt[256];
				snprintf(txt, arraysize(txt), "Camera speed: %.1f", cameraWnd.movespeedSlider.GetValue());
				save_text_message = txt;
				save_text_alpha = 1.0f;
				main->config.GetSection("camera").Set("move_speed", cameraWnd.movespeedSlider.GetValue());
			}
		}
		else
		{
			// Orbital Camera

			if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT))
			{
				XMVECTOR V = XMVectorAdd(-camera.GetRight() * xDif, camera.GetUp() * yDif) * 10;
				XMFLOAT3 vec;
				XMStoreFloat3(&vec, V);
				editorscene.camera_target.Translate(vec);
			}
			else if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL) || currentMouse.z != 0.0f)
			{
				editorscene.camera_transform.Translate(XMFLOAT3(0, 0, yDif * 4 + currentMouse.z));
				editorscene.camera_transform.translation_local.z = std::min(0.0f, editorscene.camera_transform.translation_local.z);
				camera.ortho_vertical_size = camera.ComputeOrthoVerticalSizeFromPerspective(editorscene.camera_transform.translation_local.z);
				camera.SetDirty();
			}
			else if (abs(xDif) + abs(yDif) > 0)
			{
				editorscene.camera_target.RotateRollPitchYaw(XMFLOAT3(yDif * 2, xDif * 2, 0));
				camera.SetDirty();
			}

			editorscene.camera_target.UpdateTransform();
			editorscene.camera_transform.UpdateTransform_Parented(editorscene.camera_target);
		}

		if (!translator.selected.empty() && CheckInput(EditorActions_FOCUS_ON_SELECTION))
		{
			FocusCameraOnSelected();
		}

		inspector_mode = CheckInput(EditorActions_INSPECTOR_MODE);

		// Begin picking:
		pickRay = Protean::Renderer::GetPickRay((long)currentMouse.x, (long)currentMouse.y, *renderPath, camera);
		{
			hovered = Protean::PickResult();

			const float required_dist = camera.IsOrtho() ? (camera.ortho_vertical_size * 0.025f) : 0.05f;
			auto dist_check = [&](float dis, const XMFLOAT3& position) {
				if (camera.IsOrtho())
				{
					return dis > 0.01f && dis < required_dist && dis < hovered.distance;
				}
				return dis > 0.01f && dis < Protean::Math::Distance(position, pickRay.origin) * required_dist && dis < hovered.distance;
			};

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Light))
			{
				for (size_t i = 0; i < scene.lights.GetCount(); ++i)
				{
					Entity entity = scene.lights.GetEntity(i);
					const LightComponent& light = scene.lights[i];

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), XMLoadFloat3(&light.position));
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, light.position))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
					else
					{
						if (light.GetType() == LightComponent::DIRECTIONAL || light.GetType() == LightComponent::SPOT)
						{
							// Light direction visualizer picking:
							const float dist = Protean::Math::Distance(light.position, camera.Eye);
							float siz = 0.02f * dist;
							const XMMATRIX M =
								XMMatrixScaling(siz, siz, siz) *
								XMMatrixRotationZ(-XM_PIDIV2) *
								XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation)) *
								XMMatrixTranslation(light.position.x, light.position.y, light.position.z)
								;

							const float origin_size = 0.4f * siz;
							const float axis_length = 18;

							XMFLOAT3 base;
							XMFLOAT3 tip;
							XMStoreFloat3(&base, XMVector3Transform(XMVectorSet(0, 0, 0, 1), M));
							XMStoreFloat3(&tip, XMVector3Transform(XMVectorSet(axis_length, 0, 0, 1), M));
							Capsule capsule = Capsule(base, tip, origin_size);
							if (pickRay.intersects(capsule, dis))
							{
								if (dis < hovered.distance)
								{
									hovered = Protean::PickResult();
									hovered.entity = entity;
									hovered.distance = dis;
								}
							}
						}
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Constraint))
			{
				for (size_t i = 0; i < scene.constraints.GetCount(); ++i)
				{
					Entity entity = scene.constraints.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Spline))
			{
				for (size_t i = 0; i < scene.splines.GetCount(); ++i)
				{
					//Protean::Renderer::DrawBox(XMMatrixTranslationFromVector(scene.splines[i].ClosestPointOnSpline(camera.GetEye())));
					for (size_t j = 0; j < scene.splines[i].spline_node_transforms.size(); ++j)
					{
						const TransformComponent& transform = scene.splines[i].spline_node_transforms[j];
						XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
						float dis = XMVectorGetX(disV);
						if (dist_check(dis, transform.GetPosition()))
						{
							hovered = Protean::PickResult();
							hovered.entity = scene.splines[i].spline_node_entities[j];
							hovered.distance = dis;
						}
					}

					Entity entity = scene.splines.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Decal))
			{
				for (size_t i = 0; i < scene.decals.GetCount(); ++i)
				{
					Entity entity = scene.decals.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Force))
			{
				for (size_t i = 0; i < scene.forces.GetCount(); ++i)
				{
					Entity entity = scene.forces.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Emitter))
			{
				for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
				{
					Entity entity = scene.emitters.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Hairparticle))
			{
				for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
				{
					Entity entity = scene.hairs.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_EnvironmentProbe))
			{
				for (size_t i = 0; i < scene.probes.GetCount(); ++i)
				{
					Entity entity = scene.probes.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					if (Sphere(transform.GetPosition(), 1).intersects(pickRay))
					{
						float dis = Protean::Math::Distance(transform.GetPosition(), pickRay.origin);
						if (dis < hovered.distance)
						{
							hovered = Protean::PickResult();
							hovered.entity = entity;
							hovered.distance = dis;
						}
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Camera))
			{
				for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
				{
					Entity entity = scene.cameras.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Armature))
			{
				for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
				{
					Entity entity = scene.armatures.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Sound))
			{
				for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
				{
					Entity entity = scene.sounds.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Video))
			{
				for (size_t i = 0; i < scene.videos.GetCount(); ++i)
				{
					Entity entity = scene.videos.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_VoxelGrid))
			{
				for (size_t i = 0; i < scene.voxel_grids.GetCount(); ++i)
				{
					Entity entity = scene.voxel_grids.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Metadata))
			{
				for (size_t i = 0; i < scene.metadatas.GetCount(); ++i)
				{
					Entity entity = scene.metadatas.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					XMVECTOR disV = XMVector3LinePointDistance(XMLoadFloat3(&pickRay.origin), XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction), transform.GetPositionV());
					float dis = XMVectorGetX(disV);
					if (dist_check(dis, transform.GetPosition()))
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}
			if (bone_picking)
			{
				for (auto& it : bone_picking_items)
				{
					Entity entity = it.first;
					const Capsule& capsule = it.second;
					float dis = -1;
					if (pickRay.intersects(capsule, dis) && dis < hovered.distance)
					{
						hovered = Protean::PickResult();
						hovered.entity = entity;
						hovered.distance = dis;
					}
				}
			}

			if (hovered.entity == INVALID_ENTITY)
			{
				// Object picking only when mouse button down, because it can be slow with high polycount
				if (!translator.IsInteracting() && paintToolWnd.GetMode() == PaintToolWindow::MODE_DISABLED)
				{
					if (
						Protean::Input::Down(Protean::Input::MOUSE_BUTTON_LEFT) ||
						inspector_mode
						)
					{
						hovered = Protean::Pick(pickRay, Protean::FILTER_OBJECT_ALL, ~0u, scene);
					}
				}
			}
		}

		if (hovered.entity != INVALID_ENTITY &&
			CheckInput(EditorActions_DEPTH_OF_FIELD_REFOCUS_TO_POINT) &&
			Protean::Input::Down(Protean::Input::MOUSE_BUTTON_LEFT))
		{
			camera.focal_length = hovered.distance;
			camera.SetDirty();
			cameraWnd.focalLengthSlider.SetValue(camera.focal_length);
			hovered = {};
		}

		// Interactions:
		bool interact_pick_allowed = true;
		{
			// Interact:
			if (CheckInput(EditorActions_RAGDOLL_AND_PHYSICS_IMPULSE_TESTER))
			{
				if (Protean::Input::Press(Protean::Input::MOUSE_BUTTON_MIDDLE))
				{
					// Physics impulse tester:
					Protean::Physics::RayIntersectionResult result = Protean::Physics::Intersects(scene, pickRay);
					if (result.IsValid())
					{
						XMFLOAT3 impulse;
						XMStoreFloat3(&impulse, XMVector3Normalize(XMLoadFloat3(&pickRay.direction)) * 20);
						if (result.humanoid_ragdoll_entity != INVALID_ENTITY)
						{
							// Ragdoll:
							HumanoidComponent* humanoid = scene.humanoids.GetComponent(result.humanoid_ragdoll_entity);
							if (humanoid != nullptr)
							{
								humanoid->SetRagdollPhysicsEnabled(true);
								Protean::Physics::ApplyImpulseAt(*humanoid, result.humanoid_bone, impulse, result.position_local);
							}
						}
						else
						{
							// Rigidbody:
							RigidBodyPhysicsComponent* rigidbody = scene.rigidbodies.GetComponent(result.entity);
							if (rigidbody != nullptr)
							{
								Protean::Physics::ApplyImpulseAt(*rigidbody, impulse, result.position_local);
							}
						}
						interact_pick_allowed = false;
					}
				}
			}
			else
			{
				// Physics pick dragger:
				if (Protean::Input::Down(Protean::Input::MOUSE_BUTTON_MIDDLE))
				{
					Protean::Physics::PickDrag(scene, pickRay, physicsDragOp);
					interact_pick_allowed = !physicsDragOp.IsValid();
				}
				else
				{
					physicsDragOp = {};
				}
			}

			// Decal placement:
			if (componentsWnd.decalWnd.IsEnabled() && componentsWnd.decalWnd.placementCheckBox.GetCheck() && Protean::Input::Press(Protean::Input::MOUSE_BUTTON_LEFT))
			{
				// if not water, put a decal on it:
				Entity entity = scene.Entity_CreateDecal("editorDecal", "");
				// material and decal parameters will be copied from selected:
				if (scene.decals.Contains(componentsWnd.decalWnd.entity))
				{
					*scene.decals.GetComponent(entity) = *scene.decals.GetComponent(componentsWnd.decalWnd.entity);
				}
				if (scene.materials.Contains(componentsWnd.decalWnd.entity))
				{
					*scene.materials.GetComponent(entity) = *scene.materials.GetComponent(componentsWnd.decalWnd.entity);
				}
				// place it on picked surface:
				TransformComponent& transform = *scene.transforms.GetComponent(entity);
				transform.MatrixTransform(hovered.orientation);
				transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
				scene.Component_Attach(entity, hovered.entity);

				Protean::Archive& archive = AdvanceHistory();
				archive << EditorComponent::HISTORYOP_ADD;
				RecordSelection(archive);
				RecordSelection(archive);
				RecordEntity(archive, entity);

				componentsWnd.RefreshEntityTree();
				hovered = {};
			}

			// Other:
			if (interact_pick_allowed && Protean::Input::Down(Protean::Input::MOUSE_BUTTON_MIDDLE))
			{
				hovered = Protean::Pick(pickRay, Protean::FILTER_OBJECT_ALL, ~0u, scene);
				if (hovered.entity != INVALID_ENTITY)
				{
					if (dummy_enabled)
					{
						dummy_pos = hovered.position;
					}
					else
					{
						const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
						if (object != nullptr)
						{
							if (object->GetFilterMask() & Protean::FILTER_WATER)
							{
								// if water, then put a water ripple onto it:
								scene.PutWaterRipple(hovered.position);
							}
							else
							{
								// Check for interactive grass (hair particle that is child of hovered object:
								for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
								{
									Entity entity = scene.hairs.GetEntity(i);
									const Protean::HairParticleSystem& hair = scene.hairs[i];
									HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
									if (hair.meshID == hovered.entity || (hier != nullptr && hier->parentID == hovered.entity))
									{
										XMVECTOR P = XMLoadFloat3(&hovered.position);
										P += XMLoadFloat3(&hovered.normal) * 2;
										if (grass_interaction_entity == INVALID_ENTITY)
										{
											grass_interaction_entity = CreateEntity();
										}
										ForceFieldComponent& force = scene.forces.Create(grass_interaction_entity);
										TransformComponent& transform = scene.transforms.Create(grass_interaction_entity);
										force.type = ForceFieldComponent::Type_Point;
										force.gravity = -40;
										force.range = 2;
										transform.Translate(P);
										break;
									}
								}
							}
						}
					}
				}
			}
		}

		// Select top-level parent...
		if (altmouse_select_parent && hovered.entity != INVALID_ENTITY)
		{
			const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(hovered.entity);
			if (hierarchy != nullptr && hierarchy->parentID != INVALID_ENTITY)
			{
				Entity topParent = hierarchy->parentID;
				const HierarchyComponent* parentHierarchy = scene.hierarchy.GetComponent(topParent);
				while (parentHierarchy != nullptr && parentHierarchy->parentID != INVALID_ENTITY)
				{
					topParent = parentHierarchy->parentID;
					parentHierarchy = scene.hierarchy.GetComponent(topParent);
				}

				Protean::Archive& archive = AdvanceHistory(true);
				archive << HISTORYOP_SELECTION;
				// record PREVIOUS selection state...
				RecordSelection(archive);

				// Select the top-level parent entity:
				translator.selected.clear();
				AddSelected(topParent);

				// record NEW selection state...
				RecordSelection(archive);

				componentsWnd.RefreshEntityTree();
			}
		}

		// Select...
		if (leftmouse_select || selectAll || clear_selected)
		{
			Protean::Archive& archive = AdvanceHistory(true);
			archive << HISTORYOP_SELECTION;
			// record PREVIOUS selection state...
			RecordSelection(archive);

			if (selectAll)
			{
				// Add everything to selection:
				selectAll = false;
				ClearSelected();

				selectAllStorage.clear();
				scene.FindAllEntities(selectAllStorage);
				for (auto& entity : selectAllStorage)
				{
					AddSelected(entity);
				}
			}
			else if (hovered.entity != INVALID_ENTITY)
			{
				// Add the hovered item to the selection:

				if (!translator.selected.empty() && (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL)))
				{
					// Union selection:
					Protean::vector<Protean::PickResult> saved = translator.selected;
					translator.selected.clear();
					for (const Protean::PickResult& picked : saved)
					{
						AddSelected(picked);
					}
					AddSelected(hovered);
				}
				else
				{
					// Replace selection:
					translator.selected.clear();
					AddSelected(hovered);
				}
			}
			else
			{
				clear_selected = true;
			}

			if (clear_selected)
			{
				ClearSelected();
			}


			// record NEW selection state...
			RecordSelection(archive);

			componentsWnd.RefreshEntityTree();
		}

	}

	main->infoDisplay.colorgrading_helper = false;

	// Control operations...
	if (!GetGUI().IsTyping() && !GetGUI().HasFocus() && (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RCONTROL)))
	{
		bool isCtrlDown = Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LCONTROL) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RCONTROL);
		bool isShiftDown = Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_LSHIFT) || Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_RSHIFT);

		// Color Grading helper
		if (CheckInput(EditorActions_COLOR_GRADING_REFERENCE))
		{
			main->infoDisplay.colorgrading_helper = true;
		}
		else
		{
			main->infoDisplay.colorgrading_helper = false;
		}

		// Toggle wireframe mode
		if (CheckInput(EditorActions_WIREFRAME_MODE))
		{
			Protean::Renderer::WIREFRAME_MODE currentMode = Protean::Renderer::GetWireframeMode();
			Protean::Renderer::WIREFRAME_MODE mode;
			switch (currentMode)
			{
			case Protean::Renderer::WIREFRAME_DISABLED:
				mode = Protean::Renderer::WIREFRAME_ONLY;
				break;
			case Protean::Renderer::WIREFRAME_ONLY:
				mode = Protean::Renderer::WIREFRAME_OVERLAY;
				break;
			case Protean::Renderer::WIREFRAME_OVERLAY:
			default:
				mode = Protean::Renderer::WIREFRAME_DISABLED;
				break;
			}
			Protean::Renderer::SetWireframeMode(mode);
			generalWnd.wireFrameComboBox.SetSelected((int)mode);
		}

		// Enable transform tool
		if (CheckInput(EditorActions_ENABLE_TRANSFORM_TOOL))
		{
			translator.SetEnabled(!translator.IsEnabled());
		}

		if (CheckInput(EditorActions_SAVE_SCENE_AS))
		{
			SaveAs();
		}
		else if (CheckInput(EditorActions_SAVE_SCENE))
		{
			if (!GetCurrentEditorScene().path.empty())
			{
				Save(GetCurrentEditorScene().path);
			}
			else
			{
				SaveAs();
			}
		}

		// Select All
		if (CheckInput(EditorActions_SELECT_ALL_ENTITIES))
		{
			selectAll = true;
		}

		// Copy
		if (CheckInput(EditorActions_COPY_ACTION))
		{
			auto& prevSel = translator.selectedEntitiesNonRecursive;

			EntitySerializer seri;
			clipboard.SetReadModeAndResetPos(false);
			clipboard << prevSel.size();
			for (auto& x : prevSel)
			{
				scene.Entity_Serialize(clipboard, seri, x);
			}
		}

		// Cut
		if (CheckInput(EditorActions_CUT_ACTION))
		{
			auto& prevSel = translator.selectedEntitiesNonRecursive;

			EntitySerializer seri;
			clipboard.SetReadModeAndResetPos(false);
			clipboard << prevSel.size();
			for (auto& x : prevSel)
			{
				scene.Entity_Serialize(clipboard, seri, x);
			}

			deleting = true;
		}

		// Paste
		if (CheckInput(EditorActions_PASTE_ACTION))
		{
			Protean::Archive& archive = AdvanceHistory();
			archive << HISTORYOP_ADD;
			RecordSelection(archive);

			ClearSelected();

			EntitySerializer seri;
			clipboard.SetReadModeAndResetPos(true);
			size_t count;
			clipboard >> count;
			Protean::vector<Entity> addedEntities;
			for (size_t i = 0; i < count; ++i)
			{
				Protean::PickResult picked;
				picked.entity = scene.Entity_Serialize(clipboard, seri, INVALID_ENTITY, Scene::EntitySerializeFlags_RECURSIVE);
				AddSelected(picked);
				addedEntities.push_back(picked.entity);
			}

			RecordSelection(archive);
			RecordEntity(archive, addedEntities);

			componentsWnd.RefreshEntityTree();
		}

		// Duplicate/Put Instances
		if (CheckInput(EditorActions_PLACE_INSTANCES))
		{
			Protean::vector<Entity> addedEntities;
			EntitySerializer seri;
			clipboard.SetReadModeAndResetPos(true);
			size_t count;
			clipboard >> count;
			for (size_t i = 0; i < count; ++i)
			{
				Entity entity = scene.Entity_Serialize(clipboard, seri, INVALID_ENTITY, Scene::EntitySerializeFlags_RECURSIVE | Scene::EntitySerializeFlags_KEEP_INTERNAL_ENTITY_REFERENCES);
				const HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
				if (hier != nullptr)
				{
					scene.Component_Detach(entity);
				}
				TransformComponent* transform = scene.transforms.GetComponent(entity);
				if (transform != nullptr)
				{
					transform->translation_local = {};
#if 0
					// orient around surface normal:
					transform->MatrixTransform(hovered.orientation);
#else
					// orient in random vertical rotation only:
					transform->RotateRollPitchYaw(XMFLOAT3(0, Protean::Random::GetRandom(XM_PI), 0));
					transform->Translate(hovered.position);
#endif
					transform->UpdateTransform();
				}
				if (hier != nullptr)
				{
					scene.Component_Attach(entity, hier->parentID);
				}
				addedEntities.push_back(entity);
				}

			Protean::Archive& archive = AdvanceHistory();
			archive << HISTORYOP_ADD;
			// because selection didn't change here, we record same selection state twice, it's not a bug:
			RecordSelection(archive);
			RecordSelection(archive);
			RecordEntity(archive, addedEntities);

			componentsWnd.RefreshEntityTree();
		}

		// Undo
		if (CheckInput(EditorActions_UNDO_ACTION))
		{
			ConsumeHistoryOperation(true);

			componentsWnd.RefreshEntityTree();
		}

		// Redo
		if (CheckInput(EditorActions_REDO_ACTION))
		{
			ConsumeHistoryOperation(false);

			componentsWnd.RefreshEntityTree();
		}
	}

	// These keys work but for some reason in my keyboard 1 returns TAB and 2 returns TILDE keys.
	if (!Protean::Backlog::isActive() && !GetGUI().IsTyping())
	{
		if (CheckInput(EditorActions_MOVE_TOGGLE_ACTION))
		{
			translator.isTranslator = !translator.isTranslator;
			translator.isScalator = false;
			translator.isRotator = false;
		}
		else if (CheckInput(EditorActions_ROTATE_TOGGLE_ACTION))
		{
			translator.isRotator = !translator.isRotator;
			translator.isScalator = false;
			translator.isTranslator = false;
		}
		else if (CheckInput(EditorActions_SCALE_TOGGLE_ACTION))
		{
			translator.isScalator = !translator.isScalator;
			translator.isTranslator = false;
			translator.isRotator = false;
		}
		else if (CheckInput(EditorActions_LOCAL_GLOBAL_TOGGLE_ACTION))
		{
			translator.isLocalSpace = !translator.isLocalSpace;
			UpdateLocalGlobalButton();
		}
	}

	// Delete
	if (deleting)
	{
		deleting = false;
		Protean::Archive& archive = AdvanceHistory();
		archive << HISTORYOP_DELETE;
		RecordSelection(archive);

		archive << translator.selectedEntitiesNonRecursive;
		EntitySerializer seri;
		for (auto& x : translator.selectedEntitiesNonRecursive)
		{
			scene.Entity_Serialize(archive, seri, x);
		}
		for (auto& x : translator.selectedEntitiesNonRecursive)
		{
			scene.Entity_Remove(x);
		}

		ClearSelected();

		componentsWnd.RefreshEntityTree();
	}

	// Update window data bindings...
	if (translator.selected.empty())
	{
		cameraWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.objectWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.emitterWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.hairWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.meshWnd.SetEntity(INVALID_ENTITY, -1);
		componentsWnd.materialWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.soundWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.lightWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.videoWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.decalWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.envProbeWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.forceFieldWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.springWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.ikWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.transformWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.layerWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.nameWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.weatherWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.animWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.scriptWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.rigidWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.softWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.colliderWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.hierarchyWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.cameraComponentWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.expressionWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.armatureWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.humanoidWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.terrainWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.spriteWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.fontWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.voxelGridWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.metadataWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.constraintWnd.SetEntity(INVALID_ENTITY);
		componentsWnd.gaussiansplatWnd.SetEntity(INVALID_ENTITY);
	}
	else
	{
		const Protean::PickResult& picked = translator.selected.back();

		assert(picked.entity != INVALID_ENTITY);
		cameraWnd.SetEntity(picked.entity);
		componentsWnd.emitterWnd.SetEntity(picked.entity);
		componentsWnd.hairWnd.SetEntity(picked.entity);
		componentsWnd.lightWnd.SetEntity(picked.entity);
		componentsWnd.soundWnd.SetEntity(picked.entity);
		componentsWnd.videoWnd.SetEntity(picked.entity);
		componentsWnd.decalWnd.SetEntity(picked.entity);
		componentsWnd.envProbeWnd.SetEntity(picked.entity);
		componentsWnd.forceFieldWnd.SetEntity(picked.entity);
		componentsWnd.springWnd.SetEntity(picked.entity);
		componentsWnd.ikWnd.SetEntity(picked.entity);
		componentsWnd.transformWnd.SetEntity(picked.entity);
		componentsWnd.layerWnd.SetEntity(picked.entity);
		componentsWnd.nameWnd.SetEntity(picked.entity);
		componentsWnd.weatherWnd.SetEntity(picked.entity);
		componentsWnd.animWnd.SetEntity(picked.entity);
		componentsWnd.scriptWnd.SetEntity(picked.entity);
		componentsWnd.rigidWnd.SetEntity(picked.entity);
		componentsWnd.colliderWnd.SetEntity(picked.entity);
		componentsWnd.hierarchyWnd.SetEntity(picked.entity);
		componentsWnd.cameraComponentWnd.SetEntity(picked.entity);
		componentsWnd.expressionWnd.SetEntity(picked.entity);
		componentsWnd.armatureWnd.SetEntity(picked.entity);
		componentsWnd.humanoidWnd.SetEntity(picked.entity);
		componentsWnd.terrainWnd.SetEntity(picked.entity);
		componentsWnd.spriteWnd.SetEntity(picked.entity);
		componentsWnd.fontWnd.SetEntity(picked.entity);
		componentsWnd.voxelGridWnd.SetEntity(picked.entity);
		componentsWnd.metadataWnd.SetEntity(picked.entity);
		componentsWnd.constraintWnd.SetEntity(picked.entity);
		componentsWnd.gaussiansplatWnd.SetEntity(picked.entity);

		bool found_object = false;
		bool found_mesh = false;
		bool found_soft = false;
		bool found_material = false;
		bool found_spline = false;

		const ObjectComponent* object = scene.objects.GetComponent(picked.entity);
		if (object != nullptr) // maybe it was deleted...
		{
			found_object = true;
			componentsWnd.objectWnd.SetEntity(picked.entity);
			componentsWnd.meshWnd.SetEntity(object->meshID, picked.subsetIndex);
			componentsWnd.softWnd.SetEntity(object->meshID);

			found_mesh = scene.meshes.Contains(object->meshID);
			found_soft = scene.softbodies.Contains(object->meshID);

			const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh != nullptr && (int)mesh->subsets.size() > picked.subsetIndex)
			{
				found_material = scene.materials.Contains(mesh->subsets[picked.subsetIndex].materialID);
				componentsWnd.materialWnd.SetEntity(mesh->subsets[picked.subsetIndex].materialID);
			}
		}

		for (auto& x : translator.selected)
		{
			if (!found_object && scene.objects.Contains(x.entity))
			{
				componentsWnd.objectWnd.SetEntity(x.entity);
				found_object = true;
			}
			if (!found_mesh && scene.meshes.Contains(x.entity))
			{
				componentsWnd.meshWnd.SetEntity(x.entity, 0);
				found_mesh = true;
			}
			if (!found_soft && scene.softbodies.Contains(x.entity))
			{
				componentsWnd.softWnd.SetEntity(x.entity);
				found_soft = true;
			}
			if (!found_material && scene.materials.Contains(x.entity))
			{
				componentsWnd.materialWnd.SetEntity(x.entity);
				found_material = true;
			}

			// Indirectly bind selected to spline window if selected is part of a spline:
			for (size_t i = 0; i < scene.splines.GetCount(); ++i)
			{
				const SplineComponent& spline = scene.splines[i];
				Entity spline_entity = scene.splines.GetEntity(i);
				if (x.entity == spline_entity)
				{
					found_spline = true;
					componentsWnd.splineWnd.SetEntity(spline_entity);
					break;
				}
				for (size_t j = 0; j < spline.spline_node_entities.size(); ++j)
				{
					if (x.entity == spline.spline_node_entities[j])
					{
						found_spline = true;
						componentsWnd.splineWnd.SetEntity(spline_entity);
						break;
					}
				}
			}
		}

		if (!found_object)
		{
			componentsWnd.objectWnd.SetEntity(INVALID_ENTITY);
		}
		if (!found_mesh)
		{
			componentsWnd.meshWnd.SetEntity(INVALID_ENTITY, -1);
		}
		if (!found_soft)
		{
			componentsWnd.softWnd.SetEntity(INVALID_ENTITY);
		}
		if (!found_material)
		{
			componentsWnd.materialWnd.SetEntity(INVALID_ENTITY);
		}
		if (!found_spline)
		{
			componentsWnd.splineWnd.SetEntity(INVALID_ENTITY);
		}

	}

	// Clear highlight state:
	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		scene.materials[i].SetUserStencilRef(EDITORSTENCILREF_CLEAR);
	}
	for (size_t i = 0; i < scene.objects.GetCount(); ++i)
	{
		scene.objects[i].SetUserStencilRef(EDITORSTENCILREF_CLEAR);
	}
	for (auto& x : translator.selected)
	{
		ObjectComponent* object = scene.objects.GetComponent(x.entity);
		if (object != nullptr) // maybe it was deleted...
		{
			object->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT_OBJECT);
			if (x.subsetIndex >= 0)
			{
				const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh != nullptr && (int)mesh->subsets.size() > x.subsetIndex)
				{
					MaterialComponent* material = scene.materials.GetComponent(mesh->subsets[x.subsetIndex].materialID);
					if (material != nullptr)
					{
						material->SetUserStencilRef(EDITORSTENCILREF_HIGHLIGHT_MATERIAL);
					}
				}
			}
		}
	}

	if (translator.IsDragEnded())
	{
		EntitySerializer seri;
		Protean::Archive& archive = AdvanceHistory();
		archive << HISTORYOP_TRANSLATOR;
		archive << translator.isTranslator;
		archive << translator.isRotator;
		archive << translator.isScalator;
		translator.transform_start.Serialize(archive, seri);
		translator.transform.Serialize(archive, seri);
		archive << translator.matrices_start;
		archive << translator.matrices_current;
	}

	componentsWnd.emitterWnd.UpdateData();
	componentsWnd.hairWnd.UpdateData();
	componentsWnd.humanoidWnd.UpdateHumanoids();

	// Follow camera proxy:
	if (cameraWnd.followCheckBox.IsEnabled() && cameraWnd.followCheckBox.GetCheck())
	{
		TransformComponent* proxy = scene.transforms.GetComponent(cameraWnd.entity);
		if (proxy != nullptr)
		{
			editorscene.camera_transform.Lerp(editorscene.camera_transform, *proxy, 1.0f - cameraWnd.followSlider.GetValue());
			editorscene.camera_transform.UpdateTransform();
		}

		CameraComponent* proxy_camera = scene.cameras.GetComponent(cameraWnd.entity);
		if (proxy_camera != nullptr)
		{
			editorscene.camera.Lerp(editorscene.camera, *proxy_camera, 1.0f - cameraWnd.followSlider.GetValue());
		}
	}

	if (!drive_mode)
	{
		camera.TransformCamera(editorscene.camera_transform);
		camera.UpdateCamera();
	}

	if (pathtracer != nullptr)
	{
		pathtracer->setTargetSampleCount((int)graphicsWnd.pathTraceTargetSlider.GetValue());

		std::string ss;
		ss += "Sample count: " + std::to_string(pathtracer->getCurrentSampleCount()) + "\n";
		ss += "Trace progress: " + std::to_string(int(pathtracer->getProgress() * 100)) + "%\n";
		if (pathtracer->isDenoiserAvailable())
		{
			if (pathtracer->getDenoiserProgress() > 0)
			{
				ss += "Denoiser progress: " + std::to_string(int(pathtracer->getDenoiserProgress() * 100)) + "%\n";
			}
		}
		else
		{
			ss += "Denoiser not available!\nTo find out how to enable the denoiser, visit the documentation.";
		}
		graphicsWnd.pathTraceStatisticsLabel.SetText(ss);
	}

	Protean::Profiler::EndRange(profrange);

	RenderPath2D::Update(dt);

	UpdateDynamicWidgets();

	ResizeViewport3D();
	renderPath->colorspace = colorspace;
	renderPath->Update(dt);

	if (navtest_enabled)
	{
		if (Protean::Input::Down(Protean::Input::MOUSE_BUTTON_MIDDLE))
		{
			hovered = Protean::Pick(pickRay, Protean::FILTER_OBJECT_ALL, ~0u, scene);
		}
		if (hovered.entity != INVALID_ENTITY && Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_F5))
		{
			navtest_start_pick = hovered;
		}
		if (hovered.entity != INVALID_ENTITY && Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_F6))
		{
			navtest_goal_pick = hovered;
		}
		if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_F7) && Protean::Input::Down(Protean::Input::MOUSE_BUTTON_MIDDLE))
		{
			navtest_start_pick.entity = INVALID_ENTITY;
			XMStoreFloat3(&navtest_start_pick.position, XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction) * 2);
		}
		if (Protean::Input::Down(Protean::Input::KEYBOARD_BUTTON_F8) && Protean::Input::Down(Protean::Input::MOUSE_BUTTON_MIDDLE))
		{
			navtest_goal_pick.entity = INVALID_ENTITY;
			XMStoreFloat3(&navtest_goal_pick.position, XMLoadFloat3(&pickRay.origin) + XMLoadFloat3(&pickRay.direction) * 2);
		}

		navtest_pathquery.flying = false;
		if (
			navtest_start_pick.entity != INVALID_ENTITY &&
			navtest_goal_pick.entity != INVALID_ENTITY
			)
		{
			navtest_start_pick.position = scene.GetPositionOnSurface(
				navtest_start_pick.entity,
				navtest_start_pick.vertexID0,
				navtest_start_pick.vertexID1,
				navtest_start_pick.vertexID2,
				navtest_start_pick.bary
			);
			navtest_goal_pick.position = scene.GetPositionOnSurface(
				navtest_goal_pick.entity,
				navtest_goal_pick.vertexID0,
				navtest_goal_pick.vertexID1,
				navtest_goal_pick.vertexID2,
				navtest_goal_pick.bary
			);
		}
		else if(
			navtest_start_pick.entity == INVALID_ENTITY &&
			navtest_goal_pick.entity == INVALID_ENTITY
			)
		{
			navtest_pathquery.flying = true;
		}

		for (size_t i = 0; i < scene.voxel_grids.GetCount(); ++i)
		{
			const Protean::VoxelGrid& voxelgrid = scene.voxel_grids[i];
			AABB aabb = voxelgrid.get_aabb();
			if (aabb.intersects(navtest_start_pick.position) && aabb.intersects(navtest_goal_pick.position))
			{
				auto range = Protean::Profiler::BeginRangeCPU("NAVTEST PATHQUERY");
				navtest_pathquery.process(
					navtest_start_pick.position,
					navtest_goal_pick.position,
					voxelgrid
				);
				Protean::Profiler::EndRange(range);
				Protean::Renderer::DrawPathQuery(&navtest_pathquery);
				break;
			}
		}

	}

	bool force_collider_visualizer = false;
	bool force_spring_visualizer = false;
	for (auto& x : translator.selected)
	{
		if (scene.colliders.Contains(x.entity))
		{
			force_collider_visualizer = true;
		}
		if (scene.springs.Contains(x.entity))
		{
			force_spring_visualizer = true;
		}
	}
	if (force_collider_visualizer)
	{
		Protean::Renderer::SetToDrawDebugColliders(true);
	}
	else
	{
		Protean::Renderer::SetToDrawDebugColliders(generalWnd.colliderVisCheckBox.GetCheck());
	}
	if (force_spring_visualizer)
	{
		Protean::Renderer::SetToDrawDebugSprings(true);
	}
	else
	{
		Protean::Renderer::SetToDrawDebugSprings(generalWnd.springVisCheckBox.GetCheck());
	}

	if (generalWnd.focusModeCheckBox.GetCheck() || themeEditorWnd.waveColor.getA() == 0)
	{
		topmenuWnd.background_overlay = {};
		componentsWnd.background_overlay = {};
		generalWnd.background_overlay = {};
		graphicsWnd.background_overlay = {};
		paintToolWnd.background_overlay = {};
		cameraWnd.background_overlay = {};
		materialPickerWnd.background_overlay = {};
	}
	else
	{
		topmenuWnd.background_overlay = gui_background_effect;
		componentsWnd.background_overlay = gui_background_effect;
		generalWnd.background_overlay = gui_background_effect;
		graphicsWnd.background_overlay = gui_background_effect;
		paintToolWnd.background_overlay = gui_background_effect;
		cameraWnd.background_overlay = gui_background_effect;
		materialPickerWnd.background_overlay = gui_background_effect;
	}
}
void EditorComponent::PostUpdate()
{
	RenderPath2D::PostUpdate();

	renderPath->PostUpdate();

	Scene& scene = GetCurrentScene();
	EditorScene& editorscene = GetCurrentEditorScene();
	CameraComponent& camera = editorscene.camera;

	// This needs to be after scene was updated fully by EditorComponent's renderPath
	//	Because this will just render the scene without updating its resources
	if (!componentsWnd.cameraComponentWnd.IsCollapsed() && renderPath->getSceneUpdateEnabled()) // only update preview if scene was updated at all by main renderPath
	{
		componentsWnd.cameraComponentWnd.preview.RenderPreview();
	}

	if (componentsWnd.voxelGridWnd.debugAllCheckBox.GetCheck())
	{
		// Draw all voxel grids:
		for (size_t i = 0; i < scene.voxel_grids.GetCount(); ++i)
		{
			Protean::Renderer::DrawVoxelGrid(&scene.voxel_grids[i]);
		}
	}
	else
	{
		// Draw only selected:
		if (scene.voxel_grids.Contains(componentsWnd.voxelGridWnd.entity))
		{
			Protean::Renderer::DrawVoxelGrid(scene.voxel_grids.GetComponent(componentsWnd.voxelGridWnd.entity));
		}
	}

	// Drive mode camera override is done in PostUpdate so that most recent physics updates are available:
	if (drive_mode)
	{
		RigidBodyPhysicsComponent* rigidbody = scene.rigidbodies.GetComponent(componentsWnd.rigidWnd.driving_entity);
		if (rigidbody != nullptr && rigidbody->IsVehicle())
		{
			TransformComponent* vehicle_transform = scene.transforms.GetComponent(componentsWnd.rigidWnd.driving_entity);
			if (vehicle_transform != nullptr)
			{
				XMVECTOR P = vehicle_transform->GetPositionV();
				XMVECTOR Q = vehicle_transform->GetRotationV();
				XMMATRIX W = XMMatrixIdentity();
				if (rigidbody->vehicle.type == RigidBodyPhysicsComponent::Vehicle::Type_Car)
				{
					W = XMMatrixTranslation(0, 0.5f, -drive_cam_dist) * XMMatrixRotationX(XM_PI * 0.08f) * XMMatrixRotationY(drive_orbit_horizontal) * XMMatrixRotationQuaternion(Q) * XMMatrixTranslationFromVector(P);
				}
				else if (rigidbody->vehicle.type == RigidBodyPhysicsComponent::Vehicle::Type_Motorcycle)
				{
					W = XMMatrixTranslation(0, 1.5f, -drive_cam_dist) * XMMatrixRotationX(XM_PI * 0.1f) * XMMatrixRotationY(drive_orbit_horizontal) * XMMatrixRotationQuaternion(Q) * XMMatrixTranslationFromVector(P);
				}
				camera.TransformCamera(W);
				camera.UpdateCamera();
			}
		}
	}

	if (generalWnd.splineVisCheckBox.GetCheck())
	{
		spline_renderer.texMulAdd2 = XMFLOAT4(1, 1, spline_renderer.texMulAdd2.z - scene.dt, 0);
		spline_renderer.color = dummyColor;
		spline_renderer.depth_soften = 1;
		spline_renderer.width = 0.1f;
		spline_renderer.Clear();
		for (size_t i = 0; i < scene.splines.GetCount(); ++i)
		{
			const SplineComponent& spline = scene.splines[i];
			for (size_t j = 0; j < spline.spline_node_transforms.size(); ++j)
			{
				const TransformComponent& node_transform = spline.spline_node_transforms[j];
				spline_renderer.AddPoint(node_transform.GetPosition(), std::max(0.0f, node_transform.GetScale().x), XMFLOAT4(1, 1, 1, 1), spline.IsDrawAligned() ? node_transform.GetRotation() : XMFLOAT4(0, 0, 0, 0));
			}
			spline_renderer.Cut(spline.IsLooped());
		}
	}
}
void EditorComponent::PreRender()
{
	RenderPath2D::PreRender();

	renderPath->PreRender();
}
void EditorComponent::Render() const
{
	const Scene& scene = GetCurrentScene();
	const CameraComponent& camera = GetCurrentEditorScene().camera;

	// remove camera jittering
	CameraComponent cam = *renderPath->camera;
	cam.jitter = XMFLOAT2(0, 0);
	cam.UpdateCamera();
	const XMMATRIX VP = cam.GetViewProjection();

	// Hovered item boxes:
	if (GetGUI().IsVisible())
	{
		if (hovered.entity != INVALID_ENTITY)
		{
			const ObjectComponent* object = scene.objects.GetComponent(hovered.entity);
			if (object != nullptr)
			{
				const AABB& aabb = scene.aabb_objects[scene.objects.GetIndex(hovered.entity)];

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				Protean::Renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const LightComponent* light = scene.lights.GetComponent(hovered.entity);
			if (light != nullptr)
			{
				const AABB& aabb = scene.aabb_lights[scene.lights.GetIndex(hovered.entity)];

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				Protean::Renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0, 0.5f));
			}

			const DecalComponent* decal = scene.decals.GetComponent(hovered.entity);
			if (decal != nullptr)
			{
				Protean::Renderer::DrawBox(decal->world, XMFLOAT4(0.5f, 0, 0.5f, 0.5f));
			}

			const EnvironmentProbeComponent* probe = scene.probes.GetComponent(hovered.entity);
			if (probe != nullptr)
			{
				const AABB& aabb = scene.aabb_probes[scene.probes.GetIndex(hovered.entity)];

				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, aabb.getAsBoxMatrix());
				Protean::Renderer::DrawBox(hoverBox, XMFLOAT4(0.5f, 0.5f, 0.5f, 0.5f));
			}

			const Protean::HairParticleSystem* hair = scene.hairs.GetComponent(hovered.entity);
			if (hair != nullptr)
			{
				XMFLOAT4X4 hoverBox;
				XMStoreFloat4x4(&hoverBox, hair->aabb.getAsBoxMatrix());
				Protean::Renderer::DrawBox(hoverBox, XMFLOAT4(0, 0.5f, 0, 0.5f));
			}
		}
	}

	// Selected items box:
	if (GetGUI().IsVisible() && !translator.selected.empty())
	{
		AABB selectedAABB = AABB(
			XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
			XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()));
		for (auto& picked : translator.selected)
		{
			if (picked.entity != INVALID_ENTITY)
			{
				const ObjectComponent* object = scene.objects.GetComponent(picked.entity);
				if (object != nullptr)
				{
					const AABB& aabb = scene.aabb_objects[scene.objects.GetIndex(picked.entity)];
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const LightComponent* light = scene.lights.GetComponent(picked.entity);
				if (light != nullptr)
				{
					const AABB& aabb = scene.aabb_lights[scene.lights.GetIndex(picked.entity)];
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const DecalComponent* decal = scene.decals.GetComponent(picked.entity);
				if (decal != nullptr)
				{
					const AABB& aabb = scene.aabb_decals[scene.decals.GetIndex(picked.entity)];
					selectedAABB = AABB::Merge(selectedAABB, aabb);

					// also display decal OBB:
					XMFLOAT4X4 selectionBox;
					selectionBox = decal->world;
					Protean::Renderer::DrawBox(selectionBox, XMFLOAT4(1, 0, 1, 1));
				}

				const EnvironmentProbeComponent* probe = scene.probes.GetComponent(picked.entity);
				if (probe != nullptr)
				{
					const AABB& aabb = scene.aabb_probes[scene.probes.GetIndex(picked.entity)];
					selectedAABB = AABB::Merge(selectedAABB, aabb);
				}

				const Protean::HairParticleSystem* hair = scene.hairs.GetComponent(picked.entity);
				if (hair != nullptr)
				{
					selectedAABB = AABB::Merge(selectedAABB, hair->aabb);
				}

			}
		}

		XMFLOAT4X4 selectionBox;
		XMStoreFloat4x4(&selectionBox, selectedAABB.getAsBoxMatrix());
		Protean::Renderer::DrawBox(selectionBox, XMFLOAT4(1, 1, 1, 1));
	}

	renderPath->Render();

	// Editor custom render:
	if (GetGUI().IsVisible())
	{
		GraphicsDevice* device = Protean::Graphics::GetDevice();
		CommandList cmd = device->BeginCommandList();
		device->EventBegin("Editor", cmd);

		// Selection outline:
		if (renderPath->GetDepthStencil() != nullptr && !translator.selected.empty())
		{
			device->EventBegin("Selection Outline Mask", cmd);

			Viewport vp;
			vp.width = (float)rt_selectionOutline[0].GetDesc().width;
			vp.height = (float)rt_selectionOutline[0].GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			Protean::Image::Params fx;
			fx.enableFullScreen();
			fx.stencilComp = Protean::Image::STENCILMODE::STENCILMODE_EQUAL;

			// We will specify the stencil ref in user-space, don't care about engine stencil refs here:
			//	Otherwise would need to take into account engine ref and draw multiple permutations of stencil refs.
			fx.stencilRefMode = Protean::Image::STENCILREFMODE_USER;

			// Materials outline:
			{
				if (renderPath->getMSAASampleCount() > 1)
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(
							&rt_selectionOutline_MSAA,
							RenderPassImage::LoadOp_CLEAR,
							RenderPassImage::StoreOp_DONTCARE
						),
						RenderPassImage::Resolve(&rt_selectionOutline[0]),
						RenderPassImage::DepthStencil(
							renderPath->GetDepthStencil(),
							RenderPassImage::LoadOp_LOAD,
							RenderPassImage::StoreOp_STORE
						),
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}
				else
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&rt_selectionOutline[0], RenderPassImage::LoadOp_CLEAR),
						RenderPassImage::DepthStencil(
							renderPath->GetDepthStencil(),
							RenderPassImage::LoadOp_LOAD,
							RenderPassImage::StoreOp_STORE
						),
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}

				// Draw solid blocks of selected materials
				fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_MATERIAL;
				Protean::Image::Draw(nullptr, fx, cmd);

				device->RenderPassEnd(cmd);
			}

			// Objects outline:
			{
				if (renderPath->getMSAASampleCount() > 1)
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&rt_selectionOutline_MSAA, RenderPassImage::LoadOp_CLEAR, RenderPassImage::StoreOp_DONTCARE),
						RenderPassImage::Resolve(&rt_selectionOutline[1]),
						RenderPassImage::DepthStencil(
							renderPath->GetDepthStencil(),
							RenderPassImage::LoadOp_LOAD,
							RenderPassImage::StoreOp_STORE
						),
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}
				else
				{
					RenderPassImage rp[] = {
						RenderPassImage::RenderTarget(&rt_selectionOutline[1], RenderPassImage::LoadOp_CLEAR),
						RenderPassImage::DepthStencil(
							renderPath->GetDepthStencil(),
							RenderPassImage::LoadOp_LOAD,
							RenderPassImage::StoreOp_STORE
						),
					};
					device->RenderPassBegin(rp, arraysize(rp), cmd);
				}

				// Draw solid blocks of selected objects
				fx.stencilRef = EDITORSTENCILREF_HIGHLIGHT_OBJECT;
				Protean::Image::Draw(nullptr, fx, cmd);

				device->RenderPassEnd(cmd);
			}

			device->EventEnd(cmd);
		}

		// Reference dummy render:
		if(dummy_enabled)
		{
			device->EventBegin("Reference Dummy", cmd);
			RenderPassImage rp[] = {
				RenderPassImage::RenderTarget(&rt_dummyOutline, RenderPassImage::LoadOp_CLEAR),
			};
			device->RenderPassBegin(rp, arraysize(rp), cmd);

			Viewport vp;
			vp.width = (float)rt_dummyOutline.GetDesc().width;
			vp.height = (float)rt_dummyOutline.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			if (dummy_male)
			{
				dummy::draw_male(XMMatrixTranslation(dummy_pos.x, dummy_pos.y, dummy_pos.z), VP, XMFLOAT4(1, 1, 1, 1), false, cmd, camera.frustum);
			}
			else
			{
				dummy::draw_female(XMMatrixTranslation(dummy_pos.x, dummy_pos.y, dummy_pos.z), VP, XMFLOAT4(1, 1, 1, 1), false, cmd, camera.frustum);
			}

			device->RenderPassEnd(cmd);
			device->EventEnd(cmd);
		}

		if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Metadata) && scene.metadatas.GetCount() > 0)
		{
			device->EventBegin("Metadata Dummy", cmd);
			if (renderPath->getMSAASampleCount() > 1)
			{
				RenderPassImage rp[] = {
					RenderPassImage::RenderTarget(&rt_metadataDummies_MSAA, RenderPassImage::LoadOp_CLEAR, RenderPassImage::StoreOp_DONTCARE, ResourceState_RENDERTARGET, ResourceState_RENDERTARGET),
					RenderPassImage::Resolve(&rt_metadataDummies),
					RenderPassImage::DepthStencil(renderPath->GetDepthStencil()),
				};
				device->RenderPassBegin(rp, arraysize(rp), cmd);
			}
			else
			{
				RenderPassImage rp[] = {
					RenderPassImage::RenderTarget(&rt_metadataDummies, RenderPassImage::LoadOp_CLEAR),
					RenderPassImage::DepthStencil(renderPath->GetDepthStencil()),
				};
				device->RenderPassBegin(rp, arraysize(rp), cmd);
			}

			Viewport vp;
			vp.width = (float)rt_metadataDummies.GetDesc().width;
			vp.height = (float)rt_metadataDummies.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			static float tim = 0;
			tim += scene.dt;
			float sca = lerp(0.4f, 0.8f, std::abs(std::sin(tim * 2)));

			XMMATRIX ROT = XMLoadFloat3x3(&camera.rotationMatrix);
			Protean::Font::Params fp;
			fp.customProjection = &VP;
			fp.customRotation = &ROT;
			fp.scaling = 0.02f;
			fp.h_align = Protean::Font::WIFALIGN_CENTER;
			fp.v_align = Protean::Font::WIFALIGN_BOTTOM;
			fp.enableDepthTest();

			for (size_t i = 0; i < scene.metadatas.GetCount(); ++i)
			{
				Entity entity = scene.metadatas.GetEntity(i);
				if (!scene.transforms.Contains(entity))
					continue;
				const TransformComponent& transform = *scene.transforms.GetComponent(entity);
				const MetadataComponent& metadata = scene.metadatas[i];

				fp.position = transform.GetPosition();

				switch (metadata.preset)
				{
				default:
					break;
				case MetadataComponent::Preset_Waypoint:
					fp.position.y += 1;
					fp.color = Protean::Color::fromFloat4(XMFLOAT4(1, 0.2f, 0.5f, 1));
					Protean::Font::Draw("Waypoint", fp, cmd);
					dummy::draw_waypoint(XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					break;
				case MetadataComponent::Preset_Enemy:
					fp.position.y += 2;
					fp.color = Protean::Color::fromFloat4(XMFLOAT4(0.8f, 0.2f, 0.2f, 1));
					Protean::Font::Draw("Enemy", fp, cmd);
					dummy::draw_direction(XMMatrixRotationY(XM_PI) * XMMatrixTranslation(0, 0.1f, 0) * XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					dummy::draw_soldier(XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					break;
				case MetadataComponent::Preset_Player:
					fp.position.y += 2;
					fp.color = Protean::Color::fromFloat4(XMFLOAT4(0.2f, 0.8f, 0.6f, 1));
					Protean::Font::Draw("Player", fp, cmd);
					dummy::draw_direction(XMMatrixRotationY(XM_PI) * XMMatrixTranslation(0, 0.1f, 0) * XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					dummy::draw_male(XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					break;
				case MetadataComponent::Preset_NPC:
					fp.position.y += 1.8f;
					fp.color = Protean::Color::fromFloat4(XMFLOAT4(0.2f, 0.6f, 0.8f, 1));
					Protean::Font::Draw("NPC", fp, cmd);
					dummy::draw_direction(XMMatrixRotationY(XM_PI) * XMMatrixTranslation(0, 0.1f, 0) * XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					dummy::draw_female(XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					break;
				case MetadataComponent::Preset_Pickup:
					fp.position.y += 1;
					fp.color = Protean::Color::fromFloat4(XMFLOAT4(1, 0.8f, 0.4f, 1));
					Protean::Font::Draw("Pickup", fp, cmd);
					dummy::draw_pickup(XMMatrixScaling(sca, sca, sca) * XMMatrixTranslation(0, 0.5f, 0) * XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					break;
				case MetadataComponent::Preset_Vehicle:
					fp.position.y += 1.6f;
					fp.color = Protean::Color(255, 133, 76, 255);
					Protean::Font::Draw("Vehicle", fp, cmd);
					dummy::draw_vehicle(XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					break;
				case MetadataComponent::Preset_PointOfInterest:
					fp.position.y += 1.0f;
					fp.color = Protean::Color(176, 91, 255, 255);
					Protean::Font::Draw("Point of interest", fp, cmd);
					dummy::draw_poi(XMMatrixScaling(sca, sca, sca) * XMLoadFloat4x4(&transform.world), VP, fp.color, true, cmd, camera.frustum);
					break;
				}
			}

			device->RenderPassEnd(cmd);
			device->EventEnd(cmd);
		}

		// Full resolution:
		{
			const Texture& render_result_2D = renderPath->GetRenderResult2D();
			if (getMSAASampleCount() > 1)
			{
				RenderPassImage rp[] = {
					RenderPassImage::RenderTarget(
						&editor_rendertarget,
						RenderPassImage::LoadOp_CLEAR,
						RenderPassImage::StoreOp_DONTCARE,
						ResourceState_RENDERTARGET,
						ResourceState_RENDERTARGET
					),
					RenderPassImage::Resolve(&render_result_2D),
					RenderPassImage::DepthStencil(
						&editor_depthbuffer,
						RenderPassImage::LoadOp_CLEAR,
						RenderPassImage::StoreOp_DONTCARE
					),
				};
				device->RenderPassBegin(rp, arraysize(rp), cmd);
			}
			else
			{
				RenderPassImage rp[] = {
					RenderPassImage::RenderTarget(
						&render_result_2D,
						RenderPassImage::LoadOp_CLEAR
					),
					RenderPassImage::DepthStencil(
						&editor_depthbuffer,
						RenderPassImage::LoadOp_CLEAR,
						RenderPassImage::StoreOp_DONTCARE
					),
				};
				device->RenderPassBegin(rp, arraysize(rp), cmd);
			}

			Viewport vp;
			vp.width = (float)editor_depthbuffer.GetDesc().width;
			vp.height = (float)editor_depthbuffer.GetDesc().height;
			device->BindViewports(1, &vp, cmd);

			// Selection color:
			float selectionColorIntensity = std::sin(outlineTimer * XM_2PI * 0.8f) * 0.5f + 0.5f;
			XMFLOAT4 glow = Protean::Math::Lerp(Protean::Math::Lerp(XMFLOAT4(1, 1, 1, 1), selectionColor, 0.4f), selectionColor, selectionColorIntensity);
			Protean::Color selectedEntityColor = Protean::Color::fromFloat4(glow);

			// Draw selection outline to the screen:
			if (renderPath->GetDepthStencil() != nullptr && !translator.selected.empty())
			{
				device->EventBegin("Selection Outline Edge", cmd);
				Protean::Renderer::BindCommonResources(cmd);
				float opacity = Protean::Math::Lerp(0.4f, 1.0f, selectionColorIntensity) * generalWnd.outlineOpacitySlider.GetValue();
				XMFLOAT4 col = selectionColor2;
				col.w *= opacity;
				Protean::Renderer::Postprocess_Outline(rt_selectionOutline[0], cmd, 0.1f, 1, col);
				col = selectionColor;
				col.w *= opacity;
				Protean::Renderer::Postprocess_Outline(rt_selectionOutline[1], cmd, 0.1f, 1, col);
				device->EventEnd(cmd);
			}

			if (dummy_enabled)
			{
				Protean::Image::Params fx;
				fx.enableFullScreen();
				fx.blendFlag = Protean::BLENDMODE_PREMULTIPLIED;
				fx.color = XMFLOAT4(0, 0, 0, 0.4f);
				Protean::Image::Draw(&rt_dummyOutline, fx, cmd);
				XMFLOAT4 dummyColorBlinking = dummyColor;
				dummyColorBlinking.w = Protean::Math::Lerp(0.4f, 1, selectionColorIntensity);
				Protean::Renderer::Postprocess_Outline(rt_dummyOutline, cmd, 0.1f, 1, dummyColorBlinking);
			}

			MiscCB sb;
			XMStoreFloat4x4(&sb.g_xTransform, VP);
			sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
			device->BindDynamicConstantBuffer(sb, CB_GETBINDSLOT(MiscCB), cmd);

			const XMMATRIX R = XMLoadFloat3x3(&cam.rotationMatrix);

			const float ortho_dist = camera.ortho_vertical_size * 0.5f;
			auto camera_scaling = [&](const XMFLOAT3& position) {
				return camera.IsOrtho() ? ortho_dist : Protean::Math::Distance(position, camera.Eye);;
			};

			Protean::Font::Params fp;
			fp.customRotation = &R;
			fp.customProjection = &VP;
			fp.size = 32; // icon font render quality
			const float scaling = 0.0025f;
			fp.h_align = Protean::Font::WIFALIGN_CENTER;
			fp.v_align = Protean::Font::WIFALIGN_CENTER;
			fp.shadowColor = backgroundEntityColor;
			fp.shadow_softness = 1;

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Light))
			{
				for (size_t i = 0; i < scene.lights.GetCount(); ++i)
				{
					const LightComponent& light = scene.lights[i];
					Entity entity = scene.lights.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;

					const float dist = camera_scaling(light.position);

					fp.position = light.position;
					fp.scaling = scaling * dist;
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}

					switch (light.GetType())
					{
					case LightComponent::POINT:
						Protean::Font::Draw(ICON_POINTLIGHT, fp, cmd);
						break;
					case LightComponent::SPOT:
						Protean::Font::Draw(ICON_SPOTLIGHT, fp, cmd);
						break;
					case LightComponent::DIRECTIONAL:
						Protean::Font::Draw(ICON_DIRECTIONALLIGHT, fp, cmd);
						break;
					case LightComponent::RECTANGLE:
						Protean::Font::Draw(ICON_RECTLIGHT, fp, cmd);
						break;
					default:
						break;
					}

					if (light.GetType() == LightComponent::DIRECTIONAL || light.GetType() == LightComponent::SPOT)
					{
						// Light direction visualizer:
						device->EventBegin("LightDirectionVisualizer", cmd);
						static PipelineState pso;
						if (!pso.IsValid())
						{
							static auto LoadShaders = [] {
								PipelineStateDesc desc;
								desc.vs = Protean::Renderer::GetShader(Protean::VSTYPE_VERTEXCOLOR);
								desc.ps = Protean::Renderer::GetShader(Protean::PSTYPE_VERTEXCOLOR);
								desc.il = Protean::Renderer::GetInputLayout(Protean::ILTYPE_VERTEXCOLOR);
								desc.dss = Protean::Renderer::GetDepthStencilState(Protean::DSSTYPE_DEFAULT);
								desc.rs = Protean::Renderer::GetRasterizerState(Protean::RSTYPE_DOUBLESIDED);
								desc.bs = Protean::Renderer::GetBlendState(Protean::BSTYPE_TRANSPARENT);
								desc.pt = PrimitiveTopology_TRIANGLELIST;
								Protean::Graphics::GetDevice()->CreatePipelineState(&desc, &pso);
							};
							static Protean::EventHandler::Handle handle = Protean::EventHandler::Subscribe(Protean::EventHandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
							LoadShaders();
						}
						struct Vertex
						{
							XMFLOAT4 position;
							XMFLOAT4 color;
						};

						device->BindPipelineState(&pso, cmd);

						const uint32_t segmentCount = 6;
						const uint32_t cylinder_triangleCount = segmentCount * 2;
						const uint32_t cone_triangleCount = segmentCount;
						const uint32_t vertexCount = (cylinder_triangleCount + cone_triangleCount) * 3;
						auto mem = device->AllocateGPU(sizeof(Vertex) * vertexCount, cmd);

						float siz = 0.02f * dist;
						const XMMATRIX M =
							XMMatrixScaling(siz, siz, siz) *
							XMMatrixRotationZ(-XM_PIDIV2) *
							XMMatrixRotationQuaternion(XMLoadFloat4(&light.rotation)) *
							XMMatrixTranslation(light.position.x, light.position.y, light.position.z)
							;

						const XMFLOAT4 col = fp.color;
						const XMFLOAT4 col_fade = XMFLOAT4(col.x, col.y, col.z, 0);
						const float origin_size = 0.2f;
						const float cone_length = 0.75f;
						const float axis_length = 18;
						float cylinder_length = axis_length;
						cylinder_length -= cone_length;
						uint8_t* dst = (uint8_t*)mem.data;
						for (uint32_t i = 0; i < segmentCount; ++i)
						{
							const float angle0 = (float)i / (float)segmentCount * XM_2PI;
							const float angle1 = (float)(i + 1) / (float)segmentCount * XM_2PI;
							// cylinder base:
							{
								const float cylinder_radius = 0.075f;
								Vertex verts[] = {
									{XMFLOAT4(0, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), col_fade},
									{XMFLOAT4(0, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), col_fade},
									{XMFLOAT4(cylinder_length, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), col},
									{XMFLOAT4(cylinder_length, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), col},
									{XMFLOAT4(cylinder_length, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), col},
									{XMFLOAT4(0, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), col_fade},
								};
								for (auto& vert : verts)
								{
									XMStoreFloat4(&vert.position, XMVector3Transform(XMLoadFloat4(&vert.position), M));
								}
								std::memcpy(dst, verts, sizeof(verts));
								dst += sizeof(verts);
							}
							// cone cap:
							{
								const float cone_radius = origin_size;
								Vertex verts[] = {
									{XMFLOAT4(axis_length, 0, 0, 1), col},
									{XMFLOAT4(cylinder_length, std::sin(angle0) * cone_radius, std::cos(angle0) * cone_radius, 1), col},
									{XMFLOAT4(cylinder_length, std::sin(angle1) * cone_radius, std::cos(angle1) * cone_radius, 1), col},
								};
								for (auto& vert : verts)
								{
									XMStoreFloat4(&vert.position, XMVector3Transform(XMLoadFloat4(&vert.position), M));
								}
								std::memcpy(dst, verts, sizeof(verts));
								dst += sizeof(verts);
							}
						}
						const GPUBuffer* vbs[] = {
							&mem.buffer,
						};
						const uint32_t strides[] = {
							sizeof(Vertex),
						};
						const uint64_t offsets[] = {
							mem.offset,
						};
						device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
						device->Draw(vertexCount, 0, cmd);
						device->EventEnd(cmd);
					}
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Constraint))
			{
				for (size_t i = 0; i < scene.constraints.GetCount(); ++i)
				{
					Entity entity = scene.constraints.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_CONSTRAINT, fp, cmd);

				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Spline))
			{
				for (size_t i = 0; i < scene.splines.GetCount(); ++i)
				{
					for (size_t j = 0; j < scene.splines[i].spline_node_transforms.size(); ++j)
					{
						Entity entity = scene.splines[i].spline_node_entities[j];
						const TransformComponent& transform = scene.splines[i].spline_node_transforms[j];

						fp.position = transform.GetPosition();
						fp.scaling = 0.5f * scaling * camera_scaling(transform.GetPosition());
						fp.color = inactiveEntityColor;

						if (hovered.entity == entity)
						{
							fp.color = hoveredEntityColor;
						}
						for (auto& picked : translator.selected)
						{
							if (picked.entity == entity)
							{
								fp.color = selectedEntityColor;
								break;
							}
						}
						Protean::Font::Draw(ICON_SPLINE_NODE, fp, cmd);
					}

					Entity entity = scene.splines.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}
					Protean::Font::Draw(ICON_SPLINE, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Decal))
			{
				for (size_t i = 0; i < scene.decals.GetCount(); ++i)
				{
					Entity entity = scene.decals.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_DECAL, fp, cmd);

				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Force))
			{
				for (size_t i = 0; i < scene.forces.GetCount(); ++i)
				{
					Entity entity = scene.forces.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_FORCE, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Camera))
			{
				for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
				{
					Entity entity = scene.cameras.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_CAMERA, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Armature))
			{
				for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
				{
					Entity entity = scene.armatures.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_ARMATURE, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Emitter))
			{
				for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
				{
					Entity entity = scene.emitters.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_EMITTER, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Hairparticle))
			{
				for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
				{
					Entity entity = scene.hairs.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_HAIR, fp, cmd);
				}
			}

			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Sound))
			{
				for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
				{
					Entity entity = scene.sounds.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_SOUND, fp, cmd);
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Video))
			{
				for (size_t i = 0; i < scene.videos.GetCount(); ++i)
				{
					Entity entity = scene.videos.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}


					Protean::Font::Draw(ICON_VIDEO, fp, cmd);
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_VoxelGrid))
			{
				for (size_t i = 0; i < scene.voxel_grids.GetCount(); ++i)
				{
					Entity entity = scene.voxel_grids.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}

					Protean::Font::Draw(ICON_VOXELGRID, fp, cmd);
				}
			}
			if (has_flag(componentsWnd.filter, ComponentsWindow::Filter_Metadata))
			{
				if (scene.metadatas.GetCount() > 0)
				{
					Protean::Image::Params fx;
					fx.enableFullScreen();
					fx.blendFlag = Protean::BLENDMODE_PREMULTIPLIED;
					fx.color.x = Protean::Math::Lerp(fx.color.x * 0.4f, fx.color.x, selectionColorIntensity);
					fx.color.y = Protean::Math::Lerp(fx.color.y * 0.4f, fx.color.y, selectionColorIntensity);
					fx.color.z = Protean::Math::Lerp(fx.color.z * 0.4f, fx.color.z, selectionColorIntensity);
					fx.color.w = 0.6f;
					//fx.color.w = Protean::Math::Lerp(fx.color.w * 0.8f, fx.color.w, selectionColorIntensity);
					Protean::Image::Draw(&rt_metadataDummies, fx, cmd);
				}

				for (size_t i = 0; i < scene.metadatas.GetCount(); ++i)
				{
					Entity entity = scene.metadatas.GetEntity(i);
					if (!scene.transforms.Contains(entity))
						continue;
					const TransformComponent& transform = *scene.transforms.GetComponent(entity);

					fp.position = transform.GetPosition();
					fp.scaling = scaling * camera_scaling(transform.GetPosition());
					fp.color = inactiveEntityColor;

					if (hovered.entity == entity)
					{
						fp.color = hoveredEntityColor;
					}
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							fp.color = selectedEntityColor;
							break;
						}
					}

					Protean::Font::Draw(ICON_METADATA, fp, cmd);
				}
			}
			if (bone_picking)
			{
				static PipelineState pso;
				if (!pso.IsValid())
				{
					static auto LoadShaders = [] {
						PipelineStateDesc desc;
						desc.vs = Protean::Renderer::GetShader(Protean::VSTYPE_VERTEXCOLOR);
						desc.ps = Protean::Renderer::GetShader(Protean::PSTYPE_VERTEXCOLOR);
						desc.il = Protean::Renderer::GetInputLayout(Protean::ILTYPE_VERTEXCOLOR);
						desc.dss = Protean::Renderer::GetDepthStencilState(Protean::DSSTYPE_DEPTHDISABLED);
						desc.rs = Protean::Renderer::GetRasterizerState(Protean::RSTYPE_DOUBLESIDED);
						desc.bs = Protean::Renderer::GetBlendState(Protean::BSTYPE_TRANSPARENT);
						desc.pt = PrimitiveTopology_TRIANGLELIST;
						Protean::Graphics::GetDevice()->CreatePipelineState(&desc, &pso);
					};
					static Protean::EventHandler::Handle handle = Protean::EventHandler::Subscribe(Protean::EventHandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
					LoadShaders();
				}

				size_t bone_count = bone_picking_items.size();
				if (bone_count > 0)
				{
					struct Vertex
					{
						XMFLOAT4 position;
						XMFLOAT4 color;
					};
					const size_t segment_count = 18 + 1 + 18 + 1;
					const size_t vb_size = sizeof(Vertex) * (bone_count * (segment_count + 1 + 1));
					const size_t ib_size = sizeof(uint32_t) * bone_count * (segment_count + 1) * 3;
					GraphicsDevice::GPUAllocation mem = device->AllocateGPU(vb_size + ib_size, cmd);
					Vertex* vertices = (Vertex*)mem.data;
					uint32_t* indices = (uint32_t*)((uint8_t*)mem.data + vb_size);
					uint32_t vertex_count = 0;
					uint32_t index_count = 0;

					const XMVECTOR Eye = camera.GetEye();
					const XMVECTOR Unit = XMVectorSet(0, 1, 0, 0);

					for (auto& it : bone_picking_items)
					{
						Entity entity = it.first;
						const Capsule& capsule = it.second;
						XMVECTOR a = XMLoadFloat3(&capsule.base);
						XMVECTOR b = XMLoadFloat3(&capsule.tip);
						XMVECTOR ab = XMVector3Normalize(b - a);

						XMFLOAT4 color = inactiveEntityColor;

						if (scene.springs.Contains(entity))
						{
							color = springDebugColor;
						}
						else if (scene.inverse_kinematics.Contains(entity))
						{
							color = ikDebugColor;
						}

						if (hovered.entity == entity)
						{
							color = hoveredEntityColor;
						}
						for (auto& picked : translator.selected)
						{
							if (picked.entity == entity)
							{
								color = selectedEntityColor;
								break;
							}
						}

						color.w *= generalWnd.bonePickerOpacitySlider.GetValue();

						XMVECTOR Base = XMLoadFloat3(&capsule.base);
						XMVECTOR Tip = XMLoadFloat3(&capsule.tip);
						XMVECTOR Radius = XMVectorReplicate(capsule.radius);
						XMVECTOR Normal = XMVector3Normalize(Tip - Base);
						XMVECTOR Tangent = XMVector3Normalize(XMVector3Cross(Normal, Base - Eye));
						XMVECTOR Binormal = XMVector3Normalize(XMVector3Cross(Tangent, Normal));
						XMVECTOR LineEndOffset = Normal * Radius;
						XMVECTOR A = Base + LineEndOffset;
						XMVECTOR B = Tip - LineEndOffset;
						XMVECTOR AB = Unit * XMVector3Length(B - A);
						XMMATRIX M = { Tangent,Normal,Binormal,XMVectorSetW(A, 1) };

						uint32_t center_vertex_index = vertex_count;
						Vertex center_vertex;
						XMStoreFloat4(&center_vertex.position, A);
						center_vertex.position.w = 1;
						center_vertex.color = color;
						center_vertex.color.w = 0;
						std::memcpy(vertices + vertex_count, &center_vertex, sizeof(center_vertex));
						vertex_count++;

						for (size_t i = 0; i < segment_count; ++i)
						{
							XMVECTOR segment_pos;
							const float angle0 = XM_PIDIV2 + (float)i / (float)segment_count * XM_2PI;
							if (i < 18)
							{
								segment_pos = XMVectorSet(sinf(angle0) * capsule.radius, cosf(angle0) * capsule.radius, 0, 1);
							}
							else if (i == 18)
							{
								segment_pos = XMVectorSet(sinf(angle0) * capsule.radius, cosf(angle0) * capsule.radius, 0, 1);
							}
							else if (i > 18 && i < 18 + 1 + 18)
							{
								segment_pos = AB + XMVectorSet(sinf(angle0) * capsule.radius * 0.5f, cosf(angle0) * capsule.radius * 0.5f, 0, 1);
							}
							else
							{
								segment_pos = AB + XMVectorSet(sinf(angle0) * capsule.radius * 0.5f, cosf(angle0) * capsule.radius * 0.5f, 0, 1);
							}
							segment_pos = XMVector3Transform(segment_pos, M);

							Vertex vertex;
							XMStoreFloat4(&vertex.position, segment_pos);
							vertex.position.w = 1;
							vertex.color = color;
							//vertex.color.w = 0;
							std::memcpy(vertices + vertex_count, &vertex, sizeof(vertex));
							uint32_t ind[] = { center_vertex_index,vertex_count - 1,vertex_count };
							std::memcpy(indices + index_count, ind, sizeof(ind));
							index_count += arraysize(ind);
							vertex_count++;
						}
						// closing triangle fan:
						uint32_t ind[] = { center_vertex_index,vertex_count - 1,center_vertex_index + 1 };
						std::memcpy(indices + index_count, ind, sizeof(ind));
						index_count += arraysize(ind);
					}

					device->EventBegin("Bone capsules", cmd);
					device->BindPipelineState(&pso, cmd);

					const GPUBuffer* vbs[] = {
						&mem.buffer,
					};
					const uint32_t strides[] = {
						sizeof(Vertex)
					};
					const uint64_t offsets[] = {
						mem.offset,
					};
					device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);
					device->BindIndexBuffer(&mem.buffer, IndexBufferFormat_UINT32, mem.offset + vb_size, cmd);

					device->DrawIndexed(index_count, 0, 0, cmd);
					device->EventEnd(cmd);
				}
			}

			if (generalWnd.nameDebugCheckBox.GetCheck())
			{
				device->EventBegin("Debug Names", cmd);
				struct DebugNameEntitySorter
				{
					size_t name_index;
					float distance;
					XMFLOAT3 position;
				};
				static Protean::vector<DebugNameEntitySorter> debugNameEntitiesSorted;
				debugNameEntitiesSorted.clear();
				for (size_t i = 0; i < scene.names.GetCount(); ++i)
				{
					Entity entity = scene.names.GetEntity(i);
					const TransformComponent* transform = scene.transforms.GetComponent(entity);
					if (transform != nullptr)
					{
						auto& x = debugNameEntitiesSorted.emplace_back();
						x.name_index = i;
						x.position = transform->GetPosition();
						const ObjectComponent* object = scene.objects.GetComponent(entity);
						if (object != nullptr)
						{
							x.position = object->center;
						}
						x.distance = Protean::Math::Distance(x.position, camera.Eye);
					}
				}
				std::sort(debugNameEntitiesSorted.begin(), debugNameEntitiesSorted.end(), [](const DebugNameEntitySorter& a, const DebugNameEntitySorter& b)
					{
						return a.distance > b.distance;
					});
				for (auto& x : debugNameEntitiesSorted)
				{
					Entity entity = scene.names.GetEntity(x.name_index);
					Protean::Font::Params params;
					params.position = x.position;
					params.size = Protean::Font::WIFONTSIZE_DEFAULT;
					params.scaling = 1.0f / params.size * x.distance * 0.03f;
					params.color = Protean::Color::White();
					for (auto& picked : translator.selected)
					{
						if (picked.entity == entity)
						{
							params.color = selectedEntityColor;
							break;
						}
					}
					params.h_align = Protean::Font::WIFALIGN_CENTER;
					params.v_align = Protean::Font::WIFALIGN_CENTER;
					params.softness = 0.1f;
					params.shadowColor = Protean::Color::Black();
					params.shadow_softness = 0.5f;
					params.customProjection = &VP;
					params.customRotation = &R;
					Protean::Font::Draw(scene.names[x.name_index].name, params, cmd);
				}
				device->EventEnd(cmd);
			}

			if (inspector_mode)
			{
				std::string str;
				str += "Entity: " + std::to_string(hovered.entity);
				const NameComponent* name = scene.names.GetComponent(hovered.entity);
				if (name != nullptr)
				{
					str += "\nName: " + name->name;
				}
				XMFLOAT4 pointer = Protean::Input::GetPointer();
				Protean::Font::Params params;
				params.position = XMFLOAT3(pointer.x - 10, pointer.y, 0);
				params.shadowColor = Protean::Color::Black();
				params.h_align = Protean::Font::WIFALIGN_RIGHT;
				params.v_align = Protean::Font::WIFALIGN_CENTER;
				Protean::Font::Draw(str, params, cmd);
			}

			if (generalWnd.splineVisCheckBox.GetCheck())
			{
				spline_renderer.Draw(camera, cmd);
			}

			paintToolWnd.DrawBrush(*this, cmd);
			if (paintToolWnd.GetMode() == PaintToolWindow::MODE::MODE_DISABLED)
			{
				translator.Draw(GetCurrentEditorScene().camera, currentMouse, cmd);
			}

#if 0
			// Debug object AABB screen projection for LOD:
			for (size_t i = 0; i < scene.objects.GetCount(); ++i)
			{
				const AABB& aabb = scene.aabb_objects[i];
				XMFLOAT4 rect = aabb.ProjectToScreen(VP);

				Protean::Image::Params fx;
				fx.color = Protean::Color::Yellow();
				fx.color.w = 0.4f;
				fx.pos.x = PhysicalToLogical(rect.x * vp.width);
				fx.pos.y = PhysicalToLogical(rect.y * vp.height);
				fx.siz.x = PhysicalToLogical(rect.z * vp.width - rect.x * vp.width);
				fx.siz.y = PhysicalToLogical(rect.w * vp.height - rect.y * vp.height);
				Protean::Image::Draw(nullptr, fx, cmd);
			}
#endif

			if (drive_mode)
			{
				Protean::Font::Params params;
				params.color = save_text_color;
				params.shadowColor = Protean::Color::Black();
				params.shadowColor.setA(params.color.getA());
				params.position = XMFLOAT3(PhysicalToLogical(viewport3D.width * 0.5f), 20, 0);
				params.h_align = Protean::Font::WIFALIGN_CENTER;
				params.v_align = Protean::Font::WIFALIGN_TOP;
				params.size = 30;
				params.shadow_softness = 1;
				Protean::Font::Draw("Drive Mode", params, cmd);
			}

			device->RenderPassEnd(cmd);
		}

		if(!generalWnd.focusModeCheckBox.GetCheck() && themeEditorWnd.waveColor.getA() > 0)
		{
			device->EventBegin("Background wave effect", cmd);
			device->RenderPassBegin(&gui_background_effect, cmd);

			Viewport vp;
			vp.width = (float)gui_background_effect.desc.width;
			vp.height = (float)gui_background_effect.desc.height;
			device->BindViewports(1, &vp, cmd);

			Protean::Graphics::Rect rect;
			rect.from_viewport(vp);
			device->BindScissorRects(1, &rect, cmd);

			Protean::Renderer::DrawWaveEffect(themeEditorWnd.waveColor, cmd);

			device->RenderPassEnd(cmd);
			device->EventEnd(cmd);
		}

		device->EventEnd(cmd);
	}

	RenderPath2D::Render();

	Protean::Renderer::SetDebugDrawEnabled(true);
}
void EditorComponent::Compose(CommandList cmd) const
{
	GetDevice()->BindViewports(1, &viewport3D, cmd);
	renderPath->Compose(cmd);

	Viewport vp;
	vp.top_left_x = 0;
	vp.top_left_y = 0;
	vp.width = (float)GetPhysicalWidth();
	vp.height = (float)GetPhysicalHeight();
	GetDevice()->BindViewports(1, &vp, cmd);

	RenderPath2D::Compose(cmd);

	if (save_text_alpha > 0)
	{
		Protean::Font::Params params;
		params.color = save_text_color;
		params.shadowColor = Protean::Color::Black();
		params.color.setA(uint8_t(std::min(1.0f, save_text_alpha) * 255));
		params.shadowColor.setA(params.color.getA());
		params.position = XMFLOAT3(GetLogicalWidth() * 0.5f, GetLogicalHeight() * 0.5f, 0);
		params.h_align = Protean::Font::WIFALIGN_CENTER;
		params.v_align = Protean::Font::WIFALIGN_CENTER;
		params.size = 30;
		params.shadow_softness = 1;
		if (colorspace != ColorSpace_SRGB)
		{
			params.enableLinearOutputMapping(9);
		}
		Protean::Font::Cursor cursor = Protean::Font::Draw(save_text_message, params, cmd);

		params.size = 24;
		params.position.y += cursor.size.y;
		Protean::Font::Draw(save_text_filename, params, cmd);
	}
}

void EditorComponent::ResizeViewport3D()
{
	int width = GetPhysicalWidth();
	int height = GetPhysicalHeight();
	if (GetGUI().IsVisible())
	{
		if (componentsWnd.IsVisible())
		{
			width -= LogicalToPhysical(componentsWnd.scale_local.x);
		}
		if (generalWnd.IsVisible())
		{
			width -= LogicalToPhysical(generalWnd.scale_local.x);
		}
		if (graphicsWnd.IsVisible())
		{
			width -= LogicalToPhysical(graphicsWnd.scale_local.x);
		}
		if (cameraWnd.IsVisible())
		{
			width -= LogicalToPhysical(cameraWnd.scale_local.x);
		}
		if (materialPickerWnd.IsVisible())
		{
			width -= LogicalToPhysical(materialPickerWnd.scale_local.x);
		}
		if (paintToolWnd.IsVisible())
		{
			width -= LogicalToPhysical(paintToolWnd.scale_local.x);
		}
		height -= LogicalToPhysical(topmenuWnd.scale_local.y);
	}
	width = std::max(64, width);
	height = std::max(64, height);
	if (renderPath->width != width || renderPath->height != height || rt_selectionOutline_MSAA.desc.sample_count != renderPath->getMSAASampleCount())
	{
		renderPath->width = width;
		renderPath->height = height;
		renderPath->dpi = dpi;
		renderPath->scaling = scaling;
		renderPath->ResizeBuffers();

		viewport3D.top_left_x = 0;
		viewport3D.top_left_y = 0;
		if (GetGUI().IsVisible())
		{
			if (generalWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(generalWnd.scale_local.x);
			}
			if (graphicsWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(graphicsWnd.scale_local.x);
			}
			if (cameraWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(cameraWnd.scale_local.x);
			}
			if (materialPickerWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(materialPickerWnd.scale_local.x);
			}
			if (paintToolWnd.IsVisible())
			{
				viewport3D.top_left_x = (float)LogicalToPhysical(paintToolWnd.scale_local.x);
			}
			viewport3D.top_left_y = (float)LogicalToPhysical(topmenuWnd.scale_local.y);
		}
		viewport3D.width = (float)renderPath->width;
		viewport3D.height = (float)renderPath->height;

		GraphicsDevice* device = Protean::Graphics::GetDevice();

		XMUINT2 internalResolution = renderPath->GetInternalResolution();

		if (renderPath->GetDepthStencil() != nullptr)
		{
			bool success = false;

			TextureDesc desc;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;

			desc.format = Format_R8_UNORM;
			desc.bind_flags = BindFlag_RENDER_TARGET | BindFlag_SHADER_RESOURCE;
			if (renderPath->getMSAASampleCount() > 1)
			{
				desc.sample_count = renderPath->getMSAASampleCount();
				success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline_MSAA);
				assert(success);
				device->SetName(&rt_selectionOutline_MSAA, "editor.rt_selectionOutline_MSAA");
				desc.sample_count = 1;
			}
			else
			{
				rt_selectionOutline_MSAA = {};
			}
			success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[0]);
			assert(success);
			device->SetName(&rt_selectionOutline[0], "editor.rt_selectionOutline[0]");
			success = device->CreateTexture(&desc, nullptr, &rt_selectionOutline[1]);
			assert(success);
			device->SetName(&rt_selectionOutline[1], "editor.rt_selectionOutline[1]");
		}

		const TextureDesc& renderResultDesc = renderPath->GetRenderResult2D().GetDesc();
		{
			TextureDesc desc;
			desc.width = renderResultDesc.width;
			desc.height = renderResultDesc.height;
			desc.format = Format_R8_UNORM;
			desc.bind_flags = BindFlag_RENDER_TARGET | BindFlag_SHADER_RESOURCE;
			desc.swizzle.r = ComponentSwizzle_R;
			desc.swizzle.g = ComponentSwizzle_R;
			desc.swizzle.b = ComponentSwizzle_R;
			desc.swizzle.a = ComponentSwizzle_R;
			device->CreateTexture(&desc, nullptr, &rt_dummyOutline);
			device->SetName(&rt_dummyOutline, "editor.rt_dummyOutline");
		}

		{
			TextureDesc desc;
			desc.width = renderResultDesc.width;
			desc.height = renderResultDesc.height;
			desc.misc_flags = ResourceMiscFlag_TRANSIENT_ATTACHMENT;
			desc.sample_count = getMSAASampleCount();

			desc.format = Format_D32_FLOAT;
			desc.bind_flags = BindFlag_DEPTH_STENCIL;
			desc.layout = ResourceState_DEPTHSTENCIL;
			device->CreateTexture(&desc, nullptr, &editor_depthbuffer);
			device->SetName(&editor_depthbuffer, "editor.editor_depthbuffer");

			if (getMSAASampleCount() > 1)
			{
				desc.format = renderResultDesc.format;
				desc.bind_flags = BindFlag_RENDER_TARGET;
				desc.layout = ResourceState_RENDERTARGET;
				device->CreateTexture(&desc, nullptr, &editor_rendertarget);
				device->SetName(&editor_rendertarget, "editor.editor_rendertarget");
			}
		}

		{
			TextureDesc desc;
			desc.width = internalResolution.x;
			desc.height = internalResolution.y;
			desc.format = renderResultDesc.format;
			desc.bind_flags = BindFlag_RENDER_TARGET | BindFlag_SHADER_RESOURCE;
			device->CreateTexture(&desc, nullptr, &rt_metadataDummies);
			device->SetName(&rt_metadataDummies, "editor.rt_metadataDummies");

			if (renderPath->getMSAASampleCount() > 1)
			{
				desc.sample_count = renderPath->getMSAASampleCount();
				desc.bind_flags = BindFlag_RENDER_TARGET;
				desc.misc_flags = ResourceMiscFlag_TRANSIENT_ATTACHMENT;
				desc.layout = ResourceState_RENDERTARGET;
				device->CreateTexture(&desc, nullptr, &rt_metadataDummies_MSAA);
				device->SetName(&rt_metadataDummies_MSAA, "editor.rt_metadataDummies_MSAA");
			}
		}
	}

	if (GetGUI().IsVisible())
	{
		main->infoDisplay.rect.from_viewport(viewport3D);
		main->infoDisplay.rect.left = LogicalToPhysical(generalButton.translation_local.x + generalButton.scale_local.x + 10);
	}

	loadmodel_font.params.posX = PhysicalToLogical(uint32_t(viewport3D.top_left_x + viewport3D.width * 0.5f));
	loadmodel_font.params.posX -= Protean::Font::TextWidth(loadmodel_font.GetText(), loadmodel_font.params) * 0.5f;
	loadmodel_font.params.posY = PhysicalToLogical(uint32_t(viewport3D.top_left_y)) + 15;

	terrain_generation_font.params.posX = PhysicalToLogical(uint32_t(viewport3D.top_left_x + viewport3D.width * 0.5f));
	terrain_generation_font.params.posX -= Protean::Font::TextWidth(terrain_generation_font.GetText(), terrain_generation_font.params) * 0.5f;
	terrain_generation_font.params.posY = PhysicalToLogical(uint32_t(viewport3D.top_left_y)) + 45;

	viewport3D_hitbox = Hitbox2D(
		XMFLOAT2(PhysicalToLogical(uint32_t(viewport3D.top_left_x)), PhysicalToLogical(uint32_t(viewport3D.top_left_y))),
		XMFLOAT2(PhysicalToLogical(uint32_t(viewport3D.width)), PhysicalToLogical(uint32_t(viewport3D.height)))
	);
}

void EditorComponent::ClearSelected()
{
	translator.selected.clear();
}
void EditorComponent::AddSelected(Entity entity, bool allow_refocus)
{
	Protean::PickResult res;
	res.entity = entity;
	AddSelected(res, allow_refocus);
}
void EditorComponent::AddSelected(const PickResult& picked, bool allow_refocus)
{
	Protean::Scene& scene = GetCurrentScene();

	bool removal = false;
	for (size_t i = 0; i < translator.selected.size(); ++i)
	{
		if(translator.selected[i] == picked)
		{
			// If already selected, it will be deselected now:
			translator.selected[i] = translator.selected.back();
			translator.selected.pop_back();
			removal = true;
			break;
		}
	}

	if (!removal)
	{
		translator.selected.push_back(picked);
		if (allow_refocus)
		{
			componentsWnd.entitytree_pending_focus = picked.entity;
		}
	}
}
bool EditorComponent::IsSelected(Entity entity) const
{
	for (auto& x : translator.selected)
	{
		if (x.entity == entity)
		{
			return true;
		}
	}
	return false;
}

void EditorComponent::RecordSelection(Protean::Archive& archive) const
{
	archive << translator.selected.size();
	for (auto& x : translator.selected)
	{
		archive << x.entity;
		archive << x.position;
		archive << x.normal;
		archive << x.subsetIndex;
		archive << x.distance;
	}
}
void EditorComponent::RecordEntity(Protean::Archive& archive, Protean::Entity entity)
{
	const Protean::vector<Entity> entities = { entity };
	RecordEntity(archive, entities);
}
void EditorComponent::RecordEntity(Protean::Archive& archive, const Protean::vector<Protean::Entity>& entities)
{
	Scene& scene = GetCurrentScene();
	EntitySerializer seri;

	archive << entities;
	for (auto& x : entities)
	{
		scene.Entity_Serialize(archive, seri, x);
	}
}

void EditorComponent::ResetHistory()
{
	EditorScene& editorscene = GetCurrentEditorScene();
	editorscene.historyPos = -1;
	editorscene.history.clear();
	editorscene.has_unsaved_changes = false;
}
Protean::Archive& EditorComponent::AdvanceHistory(const bool scene_unchanged)
{
	EditorScene& editorscene = GetCurrentEditorScene();
	editorscene.historyPos++;

	while (static_cast<int>(editorscene.history.size()) > editorscene.historyPos)
	{
		editorscene.history.pop_back();
	}

	editorscene.history.emplace_back();
	editorscene.history.back().SetReadModeAndResetPos(false);

	if (!scene_unchanged)
	{
		editorscene.has_unsaved_changes = true;
		RefreshSceneList();
	}

	return editorscene.history.back();
}
void EditorComponent::ConsumeHistoryOperation(bool undo)
{
	EditorScene& editorscene = GetCurrentEditorScene();

	if ((undo && editorscene.historyPos >= 0) || (!undo && editorscene.historyPos < (int)editorscene.history.size() - 1))
	{
		if (!undo)
		{
			editorscene.historyPos++;
		}

		Scene& scene = GetCurrentScene();

		Protean::Archive& archive = editorscene.history[editorscene.historyPos];
		archive.SetReadModeAndResetPos(true);

		int temp;
		archive >> temp;
		HistoryOperationType type = (HistoryOperationType)temp;

		switch (type)
		{
		case HISTORYOP_TRANSLATOR:
			{
				archive >> translator.isTranslator;
				archive >> translator.isRotator;
				archive >> translator.isScalator;

				EntitySerializer seri;
				Protean::TransformComponent start;
				Protean::TransformComponent end;
				start.Serialize(archive, seri);
				end.Serialize(archive, seri);
				Protean::vector<XMFLOAT4X4> matrices_start;
				Protean::vector<XMFLOAT4X4> matrices_end;
				archive >> matrices_start;
				archive >> matrices_end;

				translator.PreTranslate();
				if (undo)
				{
					translator.transform = start;
					translator.matrices_current = matrices_start;
				}
				else
				{
					translator.transform = end;
					translator.matrices_current = matrices_end;
				}
				translator.transform.UpdateTransform();
				translator.PostTranslate();
			}
			break;
		case HISTORYOP_SELECTION:
		{
			// Read selections states from archive:

			Protean::vector<Protean::PickResult> selectedBEFORE;
			size_t selectionCountBEFORE;
			archive >> selectionCountBEFORE;
			for (size_t i = 0; i < selectionCountBEFORE; ++i)
			{
				Protean::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedBEFORE.push_back(sel);
			}

			Protean::vector<Protean::PickResult> selectedAFTER;
			size_t selectionCountAFTER;
			archive >> selectionCountAFTER;
			for (size_t i = 0; i < selectionCountAFTER; ++i)
			{
				Protean::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedAFTER.push_back(sel);
			}

			// Restore proper selection state:
			if (undo)
			{
				translator.selected = selectedBEFORE;
			}
			else
			{
				translator.selected = selectedAFTER;
			}
		}
		break;
		case HISTORYOP_ADD:
		case HISTORYOP_ADD_TO_SPLINE:
		{
			// Read selections states from archive:

			Protean::vector<Protean::PickResult> selectedBEFORE;
			size_t selectionCountBEFORE;
			archive >> selectionCountBEFORE;
			for (size_t i = 0; i < selectionCountBEFORE; ++i)
			{
				Protean::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedBEFORE.push_back(sel);
			}

			Protean::vector<Protean::PickResult> selectedAFTER;
			size_t selectionCountAFTER;
			archive >> selectionCountAFTER;
			for (size_t i = 0; i < selectionCountAFTER; ++i)
			{
				Protean::PickResult sel;
				archive >> sel.entity;
				archive >> sel.position;
				archive >> sel.normal;
				archive >> sel.subsetIndex;
				archive >> sel.distance;

				selectedAFTER.push_back(sel);
			}

			if (type == HISTORYOP_ADD_TO_SPLINE)
			{
				Protean::vector<Entity> modifiedSplineEntities;
				archive >> modifiedSplineEntities;
				EntitySerializer seri;
				seri.allow_remap = false;
				for (size_t i = 0; i < modifiedSplineEntities.size(); ++i)
				{
					scene.Entity_Remove(modifiedSplineEntities[i], false); // no recursive!!!
					scene.Entity_Serialize(archive, seri, INVALID_ENTITY, Protean::Scene::EntitySerializeFlags_KEEP_INTERNAL_ENTITY_REFERENCES); // no recursive!!!
				}
			}

			Protean::vector<Entity> addedEntities;
			archive >> addedEntities;

			ClearSelected();
			if (undo)
			{
				translator.selected = selectedBEFORE;
				for (size_t i = 0; i < addedEntities.size(); ++i)
				{
					scene.Entity_Remove(addedEntities[i]);
				}
			}
			else
			{
				translator.selected = selectedAFTER;
				EntitySerializer seri;
				seri.allow_remap = false;
				for (size_t i = 0; i < addedEntities.size(); ++i)
				{
					scene.Entity_Serialize(archive, seri);
				}
			}

		}
		break;
		case HISTORYOP_DELETE:
			{
				// Read selections states from archive:

				Protean::vector<Protean::PickResult> selectedBEFORE;
				size_t selectionCountBEFORE;
				archive >> selectionCountBEFORE;
				for (size_t i = 0; i < selectionCountBEFORE; ++i)
				{
					Protean::PickResult sel;
					archive >> sel.entity;
					archive >> sel.position;
					archive >> sel.normal;
					archive >> sel.subsetIndex;
					archive >> sel.distance;

					selectedBEFORE.push_back(sel);
				}

				Protean::vector<Entity> deletedEntities;
				archive >> deletedEntities;

				ClearSelected();
				if (undo)
				{
					translator.selected = selectedBEFORE;
					EntitySerializer seri;
					seri.allow_remap = false;
					for (size_t i = 0; i < deletedEntities.size(); ++i)
					{
						scene.Entity_Serialize(archive, seri);
					}
				}
				else
				{
					for (size_t i = 0; i < deletedEntities.size(); ++i)
					{
						scene.Entity_Remove(deletedEntities[i]);
					}
				}

			}
			break;
		case HISTORYOP_COMPONENT_DATA:
			{
				Scene before, after;
				Protean::vector<Entity> entities_before, entities_after;

				archive >> entities_before;
				for (auto& x : entities_before)
				{
					EntitySerializer seri;
					seri.allow_remap = false;
					before.Entity_Serialize(archive, seri);
				}

				archive >> entities_after;
				for (auto& x : entities_after)
				{
					EntitySerializer seri;
					seri.allow_remap = false;
					after.Entity_Serialize(archive, seri);
				}

				if (undo)
				{
					for (auto& x : entities_before)
					{
						scene.Entity_Remove(x);
					}
					scene.Merge(before);
				}
				else
				{
					for (auto& x : entities_after)
					{
						scene.Entity_Remove(x);
					}
					scene.Merge(after);
				}
			}
			break;
		case HISTORYOP_PAINTTOOL:
			paintToolWnd.ConsumeHistoryOperation(archive, undo);
			break;
		case HISTORYOP_NONE:
		default:
			assert(0);
			break;
		}

		if (undo)
		{
			editorscene.historyPos--;
		}

		scene.Update(0);
	}

	componentsWnd.RefreshEntityTree();
}

void EditorComponent::RegisterRecentlyUsed(std::string filename)
{
	filename = Protean::Helper::BackslashToForwardSlash(filename);
	{
		for (size_t i = 0; i < recentFilenames.size();)
		{
			if (recentFilenames[i].compare(filename) == 0)
			{
				recentFilenames.erase(recentFilenames.begin() + i);
			}
			else
			{
				i++;
			}
		}
		while (recentFilenames.size() >= maxRecentFilenames)
		{
			recentFilenames.erase(recentFilenames.begin());
		}
		recentFilenames.push_back(filename);
		auto& recent = main->config.GetSection("recent");
		for (size_t i = 0; i < recentFilenames.size(); ++i)
		{
			recent.Set(std::to_string(i).c_str(), recentFilenames[i]);
		}
	}
	{
		std::string folder = Protean::Helper::GetDirectoryFromPath(filename);
		for (size_t i = 0; i < recentFolders.size();)
		{
			if (recentFolders[i].compare(folder) == 0)
			{
				recentFolders.erase(recentFolders.begin() + i);
			}
			else
			{
				i++;
			}
		}
		while (recentFolders.size() >= maxRecentFolders)
		{
			recentFolders.erase(recentFolders.begin());
		}
		recentFolders.push_back(folder);
		auto& recent = main->config.GetSection("recent_folders");
		for (size_t i = 0; i < recentFolders.size(); ++i)
		{
			recent.Set(std::to_string(i).c_str(), recentFolders[i]);
		}
	}
	main->config.Commit();
	contentBrowserWnd.RefreshContent();
}

void EditorComponent::Open(std::string filename)
{
	if (main->IsScriptReplacement())
		return;
	std::string extension = Protean::Helper::toUpper(Protean::Helper::GetExtensionFromFileName(filename));

	FileType type = FileType_INVALID;
	auto it = filetypes.find(extension);
	if (it != filetypes.end())
	{
		type = it->second;
	}
	if (type == FileType_INVALID)
		return;

	if (type == FileType_LUA)
	{
		last_script_path = filename;
		main->config.Set("last_script_path", last_script_path);
		main->config.Commit();
		playButton.SetScriptTip("dofile(\"" + last_script_path + "\")");
		Protean::Lua::RunFile(filename);
		componentsWnd.RefreshEntityTree();
		RegisterRecentlyUsed(filename);
		return;
	}
	if (type == FileType_VIDEO)
	{
		GetCurrentScene().Entity_CreateVideo(Protean::Helper::GetFileNameFromPath(filename), filename);
		componentsWnd.RefreshEntityTree();
		return;
	}
	if (type == FileType_SOUND)
	{
		GetCurrentScene().Entity_CreateSound(Protean::Helper::GetFileNameFromPath(filename), filename);
		componentsWnd.RefreshEntityTree();
		return;
	}
	if (type == FileType_TEXT)
	{
		Entity entity = CreateEntity();
		Protean::SpriteFont& font = GetCurrentScene().fonts.Create(entity);
		font.params.h_align = Protean::Font::Alignment::WIFALIGN_CENTER;
		font.params.v_align = Protean::Font::Alignment::WIFALIGN_CENTER;
		font.params.scaling = 0.1f;
		font.params.size = 26;
		font.anim.typewriter.looped = true;
		GetCurrentScene().transforms.Create(entity).Translate(XMFLOAT3(0, 2, 0));
		GetCurrentScene().names.Create(entity) = "font";

		Protean::vector<uint8_t> filedata;
		Protean::Helper::FileRead(filename, filedata);
		std::string fileText;
		fileText.resize(filedata.size() + 1);
		std::memcpy(fileText.data(), filedata.data(), filedata.size());
		font.SetText(fileText);

		componentsWnd.RefreshEntityTree();
		return;
	}
	if (type == FileType_IMAGE)
	{
		Scene& scene = GetCurrentScene();
		Entity entity = CreateEntity();
		Protean::Sprite& sprite = scene.sprites.Create(entity);
		sprite = Protean::Sprite(filename);
		if (sprite.textureResource.IsValid())
		{
			const TextureDesc& desc = sprite.textureResource.GetTexture().GetDesc();
			sprite.params.siz = XMFLOAT2(1, float(desc.height) / float(desc.width));
		}
		else
		{
			sprite.params.siz = XMFLOAT2(1, 1);
		}
		sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
		sprite.anim.repeatable = true;
		scene.transforms.Create(entity).Translate(XMFLOAT3(0, 2, 0));
		scene.names.Create(entity) = Protean::Helper::GetFileNameFromPath(filename);
		componentsWnd.RefreshEntityTree();
		return;
	}

	RegisterRecentlyUsed(filename);

	// Capture the current scene ID to ensure content is loaded into the correct tab
	// even if the user switches tabs during async loading
	const uint64_t target_scene_id = GetCurrentEditorScene().id;

	Protean::JobSystem::Execute(loadmodel_workload, [=] (Protean::JobSystem::JobArgs) {
		Protean::Backlog::post("[Editor] started loading model: " + filename);
		Protean::Allocator::shared_ptr<Scene> scene = Protean::Allocator::make_shared_single<Scene>();
		if (type == FileType_WISCENE)
		{
			Protean::LoadModel(*scene, filename);
		}
		else if (type == FileType_OBJ)
		{
			ImportModel_OBJ(filename, *scene);
		}
		else if (type == FileType_GLTF || type == FileType_GLB || type == FileType_VRM || type == FileType_VRMA)
		{
			ImportModel_GLTF(filename, *scene);
		}
		else if (type == FileType_FBX)
		{
			ImportModel_FBX(filename, *scene);
		}
		else if (type == FileType_PLY)
		{
			ImportModel_PLY(filename, *scene);
		}

		Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=] (uint64_t userdata) {

			// Get the target scene (the one that was selected when Open was triggered)
			// If it was closed during loading, a new empty tab is created as fallback
			EditorScene& target_editorscene = GetOrCreateEditorSceneForLoading(target_scene_id);
			Protean::Scene& target_scene = target_editorscene.scene;

			const size_t camera_count_prev = target_scene.cameras.GetCount();
			const size_t transform_count_prev = target_scene.transforms.GetCount();

			if (type == FileType_WISCENE && target_editorscene.path.empty())
			{
				target_editorscene.path = filename;
			}

			target_scene.Merge(*scene);

			// Place the imported model in front of the camera:
			if (type != FileType_WISCENE && generalWnd.placeInFrontOfCameraCheckBox.GetCheck())
			{
				// Imported models always have a root transform entity
				if (transform_count_prev < target_scene.transforms.GetCount())
				{
					const Entity rootEntity = target_scene.transforms.GetEntity(transform_count_prev);
					TransformComponent* transform = target_scene.transforms.GetComponent(rootEntity);
					if (transform != nullptr)
					{
						transform->translation_local = GetPositionInFrontOfCamera();
						transform->SetDirty();
					}
				}
			}

			// Detect when the new scene contains a new camera, and snap the camera onto it:
			const size_t camera_count = target_scene.cameras.GetCount();
			if (camera_count > 0 && camera_count > camera_count_prev)
			{
				const Entity entity = target_scene.cameras.GetEntity(camera_count_prev);
				if (entity != INVALID_ENTITY)
				{
					const CameraComponent* cam = target_scene.cameras.GetComponent(entity);
					if (cam != nullptr)
					{
						target_editorscene.camera.Eye = cam->Eye;
						target_editorscene.camera.At = cam->At;
						target_editorscene.camera.Up = cam->Up;
						target_editorscene.camera.fov = cam->fov;
						target_editorscene.camera.zNearP = cam->zNearP;
						target_editorscene.camera.zFarP = cam->zFarP;
						target_editorscene.camera.focal_length = cam->focal_length;
						target_editorscene.camera.aperture_size = cam->aperture_size;
						target_editorscene.camera.aperture_shape = cam->aperture_shape;
						// camera aspect should be always for the current screen
						target_editorscene.camera.width = (float)renderPath->GetInternalResolution().x;
						target_editorscene.camera.height = (float)renderPath->GetInternalResolution().y;

						const TransformComponent* camera_transform = target_scene.transforms.GetComponent(entity);
						if (camera_transform != nullptr)
						{
							target_editorscene.camera_transform = *camera_transform;
							target_editorscene.camera.TransformCamera(target_editorscene.camera_transform);
						}

						target_editorscene.camera.UpdateCamera();
					}
				}
			}
			RefreshSceneList();

			componentsWnd.weatherWnd.UpdateData();
			componentsWnd.RefreshEntityTree();

			target_editorscene.has_unsaved_changes = false;
			RefreshSceneList();

			Protean::Backlog::post("[Editor] finished loading model: " + filename);
		});
	});
}
void EditorComponent::Save(const std::string& filename)
{
	struct SetAndClearFlagOnExit {
		bool& flag;
		SetAndClearFlagOnExit(bool& flag) : flag(flag) { flag = true; }
		~SetAndClearFlagOnExit() { flag = false; }
	} scoped(save_in_progress);

	const std::string extension = Protean::Helper::toUpper(Protean::Helper::GetExtensionFromFileName(filename));

	auto type = FileType_INVALID;
	const auto it = filetypes.find(extension);
	if (it != filetypes.end())
	{
		type = it->second;
	}
	if (type == FileType_INVALID)
		return;

	if (type == FileType_WISCENE || type == FileType_HEADER || type == FileType_CPP)
	{
		const bool dump_to_header = type == FileType_HEADER;
		const bool dump_to_cpp = type == FileType_CPP;

		Protean::Archive archive = (dump_to_header || dump_to_cpp) ? Protean::Archive() : Protean::Archive(filename, false);
		if (archive.IsOpen())
		{
			if (dump_to_header || dump_to_cpp)
			{
				archive.SetCompressionEnabled(true);
				// No thumbnail for header
			}
			else
			{
				archive.SetCompressionEnabled(generalWnd.saveCompressionCheckBox.GetCheck());

				Scene& currentScene = GetCurrentScene();
				currentScene.Update(0);

				// Create dedicated thumbnail renderpath with fixed dimensions
				Protean::RenderPath3D thumbnailRenderPath;
				thumbnailRenderPath.width = 512;
				thumbnailRenderPath.height = 256;
				thumbnailRenderPath.scene = &currentScene;

				Texture thumbnailScreenshot;

				// Setup camera for single root entity scene thumbnail
				if (SetupThumbnailCamera(thumbnailRenderPath))
				{
					// Render the thumbnail
					thumbnailRenderPath.PreUpdate();
					thumbnailRenderPath.Update(0);
					thumbnailRenderPath.PostUpdate();
					thumbnailRenderPath.PreRender();
					thumbnailRenderPath.Render();
					thumbnailScreenshot = CreateThumbnail(*thumbnailRenderPath.GetLastPostprocessRT(), 256, 128);
				}
				else
				{
					thumbnailScreenshot = CreateThumbnail(*renderPath->GetLastPostprocessRT(), 256, 128);
				}

				thumbnailRenderPath.DeleteGPUResources();

				archive.SetThumbnailAndResetPos(thumbnailScreenshot);
			}

			Scene& scene = GetCurrentScene();

			Protean::ResourceManager::Mode embed_mode = (Protean::ResourceManager::Mode)generalWnd.saveModeComboBox.GetItemUserData(generalWnd.saveModeComboBox.GetSelected());
			Protean::ResourceManager::SetMode(embed_mode);

			scene.Serialize(archive);

			if (dump_to_header)
			{
				archive.SaveHeaderFile(filename, Protean::Helper::RemoveExtension(Protean::Helper::GetFileNameFromPath(filename)));
			}
			else if (dump_to_cpp)
			{
				archive.SaveCPPFile(filename, Protean::Helper::RemoveExtension(Protean::Helper::GetFileNameFromPath(filename)));
			}
		}
		else
		{
			Protean::Helper::messageBox("Could not create " + filename + "!");
			return;
		}
	}
	else if (type == FileType_GLTF || type == FileType_GLB)
	{
		ExportModel_GLTF(filename, GetCurrentScene());
	}

	GetCurrentEditorScene().path = filename;
	GetCurrentEditorScene().has_unsaved_changes = false;
	RefreshSceneList();

	RegisterRecentlyUsed(filename);

	PostSaveText("Scene saved: ", GetCurrentEditorScene().path);
}

void EditorComponent::SaveAs()
{

	// also set and reset by Save()
	save_in_progress = true;

	Protean::Helper::FileDialogParams params;
	params.type = Protean::Helper::FileDialogParams::SAVE;
	params.description = "Wicked Scene (.wiscene) | GLTF Model (.gltf) | GLTF Binary Model (.glb) | C++ code (.h,.cpp)";
	params.extensions.push_back("wiscene");
	params.extensions.push_back("gltf");
	params.extensions.push_back("glb");
	params.extensions.push_back("h");
	params.extensions.push_back("cpp");
	Protean::Helper::FileDialog(params, [=](const std::string& fileName) {
		Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
			auto extension = Protean::Helper::toUpper(Protean::Helper::GetExtensionFromFileName(fileName));
			std::string filename = (!extension.compare("GLTF") || !extension.compare("GLB") || !extension.compare("H") || !extension.compare("CPP")) ? fileName : Protean::Helper::ForceExtension(fileName, params.extensions.front());
			Save(filename);
		});
	}, [this]() { save_in_progress = false; });
}

Texture EditorComponent::CreateThumbnail(Texture texture, uint32_t target_width, uint32_t target_height, bool mipmaps) const
{
	GraphicsDevice* device = GetDevice();
	CommandList cmd = device->BeginCommandList();
	Texture thumbnail = texture;

	// Overestimate actual size with aspect (note that downscale factor will be 4x later):
	uint32_t current_width = target_width;
	uint32_t current_height = target_height;
	while (current_width < thumbnail.desc.width || current_height < thumbnail.desc.height)
	{
		current_width *= 4;
		current_height *= 4;
	}

	// It is invalid top create texture above 16K resolution, so force downsize if this happens:
	while (current_width > 16384 || current_height > 16384)
	{
		current_width /= 2;
		current_height /= 2;
	}

	// Crop target:
	{
		TextureDesc desc = thumbnail.desc;
		desc.format = Format_R8G8B8A8_UNORM;
		desc.width = current_width;
		desc.height = current_height;
		desc.mip_levels = mipmaps? 0 : 1;
		desc.bind_flags = BindFlag_SHADER_RESOURCE | BindFlag_RENDER_TARGET | BindFlag_UNORDERED_ACCESS;
		Texture upsized;
		device->CreateTexture(&desc, nullptr, &upsized);
		device->SetName(&upsized, "editor.crop.upsized");

		Viewport vp;
		vp.width = (float)desc.width;
		vp.height = (float)desc.height;
		device->BindViewports(1, &vp, cmd);

		RenderPassImage rp = RenderPassImage::RenderTarget(&upsized, RenderPassImage::LoadOp_CLEAR);
		device->RenderPassBegin(&rp, 1, cmd);

		Protean::Canvas canvas;
		canvas.width = desc.width;
		canvas.height = desc.height;
		Protean::Image::SetCanvas(canvas);

		Protean::Image::Params fx;
		fx.blendFlag = Protean::BLENDMODE_OPAQUE;

		const float canvas_aspect = canvas.GetLogicalWidth() / canvas.GetLogicalHeight();
		const float image_aspect = float(thumbnail.desc.width) / float(thumbnail.desc.height);

		if (canvas_aspect > image_aspect)
		{
			// display aspect is wider than image:
			fx.siz.x = canvas.GetLogicalWidth();
			fx.siz.y = canvas.GetLogicalHeight() / image_aspect * canvas_aspect;
		}
		else
		{
			// image aspect is wider or equal to display
			fx.siz.x = canvas.GetLogicalWidth() / canvas_aspect * image_aspect;
			fx.siz.y = canvas.GetLogicalHeight();
		}

		fx.pos = XMFLOAT3(canvas.GetLogicalWidth() * 0.5f, canvas.GetLogicalHeight() * 0.5f, 0);
		fx.pivot = XMFLOAT2(0.5f, 0.5f);

		Protean::Image::Draw(&thumbnail, fx, cmd);

		device->RenderPassEnd(cmd);

		Protean::Image::SetCanvas(*this);

		thumbnail = upsized;
	}

	// Downsize until target size is reached:
	while (thumbnail.desc.width > target_width || thumbnail.desc.height > target_height)
	{
		TextureDesc desc = thumbnail.desc;
		desc.format = Format_R8G8B8A8_UNORM;
		desc.width = std::max(target_width, desc.width / 4u);
		desc.height = std::max(target_height, desc.height / 4u);
		desc.mip_levels = mipmaps ? 0 : 1;
		desc.bind_flags = BindFlag_SHADER_RESOURCE | BindFlag_RENDER_TARGET | BindFlag_UNORDERED_ACCESS;
		Texture downsized;
		device->CreateTexture(&desc, nullptr, &downsized);
		device->SetName(&downsized, "editor.crop.downsized");
		Protean::Renderer::Postprocess_Downsample4x(thumbnail, downsized, cmd);
		thumbnail = downsized;
	}

	if (mipmaps)
	{
		device->CreateMipgenSubresources(thumbnail);
		Protean::Renderer::GenerateMipChain(thumbnail, Protean::Renderer::MIPGENFILTER_LINEAR, cmd);
	}

	// Ensure GPU has finished rendering
	device->SubmitCommandLists();
	device->WaitForGPU();

	return thumbnail;
}

bool EditorComponent::SetupThumbnailCamera(Protean::RenderPath3D& thumbnailRenderPath)
{
	Scene& scene = GetCurrentScene();

	// Check if scene has exactly one root entity
	Protean::unordered_set<Entity> all_entities;
	scene.FindAllEntities(all_entities);

	Entity root_entity = INVALID_ENTITY;
	for (Entity entity : all_entities)
	{
		const HierarchyComponent* hierarchy_component = scene.hierarchy.GetComponent(entity);
		if (hierarchy_component == nullptr || hierarchy_component->parentID == INVALID_ENTITY)
		{
			if (root_entity != INVALID_ENTITY)
			{
				return false; // More than one root entity found
			}
			root_entity = entity;
		}
	}

	if (root_entity == INVALID_ENTITY)
		return false;

	AABB combined_bounds;
	bool has_bounds = false;

	auto add_entity_bounds = [&](const Entity entity) {
		const ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object != nullptr && object->IsRenderable() && object->meshID != INVALID_ENTITY)
		{
			const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			const TransformComponent* transform = scene.transforms.GetComponent(entity);
			if (mesh != nullptr && transform != nullptr && mesh->aabb.IsValid())
			{
				const XMMATRIX world_transform = XMLoadFloat4x4(&transform->world);
				AABB world_aabb = mesh->aabb.transform(world_transform);
				combined_bounds = has_bounds ? AABB::Merge(combined_bounds, world_aabb) : world_aabb;
				has_bounds = true;
			}
		}
	};

	add_entity_bounds(root_entity);

	scene.ForEachChild(root_entity, [&](const Entity child) {
		add_entity_bounds(child);
	});

	// Only adjust camera if we have valid bounds
	if (!has_bounds || combined_bounds.getArea() <= 0)
		return false;

	// Create temp camera for thumbnail
	XMFLOAT3 bounds_center = combined_bounds.getCenter();
	float bounds_radius = combined_bounds.getRadius();
	float camera_distance = bounds_radius * 2.0f; // Distance multiplier for framing

	float horizontal_angle = Protean::Math::DegreesToRadians(-50.0f);
	float elevation_angle = Protean::Math::DegreesToRadians(20.0f);
	float horizontal_distance = camera_distance * std::cos(elevation_angle);
	float vertical_offset = camera_distance * std::sin(elevation_angle);

	auto camera_position = XMFLOAT3(
		bounds_center.x + horizontal_distance * std::cos(horizontal_angle),
		bounds_center.y + vertical_offset,
		bounds_center.z + horizontal_distance * std::sin(horizontal_angle)
	);

	TransformComponent temp_camera_transform;
	temp_camera_transform.Translate(XMLoadFloat3(&camera_position));
	temp_camera_transform.UpdateTransform();

	XMVECTOR eye = XMLoadFloat3(&camera_position);
	XMVECTOR at = XMLoadFloat3(&bounds_center);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX view = XMMatrixLookAtLH(eye, at, up);
	XMMATRIX view_inv = XMMatrixInverse(nullptr, view);
	XMStoreFloat4x4(&temp_camera_transform.world, view_inv);

	CameraComponent temp_camera;
	temp_camera.fov = Protean::Math::DegreesToRadians(60.0f);
	temp_camera.width = (float)thumbnailRenderPath.width;
	temp_camera.height = (float)thumbnailRenderPath.height;
	temp_camera.TransformCamera(temp_camera_transform);
	temp_camera.UpdateCamera();

	// Set the thumbnail renderpath camera
	thumbnail_saved_camera = temp_camera;
	thumbnailRenderPath.camera = &thumbnail_saved_camera;

	return true;
}

void EditorComponent::PostSaveText(const std::string& message, const std::string& filename, float time_seconds)
{
	save_text_message = message;
	save_text_filename = filename;
	save_text_alpha = time_seconds;
	Protean::Backlog::post(message + filename);
}

void EditorComponent::CheckBonePickingEnabled()
{
	const Scene& scene = GetCurrentScene();

	if (generalWnd.bonePickerOpacitySlider.GetValue() <= 0.0f)
	{
		bone_picking = false;
		return;
	}

	if (generalWnd.skeletonsVisibleCheckBox.GetCheck())
	{
		bone_picking = true;
	}
	else
	{
		// Check if armature or bone or humanoid is selected to allow bone picking:
		//	(Don't want to always enable bone picking, because it can make the screen look very busy.)

		bone_picking = false;
		for (size_t i = 0; i < scene.humanoids.GetCount() && !bone_picking; ++i)
		{
			Entity entity = scene.humanoids.GetEntity(i);
			for (auto& x : translator.selected)
			{
				if (entity == x.entity)
				{
					bone_picking = true;
					break;
				}
			}
			for (Entity bone : scene.humanoids[i].bones)
			{
				for (auto& x : translator.selected)
				{
					if (bone == x.entity)
					{
						bone_picking = true;
						break;
					}
				}
			}
		}
		for (size_t i = 0; i < scene.armatures.GetCount() && !bone_picking; ++i)
		{
			Entity entity = scene.armatures.GetEntity(i);
			for (auto& x : translator.selected)
			{
				if (entity == x.entity)
				{
					bone_picking = true;
					break;
				}
			}
			for (Entity bone : scene.armatures[i].boneCollection)
			{
				for (auto& x : translator.selected)
				{
					if (bone == x.entity)
					{
						bone_picking = true;
						break;
					}
				}
			}
		}
	}

	if (!bone_picking)
		return;

	bone_picking_items.clear();
	auto bone_pick_iterate = [&](const Entity* entities, size_t entity_count) {
		for (size_t i = 0; i < entity_count; ++i)
		{
			Entity entity = entities[i];
			if (entity == INVALID_ENTITY)
				continue;
			if (bone_picking_items.count(entity) > 0)
				continue;
			if (!scene.transforms.Contains(entity))
				continue;
			const TransformComponent& transform = *scene.transforms.GetComponent(entity);
			XMVECTOR a = transform.GetPositionV();
			XMVECTOR b = a + XMVectorSet(0, 0.1f, 0, 0);
			const SpringComponent* spring = scene.springs.GetComponent(entity);
			if (spring != nullptr)
			{
				// Spring has information about bone tip already:
				b = XMLoadFloat3(&spring->currentTail);
			}
			else
			{
				// Search for child to connect bone tip:
				bool child_found = false;
				for (size_t h = 0; (h < scene.humanoids.GetCount()) && !child_found; ++h)
				{
					const HumanoidComponent& humanoid = scene.humanoids[h];
					int bodypart = 0;
					for (Entity child : humanoid.bones)
					{
						const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(child);
						if (hierarchy != nullptr && hierarchy->parentID == entity && scene.transforms.Contains(child))
						{
							if (bodypart == int(HumanoidComponent::HumanoidBone_Hips))
							{
								// skip root-hip connection
								child_found = true;
								break;
							}
							const TransformComponent& child_transform = *scene.transforms.GetComponent(child);
							b = child_transform.GetPositionV();
							child_found = true;
							break;
						}
						bodypart++;
					}
				}
				if (!child_found)
				{
					for (size_t j = 0; j < entity_count; ++j)
					{
						Entity child = entities[j];
						const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(child);
						if (hierarchy != nullptr && hierarchy->parentID == entity && scene.transforms.Contains(child))
						{
							const TransformComponent& child_transform = *scene.transforms.GetComponent(child);
							b = child_transform.GetPositionV();
							child_found = true;
							break;
						}
					}
				}
				if (!child_found)
				{
					// No child, try to guess bone tip compared to parent (if it has parent):
					const HierarchyComponent* hierarchy = scene.hierarchy.GetComponent(entity);
					if (hierarchy != nullptr && scene.transforms.Contains(hierarchy->parentID))
					{
						const TransformComponent& parent_transform = *scene.transforms.GetComponent(hierarchy->parentID);
						XMVECTOR ab = a - parent_transform.GetPositionV();
						b = a + ab;
					}
				}
			}
			XMVECTOR ab = XMVector3Normalize(b - a);

			Protean::Capsule capsule;
			capsule.radius = Protean::Math::Distance(a, b) * 0.1f;
			a -= ab * capsule.radius;
			b += ab * capsule.radius;
			XMStoreFloat3(&capsule.base, a);
			XMStoreFloat3(&capsule.tip, b);

			bone_picking_items[entity] = capsule;
		}
	};
	for (size_t i = 0; i < scene.humanoids.GetCount(); ++i)
	{
		const HumanoidComponent& humanoid = scene.humanoids[i];
		bone_pick_iterate(humanoid.bones, arraysize(humanoid.bones));
	}
	for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
	{
		const ArmatureComponent& armature = scene.armatures[i];
		bone_pick_iterate(armature.boneCollection.data(), armature.boneCollection.size());
	}
}

void EditorComponent::UpdateLocalGlobalButton()
{
	if (translator.isLocalSpace)
	{
		localGlobalButton.SetText(ICON_LOCAL_SPACE);
		localGlobalButton.SetTooltip("Toggle from Local coordinate space to Global for Translate and Rotate\nHotkey: " + GetInputString(EditorActions_LOCAL_GLOBAL_TOGGLE_ACTION));
	}
	else
	{
		localGlobalButton.SetText(ICON_GLOBAL_SPACE);
		localGlobalButton.SetTooltip("Toggle from Global coordinate space to Local for Translate and Rotate\nHotkey: " + GetInputString(EditorActions_LOCAL_GLOBAL_TOGGLE_ACTION));
	}
}

void EditorComponent::UpdateDynamicWidgets()
{
	float screenW = GetLogicalWidth();
	float screenH = GetLogicalHeight();
	float wid_idle = 40;
	float wid_focus = wid_idle * 2.5f;
	float padding = 4;
	float lerp = 0.4f;

	bool fullscreen = main->config.GetBool("fullscreen");

	static std::string tmp;

	if (saveButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_Save) != nullptr)
	{
		tmp = ICON_SAVE " ";
		tmp += current_localization.Get((size_t)EditorLocalization_Save);
		saveButton.SetText(tmp);
	}
	else
	{
		saveButton.SetText(saveButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_SAVE " Save" : ICON_SAVE);
	}

	if (openButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_Open) != nullptr)
	{
		tmp = ICON_OPEN " ";
		tmp += current_localization.Get((size_t)EditorLocalization_Open);
		openButton.SetText(tmp);
	}
	else
	{
		openButton.SetText(openButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_OPEN " Open" : ICON_OPEN);
	}

	if (contentBrowserButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_ContentBrowser) != nullptr)
	{
		tmp = ICON_CONTENT_BROWSER " ";
		tmp += current_localization.Get((size_t)EditorLocalization_ContentBrowser);
		contentBrowserButton.SetText(tmp);
	}
	else
	{
		contentBrowserButton.SetText(contentBrowserButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_CONTENT_BROWSER " Content" : ICON_CONTENT_BROWSER);
	}


	if (logButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_Backlog) != nullptr)
	{
		tmp = ICON_BACKLOG " ";
		tmp += current_localization.Get((size_t)EditorLocalization_Backlog);
		logButton.SetText(tmp);
	}
	else
	{
		logButton.SetText(logButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_BACKLOG " Backlog" : ICON_BACKLOG);
	}

	if (profilerButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_Profiler) != nullptr)
	{
		tmp = ICON_PROFILER " ";
		tmp += current_localization.Get((size_t)EditorLocalization_Profiler);
		profilerButton.SetText(tmp);
	}
	else
	{
		profilerButton.SetText(profilerButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_PROFILER " Profiler" : ICON_PROFILER);
	}

	if (bugButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_BugReport) != nullptr)
	{
		tmp = ICON_BUG " ";
		tmp += current_localization.Get((size_t)EditorLocalization_BugReport);
		bugButton.SetText(tmp);
	}
	else
	{
		bugButton.SetText(bugButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_BUG " Bug report" : ICON_BUG);
	}

	if (fullscreen)
	{
		if (fullscreenButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_Windowed) != nullptr)
		{
			tmp = ICON_WINDOWED " ";
			tmp += current_localization.Get((size_t)EditorLocalization_Windowed);
			fullscreenButton.SetText(tmp);
		}
		else
		{
			fullscreenButton.SetText(fullscreenButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_WINDOWED " Windowed" : ICON_WINDOWED);
		}
	}
	else
	{
		if (fullscreenButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_FullScreen) != nullptr)
		{
			tmp = ICON_FULLSCREEN " ";
			tmp += current_localization.Get((size_t)EditorLocalization_FullScreen);
			fullscreenButton.SetText(tmp);
		}
		else
		{
			fullscreenButton.SetText(fullscreenButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_FULLSCREEN " Full screen" : ICON_FULLSCREEN);
		}
	}

	if (aboutButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_About) != nullptr)
	{
		tmp = ICON_HELP " ";
		tmp += current_localization.Get((size_t)EditorLocalization_About);
		aboutButton.SetText(tmp);
	}
	else
	{
		aboutButton.SetText(aboutButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_HELP " About" : ICON_HELP);
	}

	if (exitButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE && current_localization.Get((size_t)EditorLocalization_Exit) != nullptr)
	{
		tmp = ICON_EXIT " ";
		tmp += current_localization.Get((size_t)EditorLocalization_Exit);
		exitButton.SetText(tmp);
	}
	else
	{
		exitButton.SetText(exitButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? ICON_EXIT " Exit" : ICON_EXIT);
	}

	float hei = topmenuWnd.GetSize().y - topmenuWnd.GetShadowRadius() - 2;
	float y = topmenuWnd.GetShadowRadius() + 2;

	exitButton.SetSize(XMFLOAT2(Protean::Math::Lerp(exitButton.GetSize().x, exitButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	aboutButton.SetSize(XMFLOAT2(Protean::Math::Lerp(aboutButton.GetSize().x, aboutButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	fullscreenButton.SetSize(XMFLOAT2(Protean::Math::Lerp(fullscreenButton.GetSize().x, fullscreenButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	bugButton.SetSize(XMFLOAT2(Protean::Math::Lerp(bugButton.GetSize().x, bugButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	profilerButton.SetSize(XMFLOAT2(Protean::Math::Lerp(profilerButton.GetSize().x, profilerButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	logButton.SetSize(XMFLOAT2(Protean::Math::Lerp(logButton.GetSize().x, logButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	contentBrowserButton.SetSize(XMFLOAT2(Protean::Math::Lerp(contentBrowserButton.GetSize().x, contentBrowserButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	openButton.SetSize(XMFLOAT2(Protean::Math::Lerp(openButton.GetSize().x, openButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));
	saveButton.SetSize(XMFLOAT2(Protean::Math::Lerp(saveButton.GetSize().x, saveButton.GetState() > Protean::Gui::WIDGETSTATE::IDLE ? wid_focus : wid_idle, lerp), hei));

	exitButton.SetPos(XMFLOAT2(screenW - exitButton.GetSize().x, y));
	aboutButton.SetPos(XMFLOAT2(exitButton.GetPos().x - aboutButton.GetSize().x - padding, y));
	bugButton.SetPos(XMFLOAT2(aboutButton.GetPos().x - bugButton.GetSize().x - padding, y));
	fullscreenButton.SetPos(XMFLOAT2(bugButton.GetPos().x - fullscreenButton.GetSize().x - padding, y));
	profilerButton.SetPos(XMFLOAT2(fullscreenButton.GetPos().x - profilerButton.GetSize().x - padding, y));
	logButton.SetPos(XMFLOAT2(profilerButton.GetPos().x - logButton.GetSize().x - padding, y));
	contentBrowserButton.SetPos(XMFLOAT2(logButton.GetPos().x - contentBrowserButton.GetSize().x - padding, y));
	openButton.SetPos(XMFLOAT2(contentBrowserButton.GetPos().x - openButton.GetSize().x - padding, y));
	saveButton.SetPos(XMFLOAT2(openButton.GetPos().x - saveButton.GetSize().x - padding, y));


	float static_pos = screenW - wid_idle * 12;

	projectCreatorButton.SetSize(XMFLOAT2(wid_idle * 0.75f, hei));
	projectCreatorButton.SetPos(XMFLOAT2(static_pos - stopButton.GetSize().x - 20, y));
	stopButton.SetSize(XMFLOAT2(wid_idle * 0.75f, hei));
	stopButton.SetPos(XMFLOAT2(projectCreatorButton.GetPos().x - playButton.GetSize().x - padding, y));
	playButton.SetSize(XMFLOAT2(wid_idle * 0.75f, hei));
	playButton.SetPos(XMFLOAT2(stopButton.GetPos().x - playButton.GetSize().x - padding, y));

	float ofs = 0;
	wid_idle = 105;
	float mid = 0;
	for (int i = 0; i < int(scenes.size()); ++i)
	{
		auto& editorscene = scenes[i];
		editorscene->tabSelectButton.SetShadowRadius(topmenuWnd.GetShadowRadius());
		editorscene->tabCloseButton.SetShadowRadius(topmenuWnd.GetShadowRadius());
		editorscene->tabSelectButton.SetSize(XMFLOAT2(wid_idle, hei));
		editorscene->tabCloseButton.SetSize(XMFLOAT2(hei, hei));
		editorscene->tabSelectButton.SetPos(XMFLOAT2(ofs, y));
		ofs += editorscene->tabSelectButton.GetSize().x + editorscene->tabSelectButton.GetShadowRadius();
		editorscene->tabCloseButton.SetPos(XMFLOAT2(ofs, y));
		ofs += editorscene->tabCloseButton.GetSize().x + editorscene->tabCloseButton.GetShadowRadius();
		ofs += padding;
		mid = editorscene->tabSelectButton.GetPos().y + editorscene->tabSelectButton.GetSize().y * 0.5f;
	}
	hei = 22;
	newSceneButton.SetShadowRadius(0);
	newSceneButton.SetSize(XMFLOAT2(hei, hei));
	newSceneButton.SetPos(XMFLOAT2(ofs, mid - hei * 0.5f));

	hei = 24;
	padding = 8;
	ofs = screenW - padding - hei;
	if (componentsWnd.IsVisible())
	{
		ofs -= componentsWnd.GetSize().x + componentsWnd.GetShadowRadius();
	}
	y = topmenuWnd.GetSize().y + padding;

	translateButton.SetSize(XMFLOAT2(hei, hei));
	translateButton.SetPos(XMFLOAT2(ofs, y));
	translateButton.Update(*this, 0);
	y += translateButton.GetSize().y + translateButton.GetShadowRadius();

	rotateButton.SetSize(XMFLOAT2(hei, hei));
	rotateButton.SetPos(XMFLOAT2(ofs, y));
	rotateButton.Update(*this, 0);
	y += rotateButton.GetSize().y + rotateButton.GetShadowRadius();

	scaleButton.SetSize(XMFLOAT2(hei, hei));
	scaleButton.SetPos(XMFLOAT2(ofs, y));
	scaleButton.Update(*this, 0);
	y += scaleButton.GetSize().y + scaleButton.GetShadowRadius();

	localGlobalButton.SetSize(XMFLOAT2(hei, hei));
	localGlobalButton.SetPos(XMFLOAT2(ofs, y));
	localGlobalButton.Update(*this, 0);
	y += localGlobalButton.GetSize().y + padding;

	physicsButton.SetSize(XMFLOAT2(hei, hei));
	physicsButton.SetPos(XMFLOAT2(ofs, y));
	physicsButton.Update(*this, 0);
	y += physicsButton.GetSize().y + padding;

	dummyButton.SetSize(XMFLOAT2(hei, hei));
	dummyButton.SetPos(XMFLOAT2(ofs, y));
	dummyButton.Update(*this, 0);
	y += dummyButton.GetSize().y + padding;

	navtestButton.SetSize(XMFLOAT2(hei, hei));
	navtestButton.SetPos(XMFLOAT2(ofs, y));
	navtestButton.Update(*this, 0);
	y += navtestButton.GetSize().y + padding;

	cinemaButton.SetSize(XMFLOAT2(hei, hei));
	cinemaButton.SetPos(XMFLOAT2(ofs, y));
	cinemaButton.Update(*this, 0);
	y += cinemaButton.GetSize().y + padding;


	newEntityCombo.SetSize(XMFLOAT2(hei * 1.4f, hei * 1.4f));
	ofs -= padding * 2 + newEntityCombo.GetSize().x;
	y = topmenuWnd.GetSize().y + padding;
	newEntityCombo.SetPos(XMFLOAT2(ofs, y));
	newEntityCombo.Update(*this, 0);
	newEntityCombo.SetText(""); // override localization


	XMFLOAT4 color_on = playButton.sprites[Protean::Gui::FOCUS].params.color;
	XMFLOAT4 color_off = playButton.sprites[Protean::Gui::IDLE].params.color;

	if (translator.isTranslator)
	{
		translateButton.sprites[Protean::Gui::IDLE].params.color = color_on;
		rotateButton.sprites[Protean::Gui::IDLE].params.color = color_off;
		scaleButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}
	else if (translator.isRotator)
	{
		translateButton.sprites[Protean::Gui::IDLE].params.color = color_off;
		rotateButton.sprites[Protean::Gui::IDLE].params.color = color_on;
		scaleButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}
	else if (translator.isScalator)
	{
		translateButton.sprites[Protean::Gui::IDLE].params.color = color_off;
		rotateButton.sprites[Protean::Gui::IDLE].params.color = color_off;
		scaleButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}

	if (dummy_enabled)
	{
		dummyButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}
	else
	{
		dummyButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}

	if (navtest_enabled)
	{
		navtestButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}
	else
	{
		navtestButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}

	if (Protean::Physics::IsSimulationEnabled())
	{
		physicsButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}
	else
	{
		physicsButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}



	if (Protean::Backlog::GetUnseenLogLevelMax() >= Protean::Backlog::LogLevel_Error)
	{
		const float pulse = std::sin(outlineTimer * XM_2PI * 1.5f) * 0.5f + 0.5f;
		const XMFLOAT4 alertColor = Protean::Math::Lerp( Protean::Color::Error(), XMFLOAT4(0, 0, 0, 1), pulse * 0.6f);
		logButton.sprites[Protean::Gui::IDLE].params.color = alertColor;
		logButton.sprites[Protean::Gui::IDLE].params.gradient = Protean::Image::Params::Gradient_None;
		logButton.sprites[Protean::Gui::FOCUS].params.color = alertColor;
		logButton.sprites[Protean::Gui::FOCUS].params.gradient = Protean::Image::Params::Gradient_None;
	}
	else if (Protean::Backlog::GetUnseenLogLevelMax() >= Protean::Backlog::LogLevel_Warning)
	{
		const float pulse = std::sin(outlineTimer * XM_2PI * 1.5f) * 0.5f + 0.5f;
		const XMFLOAT4 alertColor = Protean::Math::Lerp(Protean::Color::Warning(), XMFLOAT4(0, 0, 0, 1), pulse * 0.6f);
		logButton.sprites[Protean::Gui::IDLE].params.color = alertColor;
		logButton.sprites[Protean::Gui::IDLE].params.gradient = Protean::Image::Params::Gradient_None;
		logButton.sprites[Protean::Gui::FOCUS].params.color = alertColor;
		logButton.sprites[Protean::Gui::FOCUS].params.gradient = Protean::Image::Params::Gradient_None;
	}
	else
	{
		logButton.sprites[Protean::Gui::IDLE].params.color = color_off;
		logButton.sprites[Protean::Gui::IDLE].params.gradient = generalWnd.disableGradientCheckBox.GetCheck() ? Protean::Image::Params::Gradient_None : Protean::Image::Params::Gradient_Linear;
		logButton.sprites[Protean::Gui::FOCUS].params.color = color_on;
		logButton.sprites[Protean::Gui::FOCUS].params.gradient = Protean::Image::Params::Gradient_None;
	}

	ofs = padding;
	
	if (generalWnd.IsVisible())
	{
		generalWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		generalWnd.SetSize(XMFLOAT2(generalWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += generalWnd.scale_local.x + generalWnd.GetShadowRadius();
		generalButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}
	else
	{
		generalButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}
	if (graphicsWnd.IsVisible())
	{
		graphicsWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		graphicsWnd.SetSize(XMFLOAT2(graphicsWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += graphicsWnd.scale_local.x + graphicsWnd.GetShadowRadius();
		graphicsButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}
	else
	{
		graphicsButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}
	if (cameraWnd.IsVisible())
	{
		cameraWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		cameraWnd.SetSize(XMFLOAT2(cameraWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += cameraWnd.scale_local.x + cameraWnd.GetShadowRadius();
		cameraButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}
	else
	{
		cameraButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}
	if (materialPickerWnd.IsVisible())
	{
		materialPickerWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		materialPickerWnd.SetSize(XMFLOAT2(materialPickerWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += materialPickerWnd.scale_local.x + materialPickerWnd.GetShadowRadius();
		materialsButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}
	else
	{
		materialsButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}
	if (paintToolWnd.IsVisible())
	{
		paintToolWnd.SetPos(XMFLOAT2(0, topmenuWnd.scale_local.y + topmenuWnd.GetShadowRadius()));
		paintToolWnd.SetSize(XMFLOAT2(paintToolWnd.scale_local.x, screenH - topmenuWnd.scale_local.y - topmenuWnd.GetShadowRadius()));
		ofs += paintToolWnd.scale_local.x + paintToolWnd.GetShadowRadius();
		paintToolButton.sprites[Protean::Gui::IDLE].params.color = color_on;
	}
	else
	{
		paintToolButton.sprites[Protean::Gui::IDLE].params.color = color_off;
	}
	y = padding + topmenuWnd.GetSize().y + topmenuWnd.GetShadowRadius();
	hei = 40;
	
	ofs = std::max(ofs, GetSafeInsetLeftLogical());

	generalButton.SetPos(XMFLOAT2(ofs, y));
	generalButton.SetSize(XMFLOAT2(hei, hei));
	generalButton.Update(*this, 0);
	y += hei + padding;

	graphicsButton.SetPos(XMFLOAT2(ofs, y));
	graphicsButton.SetSize(XMFLOAT2(hei, hei));
	graphicsButton.Update(*this, 0);
	y += hei + padding;

	cameraButton.SetPos(XMFLOAT2(ofs, y));
	cameraButton.SetSize(XMFLOAT2(hei, hei));
	cameraButton.Update(*this, 0);
	y += hei + padding;

	materialsButton.SetPos(XMFLOAT2(ofs, y));
	materialsButton.SetSize(XMFLOAT2(hei, hei));
	materialsButton.Update(*this, 0);
	y += hei + padding;

	paintToolButton.SetPos(XMFLOAT2(ofs, y));
	paintToolButton.SetSize(XMFLOAT2(hei, hei));
	paintToolButton.Update(*this, 0);
	y += hei + padding;


	guiScalingCombo.SetSize(XMFLOAT2(50, 18));
	guiScalingCombo.SetPos(XMFLOAT2(ofs, screenH - guiScalingCombo.GetSize().y - padding));

	if (is_2D_mode)
	{
		is2DModeButton.SetText("2D");
	}
	else
	{
		is2DModeButton.SetText("3D");
	}
	is2DModeButton.SetSize(XMFLOAT2(50, 18));
	is2DModeButton.SetPos(XMFLOAT2(guiScalingCombo.GetPos().x + guiScalingCombo.GetSize().x + 10, guiScalingCombo.GetPos().y));
	is2DModeButton.Update(*this, 0);
}

void EditorComponent::SetCurrentScene(int index)
{
	current_scene = index;
	this->renderPath->scene = &scenes[current_scene].get()->scene;
	this->renderPath->camera = &scenes[current_scene].get()->camera;
	Protean::Lua::SceneBindings::SetGlobalScene(renderPath->scene);
	Protean::Lua::SceneBindings::SetGlobalCamera(renderPath->camera);
	componentsWnd.RefreshEntityTree();
	RefreshSceneList();

	EditorScene& editorscene = GetCurrentEditorScene();
	if (editorscene.path.empty() && !editorscene.untitled_camera_reset_once)
	{
		cameraWnd.ResetCam();
		editorscene.untitled_camera_reset_once = true;
	}
}
void EditorComponent::RefreshSceneList()
{
	generalWnd.RefreshTheme();
	for (int i = 0; i < int(scenes.size()); ++i)
	{
		auto& editorscene = scenes[i];
		std::string tabText;
		if (editorscene->path.empty())
		{
			if (current_localization.Get((size_t)EditorLocalization_UntitledScene))
			{
				tabText = current_localization.Get((size_t)EditorLocalization_UntitledScene);
			}
			else
			{
				tabText = "Untitled scene";
			}
			editorscene->tabSelectButton.SetTooltip("");
		}
		else
		{
			tabText = Protean::Helper::GetFileNameFromPath(editorscene->path);
			editorscene->tabSelectButton.SetTooltip(editorscene->path);
		}

		if (editorscene->has_unsaved_changes)
		{
			tabText = tabText + " *";
		}

		editorscene->tabSelectButton.SetText(tabText);

		editorscene->tabSelectButton.OnClick([this, i](Protean::Gui::EventArgs args) {
			SetCurrentScene(i);
			});
		editorscene->tabCloseButton.OnClick([this, i](Protean::Gui::EventArgs args) {
			// Check for unsaved changes before closing
			if (!CheckUnsavedChanges(i))
			{
				return;
			}

			Protean::Lua::KillProcesses();

			translator.selected.clear();
			Protean::Scene& scene = scenes[i]->scene;
			Protean::Renderer::ClearWorld(scene);
			cameraWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.objectWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.meshWnd.SetEntity(Protean::INVALID_ENTITY, -1);
			componentsWnd.lightWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.soundWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.videoWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.decalWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.envProbeWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.materialWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.emitterWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.hairWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.forceFieldWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.springWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.ikWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.transformWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.layerWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.nameWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.animWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.scriptWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.rigidWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.softWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.colliderWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.hierarchyWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.cameraComponentWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.expressionWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.armatureWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.humanoidWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.terrainWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.spriteWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.fontWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.voxelGridWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.metadataWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.constraintWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.splineWnd.SetEntity(Protean::INVALID_ENTITY);
			componentsWnd.gaussiansplatWnd.SetEntity(Protean::INVALID_ENTITY);

			componentsWnd.RefreshEntityTree();
			ResetHistory();
			scenes[i]->path.clear();
			scenes[i]->untitled_camera_reset_once = false;

			Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				if (scenes.size() > 1)
				{
					topmenuWnd.RemoveWidget(&scenes[i]->tabSelectButton);
					topmenuWnd.RemoveWidget(&scenes[i]->tabCloseButton);
					scenes.erase(scenes.begin() + i);
				}
				SetCurrentScene(std::max(0, i - 1));
			});
		});
	}
}
void EditorComponent::NewScene()
{
	int scene_id = int(scenes.size());
	scenes.push_back(std::make_unique<EditorScene>());
	auto& editorscene = scenes.back();
	editorscene->id = next_scene_id++;
	editorscene->tabSelectButton.Create("");
	editorscene->tabSelectButton.SetLocalizationEnabled(false);
	editorscene->tabCloseButton.Create("X");
	editorscene->tabCloseButton.SetLocalizationEnabled(false);
	editorscene->tabCloseButton.SetTooltip("Close scene. This operation cannot be undone!");
	topmenuWnd.AddWidget(&editorscene->tabSelectButton);
	topmenuWnd.AddWidget(&editorscene->tabCloseButton);
	SetCurrentScene(scene_id);
	RefreshSceneList();
	UpdateDynamicWidgets();
	cameraWnd.ResetCam();
}

EditorComponent::EditorScene* EditorComponent::FindEditorSceneByID(uint64_t id) const
{
	for (auto& scene : scenes)
	{
		if (scene->id == id)
		{
			return scene.get();
		}
	}
	return nullptr;
}

EditorComponent::EditorScene& EditorComponent::GetOrCreateEditorSceneForLoading(uint64_t target_scene_id)
{
	EditorScene* target = FindEditorSceneByID(target_scene_id);
	if (target != nullptr)
	{
		return *target;
	}
	NewScene();
	return GetCurrentEditorScene();
}

bool EditorComponent::CheckUnsavedChanges(int scene_index)
{
	if (scene_index < 0)
		scene_index = current_scene;

	if (scene_index < 0 || scene_index >= (int)scenes.size())
		return true;

	EditorScene& editorscene = *scenes[scene_index];
	if (!editorscene.has_unsaved_changes)
		return true;

	std::string sceneName = editorscene.path.empty() ? "" : Protean::Helper::GetFileNameFromPath(editorscene.path);
	std::string message = sceneName.empty() ? "Do you want to save the untitled scene?" : "Do you want to save changes to \"" + sceneName + "\"?";
	Protean::Helper::MessageBoxResult result = Protean::Helper::messageBoxCustom(message, "Unsaved changes", "YesNoCancel");

	if (result == Protean::Helper::MessageBoxResult_Yes)
	{
		// User wants to save
		if (editorscene.path.empty())
		{
			// Need to prompt for save location
			SaveAs();
			return true;
		}
		else
		{
			// Save to existing path
			Save(editorscene.path);
			return true;
		}
	}
	else if (result == Protean::Helper::MessageBoxResult_No)
	{
		// User doesn't want to save, proceed
		return true;
	}
	else if (result == Protean::Helper::MessageBoxResult_OK)
	{
		// IOS default OK return since we don't have messagebox yet
		return true;
	}
	else // Cancel or closed dialog
	{
		// User cancelled, don't proceed
		return false;
	}
}

void EditorComponent::FocusCameraOnSelected()
{
	Scene& scene = GetCurrentScene();
	EditorScene& editorscene = GetCurrentEditorScene();
	CameraComponent& camera = editorscene.camera;

	AABB aabb;
	XMVECTOR centerV = XMVectorSet(0, 0, 0, 0);
	float count = 0;
	for (auto& x : translator.selected)
	{
		TransformComponent* transform = scene.transforms.GetComponent(x.entity);
		if (transform == nullptr)
			continue;

		centerV = XMVectorAdd(centerV, transform->GetPositionV());
		count += 1.0f;

		if (scene.objects.Contains(x.entity))
		{
			aabb = AABB::Merge(aabb, scene.aabb_objects[scene.objects.GetIndex(x.entity)]);
		}
		if (scene.lights.Contains(x.entity))
		{
			size_t lightindex = scene.lights.GetIndex(x.entity);
			const LightComponent& light = scene.lights[lightindex];
			if (light.GetType() == LightComponent::DIRECTIONAL)
			{
				// Directional light AABB is huge, so we handle this as special case:
				AABB lightAABB;
				lightAABB.createFromHalfWidth(light.position, XMFLOAT3(1, 1, 1));
				aabb = AABB::Merge(aabb, lightAABB);
			}
			else
			{
				aabb = AABB::Merge(aabb, scene.aabb_lights[lightindex]);
			}
		}
		if (scene.decals.Contains(x.entity))
		{
			aabb = AABB::Merge(aabb, scene.aabb_decals[scene.decals.GetIndex(x.entity)]);
		}
		if (scene.probes.Contains(x.entity))
		{
			aabb = AABB::Merge(aabb, scene.aabb_probes[scene.probes.GetIndex(x.entity)]);
		}
	}
	if (count > 0)
	{
		centerV /= count;
	}
	else
		return;

	float focus_offset = 5;
	if (aabb.getArea() > 0)
	{
		focus_offset = aabb.getRadius() * 2;
		XMFLOAT3 aabb_center = aabb.getCenter();
		centerV = XMLoadFloat3(&aabb_center);
	}

	XMVECTOR target_forward = XMVector3Normalize(camera.GetAt());
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, target_forward));
	up = XMVector3Cross(target_forward, right);
	XMMATRIX rot_matrix = XMMATRIX(right, up, target_forward, XMVectorSet(0, 0, 0, 1));
	XMVECTOR target_quat = XMQuaternionRotationMatrix(rot_matrix);

	editorscene.camera_target = {};
	XMStoreFloat3(&editorscene.camera_target.translation_local, centerV);
	XMStoreFloat4(&editorscene.camera_target.rotation_local, target_quat);
	editorscene.camera_target.UpdateTransform();

	editorscene.camera_transform = {};
	editorscene.camera_transform.translation_local = XMFLOAT3(0, 0, -focus_offset);
	editorscene.camera_transform.UpdateTransform_Parented(editorscene.camera_target);
	editorscene.cam_move = {};
}

XMFLOAT3 EditorComponent::GetPositionInFrontOfCamera() const
{
	const EditorScene& editorscene = GetCurrentEditorScene();
	const CameraComponent& camera = editorscene.camera;
	XMFLOAT3 in_front_of_camera;
	XMStoreFloat3(&in_front_of_camera, XMVectorAdd(camera.GetEye(), camera.GetAt() * 4));
	if (is_2D_mode)
	{
		in_front_of_camera.z = 0;
	}
	return in_front_of_camera;
}

void EditorComponent::ReloadTerrainProps()
{
	Scene& scene = GetCurrentScene();
	bool any_reloaded = false;

	for (size_t i = 0; i < scene.terrains.GetCount(); ++i)
	{
		Protean::Terrain::Terrain& terrain = scene.terrains[i];
		for (Protean::Terrain::Prop& prop : terrain.props)
		{
			if (prop.source_entity == Protean::INVALID_ENTITY)
			{
				prop.data.clear();
				continue;
			}

			// Re-serialize the prop data
			Protean::Archive archive;
			Protean::EntitySerializer seri;
			scene.Entity_Serialize(
				archive,
				seri,
				prop.source_entity,
				Protean::Scene::EntitySerializeFlags_RECURSIVE | Protean::Scene::EntitySerializeFlags_KEEP_INTERNAL_ENTITY_REFERENCES
			);
			archive.WriteData(prop.data);
			any_reloaded = true;
		}

		if (any_reloaded)
		{
			terrain.InvalidateProps();
		}
	}

	if (any_reloaded)
	{
		Protean::Backlog::post("Terrain props reloaded", Protean::Backlog::LogLevel_Default);
	}
}

void EditorComponent::SetDefaultLocalization()
{
	GetGUI().ExportLocalization(default_localization);

	for (size_t i = 0; i < arraysize(EditorLocalizationStrings); ++i)
	{
		default_localization.Add(i, EditorLocalizationStrings[i]);
	}

	current_localization = default_localization;
}
void EditorComponent::SetLocalization(Protean::Localization& loc)
{
	current_localization = loc;
	GetGUI().ImportLocalization(current_localization);
	RefreshSceneList();
}
void EditorComponent::ReloadLanguage()
{
	generalWnd.languageCombo.SetSelected(generalWnd.languageCombo.GetSelected());
}

void EditorComponent::Set2DMode(bool value)
{
	is_2D_mode = value;

	EditorScene& editorscene = GetCurrentEditorScene();
	if (is_2D_mode)
	{
		editorscene.camera_transform.translation_local.z = -10;
		editorscene.camera_transform.rotation_local = XMFLOAT4(0, 0, 0, 1);
		editorscene.camera_transform.UpdateTransform();
		editorscene.camera.SetOrtho(true);
		editorscene.camera.ortho_vertical_size = editorscene.camera.ComputeOrthoVerticalSizeFromPerspective(editorscene.camera_transform.translation_local.z); // camera distance from origin
	}
	else
	{
		editorscene.camera.SetOrtho(false);
		cameraWnd.orthoCheckBox.SetCheck(false);
	}

	main->config.Set("2D_mode", is_2D_mode);
}
