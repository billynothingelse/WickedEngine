#pragma once
#include "translator.h"
#include "script/bindings/scene_bind_lua.h"
#include "components_window.h"
#include "profiler_window.h"
#include "content_browser_window.h"
#include "project_creator_window.h"
#include "theme_editor_window.h"
#include "graphics_window.h"
#include "camera_window.h"
#include "material_picker_window.h"
#include "paint_tool_window.h"
#include "general_window.h"
#include "icon_definitions.h"
#include "game_play.h"

class Editor;
class EditorComponent : public Protean::RenderPath2D
{
public:
	Editor* main = nullptr;

	Protean::Gui::Button newSceneButton;

	Protean::Gui::ComboBox newEntityCombo;

	Protean::Gui::Button translateButton;
	Protean::Gui::Button rotateButton;
	Protean::Gui::Button scaleButton;
	Protean::Gui::Button localGlobalButton;

	Protean::Gui::Button physicsButton;

	Protean::Gui::Button dummyButton;
	bool dummy_enabled = false;
	bool dummy_male = false;
	XMFLOAT3 dummy_pos = XMFLOAT3(0, 0, 0);

	bool drive_mode = false;
	float drive_cam_dist_next = 7;
	float drive_cam_dist = drive_cam_dist_next;
	float drive_orbit_horizontal = 0;
	float drive_steering_smoothed = 0;

	Protean::Gui::Button navtestButton;
	bool navtest_enabled = false;
	Protean::PickResult navtest_start_pick;
	Protean::PickResult navtest_goal_pick;
	Protean::PathQuery navtest_pathquery;

	// Game module: Play / Pause / Stop
	Protean::Gui::Button gamePlayButton;
	Protean::Gui::Button gamePauseButton;
	Protean::Gui::Button gameStopButton;
	EditorGamePlay gamePlay;
	void PlayGame();
	void PauseGame();
	void StopGame();

	// Lua scripts: run last script / kill running scripts
	Protean::Gui::Button playButton;
	Protean::Gui::Button stopButton;
	Protean::Gui::Button projectCreatorButton;

	Protean::Gui::Button saveButton;
	Protean::Gui::Button openButton;
	Protean::Gui::Button contentBrowserButton;
	Protean::Gui::Button logButton;
	Protean::Gui::Button profilerButton;
	Protean::Gui::Button cinemaButton;
	Protean::Gui::Button fullscreenButton;
	Protean::Gui::Button bugButton;
	Protean::Gui::Button aboutButton;
	Protean::Gui::Button exitButton;
	Protean::Gui::Window aboutWindow;
	Protean::Gui::Label aboutLabel;

	ComponentsWindow componentsWnd;
	ProfilerWindow profilerWnd;
	ContentBrowserWindow contentBrowserWnd;
	ProjectCreatorWindow projectCreatorWnd;
	ThemeEditorWindow themeEditorWnd;
	Protean::Gui::Window topmenuWnd;

	Protean::Gui::Button generalButton;
	Protean::Gui::Button graphicsButton;
	Protean::Gui::Button cameraButton;
	Protean::Gui::Button materialsButton;
	Protean::Gui::Button paintToolButton;

	Protean::Gui::ComboBox guiScalingCombo;

	bool is_2D_mode = false;
	Protean::Gui::Button is2DModeButton;

	GeneralWindow generalWnd;
	GraphicsWindow graphicsWnd;
	CameraWindow cameraWnd;
	MaterialPickerWindow materialPickerWnd;
	PaintToolWindow paintToolWnd;

	Protean::Ray pickRay;
	Protean::Physics::PickDragOperation physicsDragOp;

	std::unique_ptr<Protean::RenderPath3D> renderPath;
	Protean::RenderPath3D_PathTracing* pathtracer = nullptr; // This is not lifetime managing pointer, it will view renderPath if it's path tracing
	Protean::Graphics::Texture gui_background_effect;
	const Protean::Graphics::Texture* GetGUIBlurredBackground() const override { return renderPath->GetGUIBlurredBackground(); }

	void ResizeBuffers() override;
	void ResizeLayout() override;
	void Load() override;
	void Start() override;
	void PreUpdate() override;
	void FixedUpdate() override;
	void Update(float dt) override;
	void PostUpdate() override;
	void PreRender() override;
	void Render() const override;
	void Compose(Protean::Graphics::CommandList cmd) const override;

	Protean::Graphics::Viewport viewport3D;
	Protean::Hitbox2D viewport3D_hitbox;
	void ResizeViewport3D();

	bool camControlStart = true;
	XMFLOAT4 originalMouse = XMFLOAT4(0, 0, 0, 0);
	XMFLOAT4 currentMouse = XMFLOAT4(0, 0, 0, 0);

	bool cinema_mode_saved_debugEnvProbes = false;
	bool cinema_mode_saved_debugCameras = false;
	bool cinema_mode_saved_debugColliders = false;
	bool cinema_mode_saved_debugEmitters = false;
	bool cinema_mode_saved_debugForceFields = false;
	bool cinema_mode_saved_debugBoneLines = false;
	bool cinema_mode_saved_debugPartitionTree = false;
	bool cinema_mode_saved_debugSprings = false;
	bool cinema_mode_saved_gridHelper = false;

	enum EDITORSTENCILREF
	{
		EDITORSTENCILREF_CLEAR = 0x00,
		EDITORSTENCILREF_HIGHLIGHT_OBJECT = 0x01,
		EDITORSTENCILREF_HIGHLIGHT_MATERIAL = 0x02,
		EDITORSTENCILREF_LAST = 0x0F,
	};
	Protean::Graphics::Texture rt_selectionOutline_MSAA;
	Protean::Graphics::Texture rt_selectionOutline[2];
	Protean::Graphics::Texture rt_dummyOutline;
	float outlineTimer = 0;
	const XMFLOAT4 selectionColor = XMFLOAT4(1, 0.6f, 0, 1);
	const XMFLOAT4 selectionColor2 = XMFLOAT4(0, 1, 0.6f, 0.35f);
	Protean::Color inactiveEntityColor = Protean::Color::fromFloat4(XMFLOAT4(1, 1, 1, 0.5f));
	Protean::Color hoveredEntityColor = Protean::Color::fromFloat4(XMFLOAT4(1, 1, 1, 1));
	Protean::Color backgroundEntityColor = Protean::Color::Black();
	Protean::Color dummyColor = Protean::Color::White();

	Protean::Graphics::Texture rt_metadataDummies_MSAA;
	Protean::Graphics::Texture rt_metadataDummies;

	const Protean::Color springDebugColor = Protean::Color(255, 70, 165, 255);
	const Protean::Color ikDebugColor = Protean::Color(49, 190, 103, 255);

	Protean::Graphics::Texture editor_depthbuffer;
	Protean::Graphics::Texture editor_rendertarget;

	Translator translator;
	Protean::PickResult hovered;
	bool inspector_mode = false;
	Protean::Entity grass_interaction_entity = Protean::INVALID_ENTITY;

	void ClearSelected();
	void AddSelected(Protean::Entity entity, bool allow_refocus = true);
	void AddSelected(const Protean::PickResult& picked, bool allow_refocus = true);
	bool IsSelected(Protean::Entity entity) const;
	bool selectAll = false;
	Protean::unordered_set<Protean::Entity> selectAllStorage;

	bool bone_picking = false;
	void CheckBonePickingEnabled();
	Protean::unordered_map<Protean::Entity, Protean::Capsule> bone_picking_items;

	void UpdateDynamicWidgets();
	void UpdateLocalGlobalButton();

	Protean::Archive clipboard;

	enum HistoryOperationType
	{
		HISTORYOP_TRANSLATOR,		// translator interaction
		HISTORYOP_SELECTION,		// selection changed
		HISTORYOP_ADD,				// entity added
		HISTORYOP_DELETE,			// entity removed
		HISTORYOP_COMPONENT_DATA,	// generic component data changed
		HISTORYOP_PAINTTOOL,		// paint tool interaction
		HISTORYOP_ADD_TO_SPLINE,	// Spline node added
		HISTORYOP_NONE
	};

	void RecordSelection(Protean::Archive& archive) const;
	void RecordEntity(Protean::Archive& archive, Protean::Entity entity);
	void RecordEntity(Protean::Archive& archive, const Protean::vector<Protean::Entity>& entities);

	void ResetHistory();
	Protean::Archive& AdvanceHistory(bool scene_unchanged = false);
	void ConsumeHistoryOperation(bool undo);

	Protean::vector<std::string> recentFilenames;
	size_t maxRecentFilenames = 10;
	Protean::vector<std::string> recentFolders;
	size_t maxRecentFolders = 8;
	void RegisterRecentlyUsed(std::string filename);

	void Open(std::string filename);
	void Save(const std::string& filename);
	void SaveAs();
	bool deleting = false;
	bool save_in_progress = false;

	Protean::Graphics::Texture CreateThumbnail(Protean::Graphics::Texture texture, uint32_t target_width, uint32_t target_height, bool mipmaps = false) const;
	bool SetupThumbnailCamera(Protean::RenderPath3D& thumbnailRenderPath);
	Protean::CameraComponent thumbnail_saved_camera;

	std::string save_text_message = "";
	std::string save_text_filename = "";
	float save_text_alpha = 0; // seconds until save text disappears
	Protean::Color save_text_color = Protean::Color::White();
	void PostSaveText(const std::string& message, const std::string& filename = "", float time_seconds = 4);

	bool screenshot = false;
	bool screenshot_nogui = false;
	bool screenshot_alpha = false;
	bool screenshot_alpha_selection = false;

	std::string last_script_path;

	struct EditorScene
	{
		uint64_t id = 0; // unique ID for scene tabs
		std::string path;
		Protean::Scene scene;
		XMFLOAT3 cam_move = {};
		Protean::CameraComponent camera;
		Protean::TransformComponent camera_transform;
		Protean::TransformComponent camera_target;
		Protean::vector<Protean::Archive> history;
		int historyPos = -1;
		bool has_unsaved_changes = false;
		bool untitled_camera_reset_once = false;
		Protean::Gui::Button tabSelectButton;
		Protean::Gui::Button tabCloseButton;
	};
	Protean::vector<std::unique_ptr<EditorScene>> scenes;
	uint64_t next_scene_id = 1;
	int current_scene = 0;
	EditorScene& GetCurrentEditorScene() { return *scenes[current_scene].get(); }
	const EditorScene& GetCurrentEditorScene() const { return *scenes[current_scene].get(); }
	Protean::Scene& GetCurrentScene() { return scenes[current_scene].get()->scene; }
	const Protean::Scene& GetCurrentScene() const { return scenes[current_scene].get()->scene; }
	// Find EditorScene by ID, returns pointer or nullptr if not found (e.g., tab was closed)
	EditorScene* FindEditorSceneByID(uint64_t id) const;
	// Get or create an EditorScene for content loading: returns the scene with given ID, or creates a new one if not found
	EditorScene& GetOrCreateEditorSceneForLoading(uint64_t target_scene_id);
	void SetCurrentScene(int index);
	void RefreshSceneList();
	void NewScene();

	bool CheckUnsavedChanges(int scene_index = -1);

	void FocusCameraOnSelected();

	void ReloadTerrainProps();

	XMFLOAT3 GetPositionInFrontOfCamera() const;

	Protean::Localization default_localization;
	Protean::Localization current_localization;
	void SetDefaultLocalization();
	void SetLocalization(Protean::Localization& loc);
	void ReloadLanguage();

	void Set2DMode(bool value);

	struct FontData
	{
		std::string name;
		Protean::vector<uint8_t> filedata;
	};
	Protean::vector<FontData> font_datas;

	Protean::JobSystem::context loadmodel_workload;
	Protean::SpriteFont loadmodel_font;
	Protean::SpriteFont terrain_generation_font;
	float terrain_generation_font_timeout = 0;

	Protean::TrailRenderer spline_renderer;
};

class Editor : public Protean::Application
{
public:
	EditorComponent renderComponent;
	Protean::Config::File config;
	bool exit_requested = false;

	void Initialize() override;

	void HotReload();

	bool KeepRunning();

	void SaveWindowSize();

	void Exit() override;

	~Editor() override
	{
		if (!IsScriptReplacement()) // editor config is not saved by script replaced executable
		{
			config.Commit();
		}
	}
};

// Additional localizations that are outside the GUI can be defined here:
enum EditorLocalization {
	// Top menu:
	EditorLocalization_Save,
	EditorLocalization_Open,
	EditorLocalization_Backlog,
	EditorLocalization_Profiler,
	EditorLocalization___REMOVED_Cinema,
	EditorLocalization_FullScreen,
	EditorLocalization_Windowed,
	EditorLocalization_BugReport,
	EditorLocalization_About,
	EditorLocalization_Exit,
	EditorLocalization_UntitledScene,
	EditorLocalization_ContentBrowser,

	EditorLocalization_Count
};
static const char* EditorLocalizationStrings[] = {
	// Top menu:
	"Save",
	"Open",
	"Backlog",
	"Profiler",
	"Cinema",
	"Full screen",
	"Windowed",
	"Bug report",
	"About",
	"Exit",
	"Untitled scene",
	"Content",
};
static_assert(arraysize(EditorLocalizationStrings) == size_t(EditorLocalization_Count));

struct ApplicationExeCustomization
{
	char name_padded[120];
	Protean::Color font_color; // color of backlog font (startup text)
	Protean::Color background_color; // color of startup background behind startup text
};
extern ApplicationExeCustomization exe_customization;
