#include "script/bindings/SpriteBindLua.h"
#include "script/bindings/ImageParamsBindLua.h"
#include "script/bindings/SpriteAnimBindLua.h"
#include "script/bindings/TextureBindLua.h"

namespace Protean::Lua
{

	Luna<Sprite_BindLua>::FunctionType Sprite_BindLua::methods[] = {
		lunamethod(Sprite_BindLua, SetParams),
		lunamethod(Sprite_BindLua, GetParams),
		lunamethod(Sprite_BindLua, SetAnim),
		lunamethod(Sprite_BindLua, GetAnim),
		lunamethod(Sprite_BindLua, SetTexture),
		lunamethod(Sprite_BindLua, GetTexture),
		lunamethod(Sprite_BindLua, SetMaskTexture),
		lunamethod(Sprite_BindLua, GetMaskTexture),
		lunamethod(Sprite_BindLua, SetBackgroundTexture),
		lunamethod(Sprite_BindLua, GetBackgroundTexture),
		lunamethod(Sprite_BindLua, SetHidden),
		lunamethod(Sprite_BindLua, IsHidden),

		{ NULL, NULL }
	};
	Luna<Sprite_BindLua>::PropertyType Sprite_BindLua::properties[] = {
		lunaproperty(Sprite_BindLua, Params),
		lunaproperty(Sprite_BindLua, Anim),
		{ NULL, NULL }
	};

	Sprite_BindLua::Sprite_BindLua(const Protean::Sprite& sprite) :sprite(sprite)
	{
	}

	Sprite_BindLua::Sprite_BindLua(lua_State* L)
	{
		std::string name = "", mask = "";
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			name = Protean::Lua::SGetString(L, 1);
			if (argc > 1)
			{
				mask = Protean::Lua::SGetString(L, 2);
			}
		}
		sprite = Protean::Sprite(name, mask);
	}

	int Sprite_BindLua::SetParams(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Lua::ImageParams_BindLua* params = Luna<Protean::Lua::ImageParams_BindLua>::check(L, 1);
			if (params != nullptr)
			{
				sprite.params = params->params;
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetParams(ImageEffects effects) not enough arguments!");
		}
		return 0;
	}
	int Sprite_BindLua::GetParams(lua_State* L)
	{
		Luna<Protean::Lua::ImageParams_BindLua>::push(L, sprite.params);
		return 1;
	}
	int Sprite_BindLua::SetAnim(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SpriteAnim_BindLua* anim = Luna<SpriteAnim_BindLua>::check(L, 1);
			if (anim != nullptr)
			{
				sprite.anim = anim->anim;
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetAnim(SpriteAnim anim) not enough arguments!");
		}
		return 0;
	}
	int Sprite_BindLua::GetAnim(lua_State* L)
	{
		Luna<SpriteAnim_BindLua>::push(L, sprite.anim);
		return 1;
	}
	int Sprite_BindLua::SetTexture(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Texture_BindLua* tex = Luna<Texture_BindLua>::check(L, 1);
			if (tex != nullptr)
			{
				sprite.textureResource = tex->resource;
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetTexture(Texture texture) not enough arguments!");
		}
		return 0;
	}
	int Sprite_BindLua::GetTexture(lua_State* L)
	{
		Luna<Texture_BindLua>::push(L, sprite.textureResource);
		return 1;
	}
	int Sprite_BindLua::SetMaskTexture(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Texture_BindLua* tex = Luna<Texture_BindLua>::check(L, 1);
			if (tex != nullptr)
			{
				sprite.maskResource = tex->resource;
				sprite.params.setMaskMap(&tex->resource.GetTexture());
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetMaskTexture(Texture texture) not enough arguments!");
		}
		return 0;
	}
	int Sprite_BindLua::GetMaskTexture(lua_State* L)
	{
		Luna<Texture_BindLua>::push(L, sprite.maskResource);
		return 1;
	}
	int Sprite_BindLua::SetBackgroundTexture(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Texture_BindLua* tex = Luna<Texture_BindLua>::check(L, 1);
			if (tex != nullptr)
			{
				sprite.backgroundResource = tex->resource;
				sprite.params.setBackgroundMap(&tex->resource.GetTexture());
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetBackgroundTexture(Texture texture) not enough arguments!");
		}
		return 0;
	}
	int Sprite_BindLua::GetBackgroundTexture(lua_State* L)
	{
		Luna<Texture_BindLua>::push(L, sprite.backgroundResource);
		return 1;
	}
	int Sprite_BindLua::SetHidden(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc < 1)
		{
			Protean::Lua::SError(L, "SetHidden(bool value) not enough arguments!");
			return 0;
		}
		sprite.SetHidden(Protean::Lua::SGetBool(L, 1));
		return 0;
	}
	int Sprite_BindLua::IsHidden(lua_State* L)
	{
		Protean::Lua::SSetBool(L, sprite.IsHidden());
		return 1;
	}

	void Sprite_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Sprite_BindLua>::Register(Protean::Lua::GetLuaState());
		}
	}

}
