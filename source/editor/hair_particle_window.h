#pragma once
#include "sprite_rect_window.h"

class EditorComponent;

class MaterialWindow;

class HairParticleWindow : public Protean::Gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	Protean::Entity entity;
	void SetEntity(Protean::Entity entity);

	void UpdateData();

	Protean::HairParticleSystem* GetHair();

	Protean::Gui::Label infoLabel;
	Protean::Gui::ComboBox meshComboBox;
	Protean::Gui::CheckBox cameraBendCheckbox;
	Protean::Gui::Slider lengthSlider;
	Protean::Gui::Slider widthSlider;
	Protean::Gui::Slider stiffnessSlider;
	Protean::Gui::Slider dragSlider;
	Protean::Gui::Slider gravityPowerSlider;
	Protean::Gui::Slider randomnessSlider;
	Protean::Gui::Slider countSlider;
	Protean::Gui::Slider segmentcountSlider;
	Protean::Gui::Slider billboardcountSlider;
	Protean::Gui::Slider randomSeedSlider;
	Protean::Gui::Slider viewDistanceSlider;
	Protean::Gui::Slider uniformitySlider;

	Protean::Gui::Button addSpriteButton;
	std::deque<Protean::Gui::Button> sprites;
	std::deque<Protean::Gui::Button> spriteRemoveButtons;
	std::deque<Protean::Gui::Slider> spriteSizeSliders;

	SpriteRectWindow spriterectwnd;

	void RefreshSprites();

	void ResizeLayout() override;
};

