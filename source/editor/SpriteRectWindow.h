#pragma once
class EditorComponent;

class SpriteRectWindow : public Protean::Gui::Window
{
	std::function<void()> onAccepted;
public:
	void Create(EditorComponent* editor);

	Protean::Gui::Button spriteButton;
	Protean::Gui::Button okButton;

	Protean::Sprite sprite;
	XMFLOAT2 dragStartUV = XMFLOAT2(0, 0);
	XMFLOAT2 dragEndUV = XMFLOAT2(0, 0);
	XMFLOAT4 muladd = XMFLOAT4(1, 1, 0, 0);

	void SetSprite(const Protean::Sprite& sprite);
	void ResetSelection();
	void OnAccepted(std::function<void()> cb);

	void ResizeLayout() override;
	void Update(const Protean::Canvas& canvas, float dt) override;
	void Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const override;
};
