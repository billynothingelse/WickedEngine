#pragma once
class EditorComponent;

class SplineWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity = Protean::INVALID_ENTITY;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Label infoLabel;
	Protean::Gui::CheckBox loopedCheck;
	Protean::Gui::CheckBox filledCheck;
	Protean::Gui::CheckBox alignedCheck;
	Protean::Gui::Slider widthSlider;
	Protean::Gui::Slider rotSlider;
	Protean::Gui::Slider subdivSlider;
	Protean::Gui::Slider subdivVerticalSlider;
	Protean::Gui::Slider terrainSlider;
	Protean::Gui::Slider terrainTexSlider;
	Protean::Gui::Slider terrainPushdownSlider;
	Protean::Gui::Button recenterButton;
	Protean::Gui::ComboBox fillNormalsComboBox;
	Protean::Gui::Button addButton;

	struct Entry
	{
		Protean::Gui::Button removeButton;
		Protean::Gui::Button entityButton;
	};
	std::deque<Entry> entries;

	void RefreshEntries();

	void NewNode();

	void ResizeLayout() override;
};

