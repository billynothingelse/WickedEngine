#pragma once
class EditorComponent;

class TransformWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	Protean::Gui::Button clearButton;
	Protean::Gui::Button moveToEditorCameraButton;
	Protean::Gui::Button applyScaleToObjectButton;

	Protean::Gui::TextInputField txInput;
	Protean::Gui::TextInputField tyInput;
	Protean::Gui::TextInputField tzInput;

	Protean::Gui::TextInputField rollInput;
	Protean::Gui::TextInputField pitchInput;
	Protean::Gui::TextInputField yawInput;

	Protean::Gui::TextInputField rxInput;
	Protean::Gui::TextInputField ryInput;
	Protean::Gui::TextInputField rzInput;
	Protean::Gui::TextInputField rwInput;

	Protean::Gui::TextInputField sxInput;
	Protean::Gui::TextInputField syInput;
	Protean::Gui::TextInputField szInput;
	Protean::Gui::TextInputField suInput;

	Protean::Gui::TextInputField snapScaleInput;
	Protean::Gui::TextInputField snapRotateInput;
	Protean::Gui::TextInputField snapTranslateInput;

	Protean::Gui::Button resetTranslationButton;
	Protean::Gui::Button resetScaleButton;
	Protean::Gui::Button resetScaleUniformButton;
	Protean::Gui::Button resetRotationButton;

	void ResizeLayout() override;
};

