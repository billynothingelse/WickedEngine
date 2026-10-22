#pragma once
class EditorComponent;

class MetadataWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::ComboBox presetCombo;
	Protean::Gui::ComboBox addCombo;

	struct Entry
	{
		Protean::Gui::Button remove;
		Protean::Gui::TextInputField name;
		Protean::Gui::TextInputField value;
		Protean::Gui::CheckBox check;
		bool is_bool = false;
	};
	std::deque<Entry> entries;

	void RefreshEntries();

	void ResizeLayout() override;
};

