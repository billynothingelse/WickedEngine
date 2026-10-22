#include "render/SpriteFont.h"
#include "core/Helper.h"

using namespace Protean::Graphics;

namespace Protean
{

	void SpriteFont::FixedUpdate()
	{
		if (IsDisableUpdate())
			return;
	}
	void SpriteFont::Update(float dt)
	{
		if (IsDisableUpdate())
			return;

		if (fontStyleResource.IsValid())
		{
			params.style = fontStyleResource.GetFontStyle();
		}
		else
		{
			params.style = 0;
		}

		if (anim.typewriter.time > 0)
		{
			size_t text_length = text.length();
			size_t text_length_prev = std::min(text_length, size_t(Protean::Math::Lerp(float(std::min(text_length, anim.typewriter.character_start)), float(text_length + 1), anim.typewriter.elapsed / anim.typewriter.time)));
			anim.typewriter.elapsed += dt;
			size_t text_length_next = std::min(text_length, size_t(Protean::Math::Lerp(float(std::min(text_length, anim.typewriter.character_start)), float(text_length + 1), anim.typewriter.elapsed / anim.typewriter.time)));

			if (anim.typewriter.soundinstance.IsValid())
			{
				if (!anim.typewriter.IsFinished() && Protean::Audio::IsEnded(&anim.typewriter.soundinstance) && text_length_prev != text_length_next && text[text_length_next - 1] != ' ' && anim.typewriter.soundinstance.IsValid())
				{
					Protean::Audio::Stop(&anim.typewriter.soundinstance);
					if (!IsHidden())
					{
						Protean::Audio::Play(&anim.typewriter.soundinstance);
					}
				}
				Protean::Audio::ExitLoop(&anim.typewriter.soundinstance);
			}

			if (anim.typewriter.looped && anim.typewriter.elapsed > anim.typewriter.time)
			{
				anim.typewriter.reset();
			}
		}
	}

	void SpriteFont::Draw(CommandList cmd) const
	{
		if (IsHidden())
			return;
		if (params.color.getA() == 0)
			return;

		Protean::Font::Draw(text.c_str(), GetCurrentTextLength(), params, cmd);
	}

	XMFLOAT2 SpriteFont::TextSize() const
	{
		return Protean::Font::TextSize(text, params);
	}
	float SpriteFont::TextWidth() const
	{
		return Protean::Font::TextWidth(text, params);
	}
	float SpriteFont::TextHeight() const
	{
		return Protean::Font::TextHeight(text, params);
	}

	void SpriteFont::SetText(const std::string& value)
	{
		Protean::Helper::StringConvert(value, text);
	}
	void SpriteFont::SetText(std::string&& value)
	{
		Protean::Helper::StringConvert(value, text);
	}
	void SpriteFont::SetText(const std::wstring& value)
	{
		text = value;
	}
	void SpriteFont::SetText(std::wstring&& value)
	{
		text = std::move(value);
	}

	std::string SpriteFont::GetTextA() const
	{
		std::string retVal;
		Protean::Helper::StringConvert(text, retVal);
		return retVal;
	}
	const std::wstring& SpriteFont::GetText() const
	{
		return text;
	}

	size_t SpriteFont::GetCurrentTextLength() const
	{
		size_t text_length = text.length();
		if (anim.typewriter.time > 0)
		{
			text_length = std::min(text_length, size_t(Protean::Math::Lerp(float(std::min(text_length, anim.typewriter.character_start)), float(text_length + 1), anim.typewriter.elapsed / anim.typewriter.time)));
		}
		return text_length;
	}

	Protean::AABB SpriteFont::GetAABB() const
	{
		XMFLOAT2 size = TextSize();
		size.x *= params.scaling;
		size.y *= params.scaling;
		size.x *= 0.5f;
		size.y *= 0.5f;

		XMFLOAT3 center = params.position;
		if (params.h_align == Protean::Font::WIFALIGN_LEFT)
		{
			center.x += size.x * 0.5f;
		}
		else if (params.h_align == Protean::Font::WIFALIGN_RIGHT)
		{
			center.x -= size.x * 0.5f;
		}

		if (params.v_align == Protean::Font::WIFALIGN_TOP)
		{
			center.y += size.y * 0.5f;
		}
		else if (params.v_align == Protean::Font::WIFALIGN_BOTTOM)
		{
			center.y -= size.y * 0.5f;
		}
		Protean::AABB ret;
		ret.createFromHalfWidth(center, XMFLOAT3(size.x, size.y, 0));
		return ret;
	}

	void SpriteFont::Serialize(Protean::Archive& archive, Protean::EntitySerializer& seri)
	{
		const std::string& dir = archive.GetSourceDirectory();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			std::string textA;
			archive >> textA;
			SetText(textA);
			archive >> fontStyleName;

			archive >> params._flags;
			archive >> params.position;
			archive >> params.size;
			archive >> params.scaling;
			archive >> params.rotation;
			archive >> params.spacingX;
			archive >> params.spacingY;
			archive >> *(uint32_t*)&params.h_align;
			archive >> *(uint32_t*)&params.v_align;
			archive >> params.color;
			archive >> params.shadowColor;
			archive >> params.h_wrap;
			archive >> params.softness;
			archive >> params.bolden;
			archive >> params.shadow_softness;
			archive >> params.shadow_bolden;
			archive >> params.shadow_offset_x;
			archive >> params.shadow_offset_y;
			archive >> params.hdr_scaling;
			archive >> params.intensity;
			archive >> params.shadow_intensity;

			archive >> anim.typewriter.time;
			archive >> anim.typewriter.looped;
			archive >> anim.typewriter.character_start;

			if (!fontStyleName.empty())
			{
				Protean::JobSystem::Execute(seri.ctx, [&](Protean::JobSystem::JobArgs args) {
					if (!fontStyleName.empty())
					{
						fontStyleName = dir + fontStyleName;
						fontStyleResource = Protean::ResourceManager::Load(fontStyleName);
						params.style = fontStyleResource.GetFontStyle();
					}
				});
			}
		}
		else
		{
			seri.RegisterResource(fontStyleName);

			archive << _flags;
			std::string textA = GetTextA();
			archive << textA;
			archive << Protean::Helper::GetPathRelative(dir, fontStyleName);

			archive << params._flags;
			archive << params.position;
			archive << params.size;
			archive << params.scaling;
			archive << params.rotation;
			archive << params.spacingX;
			archive << params.spacingY;
			archive << *(uint32_t*)&params.h_align;
			archive << *(uint32_t*)&params.v_align;
			archive << params.color;
			archive << params.shadowColor;
			archive << params.h_wrap;
			archive << params.softness;
			archive << params.bolden;
			archive << params.shadow_softness;
			archive << params.shadow_bolden;
			archive << params.shadow_offset_x;
			archive << params.shadow_offset_y;
			archive << params.hdr_scaling;
			archive << params.intensity;
			archive << params.shadow_intensity;

			archive << anim.typewriter.time;
			archive << anim.typewriter.looped;
			archive << anim.typewriter.character_start;
		}
	}

}
