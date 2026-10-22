#pragma once
#include "script/Lua.h"
#include "script/Luna.h"
#include "core/JobSystem.h"

namespace Protean::Lua
{
	class Async_BindLua
	{
	public:
		Protean::JobSystem::context ctx;
		inline static constexpr char className[] = "Async";
		static Luna<Async_BindLua>::FunctionType methods[];
		static Luna<Async_BindLua>::PropertyType properties[];

		Async_BindLua() = default;
		Async_BindLua(lua_State* L) {}

		int Wait(lua_State* L);
		int IsCompleted(lua_State* L);

		static void Bind();
	};
}
