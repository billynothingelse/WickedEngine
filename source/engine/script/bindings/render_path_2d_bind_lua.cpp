#include "script/bindings/render_path_2d_bind_lua.h"
#include "script/bindings/render_path_3d_bind_lua.h"
#include "script/bindings/sprite_bind_lua.h"
#include "script/bindings/sprite_font_bind_lua.h"
#include "script/bindings/video_bind_lua.h"

#include <string>

namespace Protean::Lua
{

	Luna<RenderPath2D_BindLua>::FunctionType RenderPath2D_BindLua::methods[] = {
		lunamethod(RenderPath2D_BindLua, AddSprite),
		lunamethod(RenderPath2D_BindLua, AddVideoSprite),
		lunamethod(RenderPath2D_BindLua, AddFont),
		lunamethod(RenderPath2D_BindLua, RemoveSprite),
		lunamethod(RenderPath2D_BindLua, RemoveFont),
		lunamethod(RenderPath2D_BindLua, ClearSprites),
		lunamethod(RenderPath2D_BindLua, ClearFonts),
		lunamethod(RenderPath2D_BindLua, GetSpriteOrder),
		lunamethod(RenderPath2D_BindLua, GetFontOrder),
		lunamethod(RenderPath2D_BindLua, AddLayer),
		lunamethod(RenderPath2D_BindLua, GetLayers),
		lunamethod(RenderPath2D_BindLua, SetLayerOrder),
		lunamethod(RenderPath2D_BindLua, SetSpriteOrder),
		lunamethod(RenderPath2D_BindLua, SetFontOrder),
		lunamethod(RenderPath2D_BindLua, GetHDRScaling),
		lunamethod(RenderPath2D_BindLua, SetHDRScaling),
		lunamethod(RenderPath_BindLua, GetLayerMask),
		lunamethod(RenderPath_BindLua, SetLayerMask),

		lunamethod(RenderPath2D_BindLua, CopyFrom),
		{ NULL, NULL }
	};
	Luna<RenderPath2D_BindLua>::PropertyType RenderPath2D_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int RenderPath2D_BindLua::AddSprite(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "AddSprite() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Lua::Sprite_BindLua* sprite = Luna<Protean::Lua::Sprite_BindLua>::lightcheck(L, 1);
			if (sprite != nullptr)
			{
				RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
				if (ccomp != nullptr)
				{
					if (argc > 1)
						ccomp->AddSprite(&sprite->sprite, Protean::Lua::SGetString(L, 2));
					else
						ccomp->AddSprite(&sprite->sprite);
				}
				else
				{
					Protean::Lua::SError(L, "AddSprite(Sprite sprite, opt string layer) not a RenderPath2D!");
				}
			}
			else
				Protean::Lua::SError(L, "AddSprite(Sprite sprite, opt string layer) argument is not a Sprite!");
		}
		else
		{
			Protean::Lua::SError(L, "AddSprite(Sprite sprite, opt string layer) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::AddVideoSprite(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "AddVideoSprite() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			Protean::Lua::VideoInstance_BindLua* videoinstance = Luna<Protean::Lua::VideoInstance_BindLua>::lightcheck(L, 1);
			if (videoinstance == nullptr)
			{
				Protean::Lua::SError(L, "AddVideoSprite(VideoInstance videoinstance, Sprite sprite, opt string layer) first argument is not a VideoInstance!");
				return 0;
			}
			Protean::Lua::Sprite_BindLua* sprite = Luna<Protean::Lua::Sprite_BindLua>::lightcheck(L, 2);
			if (sprite == nullptr)
			{
				Protean::Lua::SError(L, "AddVideoSprite(VideoInstance videoinstance, Sprite sprite, opt string layer) second argument is not a Sprite!");
				return 0;
			}

			RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				if (argc > 1)
					ccomp->AddVideoSprite(&videoinstance->videoinstance, &sprite->sprite, Protean::Lua::SGetString(L, 3));
				else
					ccomp->AddVideoSprite(&videoinstance->videoinstance, &sprite->sprite);
			}
			else
			{
				Protean::Lua::SError(L, "AddVideoSprite(VideoInstance videoinstance, Sprite sprite, opt string layer) not a RenderPath2D!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "AddVideoSprite(VideoInstance videoinstance, Sprite sprite, opt string layer) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::AddFont(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "AddFont() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Lua::SpriteFont_BindLua* font = Luna<Protean::Lua::SpriteFont_BindLua>::lightcheck(L, 1);
			if (font != nullptr)
			{
				RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
				if (ccomp != nullptr)
				{
					if (argc > 1)
						ccomp->AddFont(&font->font, Protean::Lua::SGetString(L, 2));
					else
						ccomp->AddFont(&font->font);
				}
				else
				{
					Protean::Lua::SError(L, "AddFont(Font font, opt string layer) not a RenderPath2D!");
				}
			}
			else
				Protean::Lua::SError(L, "AddFont(Font font, opt string layer) argument is not a Font!");
		}
		else
		{
			Protean::Lua::SError(L, "AddFont(Font font, opt string layer) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::RemoveSprite(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "RemoveSprite() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Lua::Sprite_BindLua* sprite = Luna<Protean::Lua::Sprite_BindLua>::lightcheck(L, 1);
			if (sprite != nullptr)
			{
				RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
				if (ccomp != nullptr)
				{
					ccomp->RemoveSprite(&sprite->sprite);
				}
				else
				{
					Protean::Lua::SError(L, "RemoveSprite(Sprite sprite) not a RenderPath2D!");
				}
			}
			else
				Protean::Lua::SError(L, "RemoveSprite(Sprite sprite) argument is not a Sprite!");
		}
		else
		{
			Protean::Lua::SError(L, "RemoveSprite(Sprite sprite) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::RemoveFont(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "RemoveFont() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Lua::SpriteFont_BindLua* font = Luna<Protean::Lua::SpriteFont_BindLua>::lightcheck(L, 1);
			if (font != nullptr)
			{
				RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
				if (ccomp != nullptr)
				{
					ccomp->RemoveFont(&font->font);
				}
				else
				{
					Protean::Lua::SError(L, "RemoveFont(Font font) not a RenderPath2D!");
				}
			}
			else
				Protean::Lua::SError(L, "RemoveFont(Font font) argument is not a Font!");
		}
		else
		{
			Protean::Lua::SError(L, "RemoveFont(Font font) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::ClearSprites(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "ClearSprites() component is empty!");
			return 0;
		}
		RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
		if (ccomp != nullptr)
		{
			ccomp->ClearSprites();
		}
		else
		{
			Protean::Lua::SError(L, "ClearSprites() not a RenderPath2D!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::ClearFonts(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "ClearFonts() component is empty!");
			return 0;
		}
		RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
		if (ccomp != nullptr)
		{
			ccomp->ClearFonts();
		}
		else
		{
			Protean::Lua::SError(L, "ClearFonts() not a RenderPath2D!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::GetSpriteOrder(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "GetSpriteOrder() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Lua::Sprite_BindLua* sprite = Luna<Protean::Lua::Sprite_BindLua>::lightcheck(L, 1);
			if (sprite != nullptr)
			{
				RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
				if (ccomp != nullptr)
				{
					Protean::Lua::SSetInt(L, ccomp->GetSpriteOrder(&sprite->sprite));
					return 1;
				}
				else
				{
					Protean::Lua::SError(L, "GetSpriteOrder(Sprite sprite) not a RenderPath2D!");
				}
			}
			else
				Protean::Lua::SError(L, "GetSpriteOrder(Sprite sprite) argument is not a Sprite!");
		}
		else
		{
			Protean::Lua::SError(L, "GetSpriteOrder(Sprite sprite) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::GetFontOrder(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "GetFontOrder() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Lua::SpriteFont_BindLua* font = Luna<Protean::Lua::SpriteFont_BindLua>::lightcheck(L, 1);
			if (font != nullptr)
			{
				RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
				if (ccomp != nullptr)
				{
					Protean::Lua::SSetInt(L, ccomp->GetFontOrder(&font->font));
					return 1;
				}
				else
				{
					Protean::Lua::SError(L, "GetFontOrder(Font font) not a RenderPath2D!");
				}
			}
			else
				Protean::Lua::SError(L, "GetFontOrder(Font font) argument is not a Sprite!");
		}
		else
		{
			Protean::Lua::SError(L, "GetFontOrder(Font font) not enough arguments!");
		}
		return 0;
	}

	int RenderPath2D_BindLua::AddLayer(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "AddLayer() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				ccomp->AddLayer(Protean::Lua::SGetString(L, 1));
			}
			else
			{
				Protean::Lua::SError(L, "AddLayer(string name) not a RenderPath2D!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "AddLayer(string name) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::GetLayers(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "GetLayers() component is empty!");
			return 0;
		}

		RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
		if (ccomp != nullptr)
		{
			std::string ss;
			for (auto& x : ccomp->layers)
			{
				ss += x.name + "\n";
			}
			Protean::Lua::SSetString(L, ss);
			return 1;
		}
		else
		{
			Protean::Lua::SError(L, "GetLayers() not a RenderPath2D!");
		}

		return 0;
	}
	int RenderPath2D_BindLua::SetLayerOrder(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetLayerOrder() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				ccomp->SetLayerOrder(Protean::Lua::SGetString(L, 1), Protean::Lua::SGetInt(L, 2));
			}
			else
			{
				Protean::Lua::SError(L, "SetLayerOrder(string name, int order) not a RenderPath2D!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetLayerOrder(string name, int order) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::SetSpriteOrder(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetSpriteOrder() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				Protean::Lua::Sprite_BindLua* sprite = Luna<Protean::Lua::Sprite_BindLua>::lightcheck(L, 1);
				if (sprite != nullptr)
				{
					ccomp->SetSpriteOrder(&sprite->sprite, Protean::Lua::SGetInt(L, 2));
				}
				else
				{
					Protean::Lua::SError(L, "SetSpriteOrder(Sprite sprite, int order) argument is not a Sprite!");
				}
			}
			else
			{
				Protean::Lua::SError(L, "SetSpriteOrder(Sprite sprite, int order) not a RenderPath2D!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetSpriteOrder(Sprite sprite, int order) not enough arguments!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::SetFontOrder(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFontOrder() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				Protean::Lua::SpriteFont_BindLua* font = Luna<Protean::Lua::SpriteFont_BindLua>::lightcheck(L, 1);
				if (font != nullptr)
				{
					ccomp->SetFontOrder(&font->font, Protean::Lua::SGetInt(L, 2));
				}
				else
				{
					Protean::Lua::SError(L, "SetFontOrder(Font font, int order) argument is not a Font!");
				}
			}
			else
			{
				Protean::Lua::SError(L, "SetFontOrder(Font font, int order) not a RenderPath2D!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetFontOrder(Font font, int order) not enough arguments!");
		}
		return 0;
	}

	int RenderPath2D_BindLua::GetHDRScaling(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "GetHDRScaling() component is empty!");
			return 0;
		}
		RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
		if (ccomp != nullptr)
		{
			Protean::Lua::SSetFloat(L, ccomp->GetHDRScaling());
			return 1;
		}
		else
		{
			Protean::Lua::SError(L, "SetHDRScaling(float value) not a RenderPath2D!");
		}
		return 0;
	}
	int RenderPath2D_BindLua::SetHDRScaling(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetHDRScaling(float value) component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			RenderPath2D* ccomp = static_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				ccomp->SetHDRScaling(Protean::Lua::SGetFloat(L, 1));
			}
			else
			{
				Protean::Lua::SError(L, "SetHDRScaling(float value) not a RenderPath2D!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetHDRScaling(float value) not enough arguments!");
		}
		return 0;
	}

	int RenderPath2D_BindLua::CopyFrom(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "CopyFrom(RenderPath other) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			RenderPath3D_BindLua* other3D = Luna<RenderPath3D_BindLua>::lightcheck(L, 1);
			if (other3D != nullptr)
			{
				*component = *other3D->component;
				return 0;
			}
			RenderPath2D_BindLua* other2D = Luna<RenderPath2D_BindLua>::lightcheck(L, 1);
			if (other2D != nullptr)
			{
				*component = *other2D->component;
				return 0;
			}
			RenderPath_BindLua* other = Luna<RenderPath_BindLua>::lightcheck(L, 1);
			if (other != nullptr)
			{
				*component = *other->component;
				return 0;
			}

			Protean::Lua::SError(L, "CopyFrom(RenderPath other) parameter is not a RenderPath3D!");
		}
		else
			Protean::Lua::SError(L, "CopyFrom(RenderPath other) not enough arguments!");
		return 0;
	}

	void RenderPath2D_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<RenderPath2D_BindLua>::Register(Protean::Lua::GetLuaState());
		}
	}

}
