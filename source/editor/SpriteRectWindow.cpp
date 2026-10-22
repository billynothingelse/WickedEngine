#include "stdafx.h"
#include "Editor.h"
#include "SpriteRectWindow.h"

void SpriteRectWindow::Create(EditorComponent* editor)
{
	Protean::Gui::Window::Create("Select an area within the atlas...");
	SetLocalizationEnabled(false);

	SetSize(XMFLOAT2(300, 300));

	spriteButton.Create("");
	spriteButton.OnDragStart([=](Protean::Gui::EventArgs args) {
		XMFLOAT2 pos = spriteButton.GetPos();
		XMFLOAT2 size = spriteButton.GetSize();
		dragStartUV = Protean::Math::InverseLerp(pos, XMFLOAT2(pos.x + size.x, pos.y + size.y), args.clickPos);
		dragStartUV.x = saturate(dragStartUV.x);
		dragStartUV.y = saturate(dragStartUV.y);
		dragEndUV = dragStartUV;
	});
	spriteButton.OnDrag([=](Protean::Gui::EventArgs args) {
		XMFLOAT2 pos = spriteButton.GetPos();
		XMFLOAT2 size = spriteButton.GetSize();
		dragEndUV = Protean::Math::InverseLerp(pos, XMFLOAT2(pos.x + size.x, pos.y + size.y), args.clickPos);
		dragEndUV.x = saturate(dragEndUV.x);
		dragEndUV.y = saturate(dragEndUV.y);
	});
	spriteButton.OnDragEnd([=](Protean::Gui::EventArgs args) {
		XMFLOAT2 pos = spriteButton.GetPos();
		XMFLOAT2 size = spriteButton.GetSize();
		dragEndUV = Protean::Math::InverseLerp(pos, XMFLOAT2(pos.x + size.x, pos.y + size.y), args.clickPos);
		dragEndUV.x = saturate(dragEndUV.x);
		dragEndUV.y = saturate(dragEndUV.y);

		muladd = XMFLOAT4(
			saturate(std::abs(dragEndUV.x - dragStartUV.x)),
			saturate(std::abs(dragEndUV.y - dragStartUV.y)),
			saturate(std::min(dragStartUV.x, dragEndUV.x)),
			saturate(std::min(dragStartUV.y, dragEndUV.y))
		);
	});
	AddWidget(&spriteButton);

	okButton.Create("OK");
	okButton.OnClick([=](Protean::Gui::EventArgs args) {

		if (onAccepted != nullptr)
			onAccepted();

		SetVisible(false);
	});
	AddWidget(&okButton);

	SetPos(XMFLOAT2(200, 100));
}

void SpriteRectWindow::ResizeLayout()
{
	Protean::Gui::Window::ResizeLayout();

	XMFLOAT2 size = GetWidgetAreaSize();

	spriteButton.SetSize(XMFLOAT2(size.x, size.y - okButton.GetSize().y * 2 - 1));
	spriteButton.SetPos(XMFLOAT2(0, 0));

	okButton.SetSize(XMFLOAT2(size.x, okButton.GetSize().y));
	okButton.SetPos(XMFLOAT2(0, spriteButton.GetSize().y));
}
void SpriteRectWindow::Update(const Protean::Canvas& canvas, float dt)
{
	Protean::Gui::Window::Update(canvas, dt);

	if (!IsVisible())
		return;

	if (GetPointerHitbox().intersects(spriteButton.hitBox))
	{
		Protean::Input::SetCursor(Protean::Input::CURSOR_CROSS);
	}
}
void SpriteRectWindow::Render(const Protean::Canvas& canvas, Protean::Graphics::CommandList cmd) const
{
	Protean::Gui::Window::Render(canvas, cmd);

	if (!IsVisible())
		return;

	ApplyScissor(canvas, this->scissorRect, cmd);

	XMFLOAT2 pos = spriteButton.GetPos();
	XMFLOAT2 size = spriteButton.GetSize();
	XMFLOAT2 end = XMFLOAT2(pos.x + size.x, pos.y + size.y);

	Protean::Image::Params params;
	params.sampleFlag = Protean::Image::SAMPLEMODE_WRAP;
	params.quality = Protean::Image::QUALITY_NEAREST;
	params.pos = XMFLOAT3(pos.x, pos.y, 0);
	params.siz = size;
	params.color = XMFLOAT4(1.7f, 1.7f, 1.7f, 1);
	params.enableDrawRect(XMFLOAT4(0, 0, size.x / 8, size.y / 8));
	Protean::Image::Draw(Protean::TextureHelper::getCheckerBoard(), params, cmd);

	params.disableDrawRect();
	params.quality = Protean::Image::QUALITY_LINEAR;
	params.color = XMFLOAT4(1, 1, 1, 1);
	Protean::Image::Draw(sprite.getTexture(), params, cmd);

	XMFLOAT2 rectstart = Protean::Math::Lerp(pos, end, dragStartUV);
	XMFLOAT2 rectend = Protean::Math::Lerp(pos, end, dragEndUV);

	params.pos = XMFLOAT3(rectstart.x, rectstart.y, 0);
	params.siz = XMFLOAT2(rectend.x - rectstart.x, rectend.y - rectstart.y);
	params.color = shadow_color;
	params.color.w = 0.5f;
	Protean::Image::Draw(nullptr, params, cmd);
}

void SpriteRectWindow::SetSprite(const Protean::Sprite& sprite)
{
	this->sprite = sprite;
}
void SpriteRectWindow::ResetSelection()
{
	dragStartUV = XMFLOAT2(0, 0);
	dragEndUV = XMFLOAT2(0, 0);
	muladd = XMFLOAT4(1, 1, 0, 0);
}
void SpriteRectWindow::OnAccepted(std::function<void()> cb)
{
	onAccepted = cb;
}
