#pragma once
class EditorComponent;

struct ModifierWindow : public Protean::Gui::Window
{
	Protean::Terrain::Modifier* modifier = nullptr;
	std::function<void()> generation_callback;
	Protean::Gui::ComboBox blendCombo;
	Protean::Gui::Slider weightSlider;
	Protean::Gui::Slider scaleSlider;

	virtual ~ModifierWindow() = default;

	ModifierWindow(const std::string& name);
	void Bind(Protean::Terrain::Modifier* ptr);
	void From(Protean::Terrain::Modifier* ptr);
};
struct PerlinModifierWindow : public ModifierWindow
{
	Protean::Gui::Slider octavesSlider;

	PerlinModifierWindow();
	void ResizeLayout() override;
	void Bind(Protean::Terrain::PerlinModifier* ptr);
	void From(Protean::Terrain::PerlinModifier* ptr);
};
struct VoronoiModifierWindow : public ModifierWindow
{
	Protean::Gui::Slider fadeSlider;
	Protean::Gui::Slider shapeSlider;
	Protean::Gui::Slider falloffSlider;
	Protean::Gui::Slider perturbationSlider;

	VoronoiModifierWindow();
	void ResizeLayout() override;
	void Bind(Protean::Terrain::VoronoiModifier* ptr);
	void From(Protean::Terrain::VoronoiModifier* ptr);
};
struct HeightmapModifierWindow : public ModifierWindow
{
	Protean::Gui::Slider amountSlider;
	Protean::Gui::Button loadButton;

	HeightmapModifierWindow();
	void ResizeLayout() override;
	void Bind(Protean::Terrain::HeightmapModifier* ptr);
	void From(Protean::Terrain::HeightmapModifier* ptr);
};

struct PropWindow : public Protean::Gui::Window
{
	Protean::Gui::ComboBox meshCombo;
	Protean::Gui::TextInputField minCountPerChunkInput;
	Protean::Gui::TextInputField maxCountPerChunkInput;
	Protean::Gui::ComboBox regionCombo;
	Protean::Gui::Slider regionPowerSlider;
	Protean::Gui::Slider noiseFrequencySlider;
	Protean::Gui::Slider noisePowerSlider;
	Protean::Gui::Slider thresholdSlider;
	Protean::Gui::Slider minSizeSlider;
	Protean::Gui::Slider maxSizeSlider;
	Protean::Gui::TextInputField minYOffsetInput;
	Protean::Gui::TextInputField maxYOffsetInput;

	PropWindow(Protean::Terrain::Terrain* terrain, Protean::Terrain::Prop* prop, Protean::Scene* scene);
	void ResizeLayout() override;

	Protean::Terrain::Terrain* terrain = nullptr;
	Protean::Terrain::Prop* prop = nullptr;
	Protean::Scene* scene = nullptr;

	std::function<void()> generation_callback;
};

struct PropsWindow : public Protean::Gui::Window
{
	Protean::Gui::Button addButton;
	Protean::Gui::Button reloadButton;

	PropsWindow(EditorComponent* editor);

	void Rebuild();
	void AddWindow(Protean::Terrain::Prop& prop);

	void Update(const Protean::Canvas& canvas, float dt) override;

	void ResizeLayout() override;

	EditorComponent* editor = nullptr;
	Protean::Terrain::Terrain* terrain = nullptr;

	Protean::vector<std::unique_ptr<PropWindow>> windows;
	Protean::vector<PropWindow*> windows_to_remove;

	std::function<void()> generation_callback;
};

class TerrainWindow : public Protean::Gui::Window
{
public:
	Protean::Gui::CheckBox centerToCamCheckBox;
	Protean::Gui::CheckBox removalCheckBox;
	Protean::Gui::CheckBox grassCheckBox;
	Protean::Gui::CheckBox physicsCheckBox;
	Protean::Gui::CheckBox tessellationCheckBox;
	Protean::Gui::Slider lodSlider;
	Protean::Gui::Slider generationSlider;
	Protean::Gui::Slider propGenerationSlider;
	Protean::Gui::Slider physicsGenerationSlider;
	Protean::Gui::Slider propDensitySlider;
	Protean::Gui::Slider grassDensitySlider;
	Protean::Gui::Slider grassLengthSlider;
	Protean::Gui::Slider grassDistanceSlider;
	Protean::Gui::ComboBox presetCombo;
	Protean::Gui::Slider scaleSlider;
	Protean::Gui::Slider seedSlider;
	Protean::Gui::Slider bottomLevelSlider;
	Protean::Gui::Slider topLevelSlider;
	Protean::Gui::Button saveHeightmapButton;
	Protean::Gui::Button saveRegionButton;
	Protean::Gui::ComboBox addModifierCombo;

	Protean::Gui::Slider region1Slider;
	Protean::Gui::Slider region2Slider;
	Protean::Gui::Slider region3Slider;

	Protean::Gui::ComboBox materialCombos[Protean::Terrain::MATERIAL_COUNT];
	Protean::Gui::ComboBox materialCombo_GrassParticle;

	std::unique_ptr<PropsWindow> propsWindow;

	enum PRESET
	{
		PRESET_HILLS,
		PRESET_ISLANDS,
		PRESET_MOUNTAINS,
		PRESET_ARCTIC,
		PRESET_DESERT,
	};

	Protean::vector<std::unique_ptr<ModifierWindow>> modifiers;
	Protean::vector<ModifierWindow*> modifiers_to_remove;

	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	Protean::Terrain::Terrain* terrain = nullptr;
	void SetEntity(Protean::Entity entity);
	void AddModifier(ModifierWindow* modifier_window);
	void SetupAssets();

	void RefreshMaterialComboBoxes();

	void Update(const Protean::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};

