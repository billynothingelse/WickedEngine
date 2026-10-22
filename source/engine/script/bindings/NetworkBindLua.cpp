#include "script/bindings/NetworkBindLua.h"
#include "network/Network.h"
#include "core/Helper.h"

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
