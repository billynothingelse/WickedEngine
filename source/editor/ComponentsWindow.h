#pragma once
#include "MaterialWindow.h"
#include "WeatherWindow.h"
#include "ObjectWindow.h"
#include "MeshWindow.h"
#include "EnvProbeWindow.h"
#include "DecalWindow.h"
#include "LightWindow.h"
#include "AnimationWindow.h"
#include "EmitterWindow.h"
#include "HairParticleWindow.h"
#include "ForceFieldWindow.h"
#include "SoundWindow.h"
#include "VideoWindow.h"
#include "SpringWindow.h"
#include "IKWindow.h"
#include "TransformWindow.h"
#include "LayerWindow.h"
#include "NameWindow.h"
#include "ScriptWindow.h"
#include "RigidBodyWindow.h"
#include "SoftBodyWindow.h"
#include "ColliderWindow.h"
#include "HierarchyWindow.h"
#include "CameraComponentWindow.h"
#include "ExpressionWindow.h"
#include "ArmatureWindow.h"
#include "HumanoidWindow.h"
#include "TerrainWindow.h"
#include "SpriteWindow.h"
#include "FontWindow.h"
#include "VoxelGridWindow.h"
#include "MetadataWindow.h"
#include "ConstraintWindow.h"
#include "SplineWindow.h"
#include "GaussianSplatWindow.h"

class EditorComponent;

class ComponentsWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);
	void UpdateData(float dt);

	void ResizeLayout() override;

	EditorComponent* editor = nullptr;
	Protean::Gui::ComboBox newComponentCombo;
	MaterialWindow materialWnd;
	WeatherWindow weatherWnd;
	ObjectWindow objectWnd;
	MeshWindow meshWnd;
	EnvProbeWindow envProbeWnd;
	DecalWindow decalWnd;
	LightWindow lightWnd;
	AnimationWindow animWnd;
	EmitterWindow emitterWnd;
	HairParticleWindow hairWnd;
	ForceFieldWindow forceFieldWnd;
	SoundWindow soundWnd;
	VideoWindow videoWnd;
	SpringWindow springWnd;
	IKWindow ikWnd;
	TransformWindow transformWnd;
	LayerWindow layerWnd;
	NameWindow nameWnd;
	ScriptWindow scriptWnd;
	RigidBodyWindow rigidWnd;
	SoftBodyWindow softWnd;
	ColliderWindow colliderWnd;
	HierarchyWindow hierarchyWnd;
	CameraComponentWindow cameraComponentWnd;
	ExpressionWindow expressionWnd;
	ArmatureWindow armatureWnd;
	HumanoidWindow humanoidWnd;
	TerrainWindow terrainWnd;
	SpriteWindow spriteWnd;
	FontWindow fontWnd;
	VoxelGridWindow voxelGridWnd;
	MetadataWindow metadataWnd;
	ConstraintWindow constraintWnd;
	SplineWindow splineWnd;
	GaussianSplatWindow gaussiansplatWnd;

	enum Filter : uint64_t
	{
		Filter_Transform = 1ull << 0ull,
		Filter_Material = 1ull << 1ull,
		Filter_Mesh = 1ull << 2ull,
		Filter_Object = 1ull << 3ull,
		Filter_EnvironmentProbe = 1ull << 4ull,
		Filter_Decal = 1ull << 5ull,
		Filter_Sound = 1ull << 6ull,
		Filter_Weather = 1ull << 7ull,
		Filter_Light = 1ull << 8ull,
		Filter_Animation = 1ull << 9ull,
		Filter_Force = 1ull << 10ull,
		Filter_Emitter = 1ull << 11ull,
		Filter_Hairparticle = 1ull << 12ull,
		Filter_IK = 1ull << 13ull,
		Filter_Camera = 1ull << 14ull,
		Filter_Armature = 1ull << 15ull,
		Filter_Collider = 1ull << 16ull,
		Filter_Script = 1ull << 17ull,
		Filter_Expression = 1ull << 18ull,
		Filter_Terrain = 1ull << 19ull,
		Filter_Spring = 1ull << 20ull,
		Filter_Humanoid = 1ull << 21ull,
		Filter_Video = 1ull << 22ull,
		Filter_Sprite = 1ull << 23ull,
		Filter_Font = 1ull << 24ull,
		Filter_VoxelGrid = 1ull << 25ull,
		Filter_RigidBody = 1ull << 26ull,
		Filter_SoftBody = 1ull << 27ull,
		Filter_Metadata = 1ull << 28ull,
		Filter_Vehicle = 1ull << 29ull,
		Filter_Constraint = 1ull << 30ull,
		Filter_Spline = 1ull << 31ull,
		Filter_GaussianSplat = 1ull << 32ull,

		Filter_All = ~0ull,
	} filter = Filter_All;
	Protean::Gui::ComboBox filterCombo;
	Protean::Gui::TextInputField filterInput;
	Protean::Gui::CheckBox filterCaseCheckBox;
	Protean::Gui::TreeList entityTree;
	Protean::unordered_set<Protean::Entity> entitytree_temp_items;
	Protean::unordered_set<Protean::Entity> entitytree_added_items;
	Protean::unordered_set<Protean::Entity> entitytree_opened_items;
	Protean::Entity entitytree_pending_focus = Protean::INVALID_ENTITY;
	Protean::unordered_map<Protean::Entity, Protean::vector<Protean::Entity>> entity_user_order; // custom display order per parent (INVALID_ENTITY = top-level)
	void PushToEntityTree(Protean::Entity entity, int level);
	void RefreshEntityTree();
	bool CheckEntityFilter(Protean::Entity entity) const;
	void ApplyUserOrder(Protean::vector<Protean::Entity>& entities, Protean::Entity parent_entity) const;

private:
	static int GetEntityTypePriority(Protean::Entity entity, const Protean::Scene& scene);
	static std::string GetEntityNameForSorting(Protean::Entity entity, const Protean::Scene& scene);
	void SortEntitiesByMode(Protean::vector<Protean::Entity>& entities, const Protean::Scene& scene, int sortingMode);
};

template<>
struct enable_bitmask_operators<ComponentsWindow::Filter> : std::true_type {};
