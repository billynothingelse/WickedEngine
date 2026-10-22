#pragma once
#include "render/Font.h"
#include "audio/Audio.h"
#include "io/Archive.h"
#include "core/ECS.h"
#include "resource/ResourceManager.h"
#include "math/Primitive.h"

#include <string>

namespace Protean
{
	class SpriteFont
	{
	public:
		enum FLAGS
		{
			EMPTY = 0,
			HIDDEN = 1 << 0,
			DISABLE_UPDATE = 1 << 1,
			CAMERA_FACING = 1 << 2,
			CAMERA_SCALING = 1 << 3,
		};
		uint32_t _flags = EMPTY;
		std::wstring text;
		Protean::Font::Params params;
		Protean::Resource fontStyleResource;
		std::string fontStyleName;

		SpriteFont() = default;
		SpriteFont(const std::string& value, const Protean::Font::Params& params = Protean::Font::Params(), const std::string& fontStyleName = "") :params(params), fontStyleName(fontStyleName)
		{
			SetText(value);
			if (!fontStyleName.empty())
			{
				fontStyleResource = Protean::ResourceManager::Load(fontStyleName);
			}
		}
		virtual ~SpriteFont() = default;

		virtual void FixedUpdate();
		virtual void Update(float dt);
		virtual void Draw(Protean::Graphics::CommandList cmd) const;

		constexpr void SetHidden(bool value = true) { if (value) { _flags |= HIDDEN; } else { _flags &= ~HIDDEN; } }
		constexpr bool IsHidden() const { return _flags & HIDDEN; }
		constexpr void SetDisableUpdate(bool value = true) { if (value) { _flags |= DISABLE_UPDATE; } else { _flags &= ~DISABLE_UPDATE; } }
		constexpr void SetCameraFacing(bool value = true) { if (value) { _flags |= CAMERA_FACING; } else { _flags &= ~CAMERA_FACING; } }
		constexpr void SetCameraScaling(bool value = true) { if (value) { _flags |= CAMERA_SCALING; } else { _flags &= ~CAMERA_SCALING; } }

		constexpr bool IsDisableUpdate() const { return _flags & DISABLE_UPDATE; }
		constexpr bool IsCameraFacing() const { return _flags & CAMERA_FACING; }
		constexpr bool IsCameraScaling() const { return _flags & CAMERA_SCALING; }

		XMFLOAT2 TextSize() const;
		float TextWidth() const;
		float TextHeight() const;

		void SetText(const std::string& value);
		void SetText(std::string&& value);
		void SetText(const std::wstring& value);
		void SetText(std::wstring&& value);

		std::string GetTextA() const;
		const std::wstring& GetText() const;

		size_t GetCurrentTextLength() const;

		Protean::AABB GetAABB() const;

		struct Animation
		{
			struct Typewriter
			{
				float time = 0; // time to fully type the text in seconds (0: disable)
				bool looped = false; // if true, typing starts over when finished
				size_t character_start = 0; // starting character for the animation
				Protean::Audio::Sound sound;
				Protean::Audio::SoundInstance soundinstance;

				float elapsed = 0; // internal use; you don't need to initialize

				void reset()
				{
					elapsed = 0;
				}
				void Finish()
				{
					elapsed = time;
				}
				bool IsFinished() const
				{
					return time <= elapsed;
				}
			} typewriter;
		} anim;

		void Serialize(Protean::Archive& archive, Protean::EntitySerializer& seri);
	};
}
