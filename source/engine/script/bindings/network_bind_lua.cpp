#include "script/bindings/network_bind_lua.h"
#include "network/network.h"
#include "core/helper.h"

namespace Protean::Lua
{

	Luna<Network_BindLua>::FunctionType Network_BindLua::methods[] = {
		{ NULL, NULL }
	};
	Luna<Network_BindLua>::PropertyType Network_BindLua::properties[] = {
		{ NULL, NULL }
	};

	void Network_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Network_BindLua>::Register(Protean::Lua::GetLuaState());
			Luna<Network_BindLua>::push_global(Protean::Lua::GetLuaState(), "network");
		}
	}

}
