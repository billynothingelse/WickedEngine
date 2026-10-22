#include "script/bindings/AsyncBindLua.h"

namespace Protean::Lua
{
	Luna<Async_BindLua>::FunctionType Async_BindLua::methods[] = {
		lunamethod(Async_BindLua, Wait),
		lunamethod(Async_BindLua, IsCompleted),
		{ NULL, NULL }
	};
	Luna<Async_BindLua>::PropertyType Async_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int Async_BindLua::Wait(lua_State* L)
	{
		Protean::JobSystem::Wait(ctx);
		return 0;
	}
	int Async_BindLua::IsCompleted(lua_State* L)
	{
		Protean::Lua::SSetBool(L, Protean::JobSystem::IsBusy(ctx) == false);
		return 1;
	}

	void Async_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Async_BindLua>::Register(Protean::Lua::GetLuaState());
		}
	}
}
