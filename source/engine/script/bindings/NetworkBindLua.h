#pragma once
#include "script/Lua.h"
#include "script/Luna.h"

namespace Protean::Lua
{

	class Network_BindLua
	{
	public:
		inline static constexpr char className[] = "Network";
		static Luna<Network_BindLua>::FunctionType methods[];
		static Luna<Network_BindLua>::PropertyType properties[];

		Network_BindLua() = default;
		Network_BindLua(lua_State* L) {}

		static void Bind();
	};

}
