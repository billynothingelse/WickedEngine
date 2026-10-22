#pragma once
class EditorComponent;

class PaintToolWindow : public Protean::Gui::Window
{
	float rot = 0;
	float stroke_dist = 0;
	size_t history_textureIndex = 0;
	struct TextureSlot
	{
		Protean::Graphics::Texture texture;
		int srgb_subresource = -1;
	};
	Protean::vector<TextureSlot> history_textures;
	TextureSlot GetEditTextureSlot(const Protean::MaterialComponent& material, int* uvset = nullptr);
	void ReplaceEditTextureSlot(Protean::MaterialComponent& material, const TextureSlot& textureslot);

	Protean::unordered_map<Protean::Entity, Protean::Archive> historyStartDatas;

	struct SculptingIndex
	{
		size_t ind;
		float affection;
	};
	Protean::vector<SculptingIndex> sculpting_indices;
	XMFLOAT3 sculpting_normal = XMFLOAT3(0, 0, 0);

	Protean::Resource brushTex;
	Protean::Resource revealTex;

	struct Stroke
	{
		XMFLOAT2 position;
		float pressure;
	};
	std::deque<Stroke> strokes;

public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	Protean::Gui::ComboBox modeComboBox;
	Protean::Gui::Label infoLabel;
	Protean::Gui::Slider radiusSlider;
	Protean::Gui::Slider amountSlider;
	Protean::Gui::Slider smoothnessSlider;
	Protean::Gui::Slider spacingSlider;
	Protean::Gui::Slider rotationSlider;
	Protean::Gui::Slider stabilizerSlider;
	Protean::Gui::CheckBox backfaceCheckBox;
	Protean::Gui::CheckBox wireCheckBox;
	Protean::Gui::CheckBox pressureCheckBox;
	Protean::Gui::CheckBox alphaCheckBox;
	Protean::Gui::CheckBox terrainCheckBox;
	Protean::Gui::ColorPicker colorPicker;
	Protean::Gui::ComboBox textureSlotComboBox;
	Protean::Gui::ComboBox brushShapeComboBox;
	Protean::Gui::Button saveTextureButton;
	Protean::Gui::Button brushTextureButton;
	Protean::Gui::Button revealTextureButton;
	Protean::Gui::ComboBox axisCombo;

	void UpdateData(float dt);
	void DrawBrush(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const;

	XMFLOAT2 pos = XMFLOAT2(0, 0);
	Protean::PickResult brushIntersect;

	enum MODE
	{
		MODE_DISABLED,
		MODE_TEXTURE,
		MODE_VERTEXCOLOR,
		MODE_SCULPTING_ADD,
		MODE_SCULPTING_SUBTRACT,
		MODE_SOFTBODY_PINNING,
		MODE_SOFTBODY_PHYSICS,
		MODE_HAIRPARTICLE_ADD_TRIANGLE,
		MODE_HAIRPARTICLE_REMOVE_TRIANGLE,
		MODE_HAIRPARTICLE_LENGTH,
		MODE_TERRAIN_MATERIAL,
		MODE_WIND,
	};
	MODE GetMode() const;

	enum class AxisLock
	{
		Disabled,
		X,
		Y,
		Z
	};

	std::deque<Protean::Gui::Button> terrain_material_buttons;
	size_t terrain_material_layer = 0;

	float texture_paint_radius = 50;
	float vertex_paint_radius = 0.1f;
	float terrain_paint_radius = 5;

	Protean::Archive* currentHistory = nullptr;
	void WriteHistoryData(Protean::Entity entity, Protean::Archive& archive, Protean::Graphics::CommandList cmd = Protean::Graphics::CommandList());
	void RecordHistory(Protean::Entity entity, Protean::Graphics::CommandList cmd = Protean::Graphics::CommandList());
	void ConsumeHistoryOperation(Protean::Archive& archive, bool undo);

	void ResizeLayout() override;

	void RecreateTerrainMaterialButtons();
};
