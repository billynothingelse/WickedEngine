#pragma once

class Translator
{
private:
	bool dragging = false;
	bool dragStarted = false;
	bool dragEnded = false;
	XMFLOAT3 intersection_start = XMFLOAT3(0, 0, 0);
	XMFLOAT3 axis = XMFLOAT3(1, 0, 0);
	XMFLOAT4 drag_plane = XMFLOAT4(0, 0, 0, 0);
	float angle = 0;
	float angle_start = 0;
	bool has_selected_transform = false;
	Protean::vector<uint64_t> temp_filters;
	Protean::vector<Protean::Entity> selectedWithHierarchy; // all the selected entities and their descendants
public:

	void Update(const Protean::CameraComponent& camera, const XMFLOAT4& currentMouse, const Protean::Canvas& canvas);
	void Draw(const Protean::CameraComponent& camera, const XMFLOAT4& currentMouse, Protean::Graphics::CommandList cmd) const;

	// Attach selection to translator temporarily
	void PreTranslate();
	// Apply translator to selection
	void PostTranslate() const;

	Protean::Scene* scene = nullptr;
	Protean::TransformComponent transform;
	Protean::vector<Protean::PickResult> selected; // all the selected picks
	Protean::unordered_set<Protean::Entity> selectedEntitiesLookup; // fast lookup for selected entities
	Protean::vector<Protean::Entity> selectedEntitiesNonRecursive; // selected entities that don't contain entities that would be included in recursive iterations

	float scale_snap = 1;
	float rotate_snap = XM_PIDIV4;
	float translate_snap = 1;
	float tool_opacity = 1;
	float tool_darken_negative_axes = 1;
	float tool_scale = 1.0f;
	float tool_thickness = 1.0f;

	enum TRANSLATOR_STATE
	{
		TRANSLATOR_IDLE,
		TRANSLATOR_X,
		TRANSLATOR_Y,
		TRANSLATOR_Z,
		TRANSLATOR_XY,
		TRANSLATOR_XZ,
		TRANSLATOR_YZ,
		TRANSLATOR_XYZ,
	} state = TRANSLATOR_IDLE;

	XMMATRIX GetMirrorMatrix(TRANSLATOR_STATE state, const Protean::CameraComponent& camera) const;
	void WriteAxisText(TRANSLATOR_STATE axis, const Protean::CameraComponent& camera, char* text) const;

	float dist = 1;

	bool isTranslator = true;
	bool isScalator = false;
	bool isRotator = false;
	bool isLocalSpace = false;
	bool is2D = false;
	bool IsEnabled() const { return isTranslator || isRotator || isScalator; }
	XMMATRIX GetLocalRotation() const;
	void SetEnabled(bool value)
	{
		if (value && !IsEnabled())
		{
			isTranslator = true;
		}
		else if (!value && IsEnabled())
		{
			isTranslator = false;
			isScalator = false;
			isRotator = false;
		}
	}


	// Check if the drag started in this exact frame
	bool IsDragStarted() const { return dragStarted; };
	// Check if the drag ended in this exact frame
	bool IsDragEnded() const { return dragEnded; };

	bool IsInteracting() const { return state != TRANSLATOR_IDLE; }

	Protean::TransformComponent transform_start;
	Protean::vector<XMFLOAT4X4> matrices_start;
	Protean::vector<XMFLOAT4X4> matrices_current;
};

