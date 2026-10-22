#pragma once
#include "script/Lua.h"
#include "script/Luna.h"
#include "render/GraphicsAPI/Graphics.h"
#include "resource/ResourceManager.h"

namespace Protean::Lua
{

	class Texture_BindLua
	{
	public:
		Protean::Resource resource;

		inline static constexpr char className[] = "Texture";
		static Luna<Texture_BindLua>::FunctionType methods[];
		static Luna<Texture_BindLua>::PropertyType properties[];

		Texture_BindLua() = default;
		Texture_BindLua(Protean::Resource resource) :resource(resource) {}
		Texture_BindLua(Protean::Graphics::Texture texture) { resource.SetTexture(texture); }
		Texture_BindLua(lua_State* L);

		int GetLogo(lua_State* L);
		int CreateGradientTexture(lua_State* L);
		int CreateLensDistortionNormalMap(lua_State* L);
		int Save(lua_State* L);

		int IsValid(lua_State* L);
		int GetWidth(lua_State* L);
		int GetHeight(lua_State* L);
		int GetDepth(lua_State* L);
		int GetArraySize(lua_State* L);

		static void Bind();
	};

}
