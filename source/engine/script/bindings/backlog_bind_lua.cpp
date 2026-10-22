#include "script/bindings/backlog_bind_lua.h"
#include "debug/backlog.h"
#include "script/lua.h"
#include "script/bindings/math_bind_lua.h"
#include "core/helper.h"

#include <string>

#ifdef PLATFORM_WINDOWS_DESKTOP
#include <shellapi.h>
#endif

namespace Protean::Lua::BacklogBindings
{
	int backlog_clear(lua_State* L)
	{
		Protean::Backlog::clear();
		return 0;
	}
	int backlog_post(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);

		std::string ss;

		for (int i = 1; i <= argc; i++)
		{
			if (Protean::Lua::SIsString(L, i))
			{
				ss += Protean::Lua::SGetString(L, i);
			}
			else if (Protean::Lua::SIsNumber(L, i))
			{
				double arg = Protean::Lua::SGetDouble(L, i);
				ss += std::to_string(arg);
			}
			else if (Protean::Lua::SIsBool(L, i)) {
				bool arg = Protean::Lua::SGetBool(L, i);
				ss += arg ? "true" : "false";
			}
			else if (Protean::Lua::SIsNil(L, i)) {
				ss += "nil";
			}
			else
			{
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, i);
				if (vec != nullptr)
				{
					ss += "Vector(" + std::to_string(vec->data.x) + ", " + std::to_string(vec->data.y) + ", " + std::to_string(vec->data.z) + ", " + std::to_string(vec->data.w) + ")";
				}
				else
				{
					Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, i);
					if (mat != nullptr)
					{
						ss += "Matrix(\n";
						ss += "\t" + std::to_string(mat->data._11) + ", " + std::to_string(mat->data._12) + ", " + std::to_string(mat->data._13) + ", " + std::to_string(mat->data._14) + "\n";
						ss += "\t" + std::to_string(mat->data._21) + ", " + std::to_string(mat->data._22) + ", " + std::to_string(mat->data._23) + ", " + std::to_string(mat->data._24) + "\n";
						ss += "\t" + std::to_string(mat->data._31) + ", " + std::to_string(mat->data._32) + ", " + std::to_string(mat->data._33) + ", " + std::to_string(mat->data._34) + "\n";
						ss += "\t" + std::to_string(mat->data._41) + ", " + std::to_string(mat->data._42) + ", " + std::to_string(mat->data._43) + ", " + std::to_string(mat->data._44) + "\n";
						ss += ")";
					}
				}
			}
		}

		if (!ss.empty())
		{
			Protean::Backlog::post(ss);
		}

		return 0;
	}
	int backlog_fontsize(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);

		if (argc > 0)
		{
			Protean::Backlog::setFontSize(Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "backlog_fontsize(int val) not enough arguments!");

		return 0;
	}
	int backlog_isactive(lua_State* L)
	{
		Protean::Lua::SSetBool(L, Protean::Backlog::isActive());
		return 1;
	}
	int backlog_fontrowspacing(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Backlog::setFontRowspacing(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "backlog_fontrowspacing(int val) not enough arguments!");
		return 0;
	}
	int backlog_setlevel(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Backlog::SetLogLevel((Protean::Backlog::LogLevel)Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "backlog_setlevel(LogLevel val) not enough arguments!");
		return 0;
	}
	int backlog_lock(lua_State* L)
	{
		Protean::Backlog::Lock();
		return 0;
	}
	int backlog_unlock(lua_State* L)
	{
		Protean::Backlog::Unlock();
		return 0;
	}
	int backlog_blocklua(lua_State* L)
	{
		Protean::Backlog::BlockLuaExecution();
		return 0;
	}
	int backlog_unblocklua(lua_State* L)
	{
		Protean::Backlog::UnblockLuaExecution();
		return 0;
	}
	int backlog_open(lua_State* L)
	{
		Protean::Backlog::Flush();
		const std::string logfile = Protean::Backlog::GetLogFile();
#ifdef PLATFORM_WINDOWS_DESKTOP
		HINSTANCE result = ShellExecuteA(NULL, "open", logfile.c_str(), NULL, NULL, SW_SHOWNORMAL);
		if ((INT_PTR)result > 32)
		{
			Protean::Backlog::post("Opening log file: " + logfile);
		}
		else
		{
			Protean::Backlog::post("Failed to open log file: " + logfile, Protean::Backlog::LogLevel_Error);
		}
#elif defined(PLATFORM_LINUX)
		std::string op = "xdg-open \"" + logfile + "\"";
		int status = system(op.c_str());
		if (status == 0)
		{
			Protean::Backlog::post("Opening log file: " + logfile);
		}
		else
		{
			Protean::Backlog::post("Failed to open log file: " + logfile, Protean::Backlog::LogLevel_Error);
		}
#else
		Protean::Backlog::post("backlog_open(): not implemented for this operating system!", Protean::Backlog::LogLevel_Warning);
#endif
		return 0;
	}
	int backlog_flush(lua_State* L)
	{
		Protean::Backlog::Flush();
		return 0;
	}
	int backlog_setautoflushinterval(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Backlog::SetAutoFlushInterval((uint32_t)Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "backlog_setautoflushinterval(int milliseconds) not enough arguments!");
		return 0;
	}

	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Protean::Lua::RegisterFunc("backlog_clear", backlog_clear);
			Protean::Lua::RegisterFunc("backlog_post", backlog_post);
			Protean::Lua::RegisterFunc("backlog_fontsize", backlog_fontsize);
			Protean::Lua::RegisterFunc("backlog_isactive", backlog_isactive);
			Protean::Lua::RegisterFunc("backlog_fontrowspacing", backlog_fontrowspacing);
			Protean::Lua::RegisterFunc("backlog_setlevel", backlog_setlevel);
			Protean::Lua::RegisterFunc("backlog_lock", backlog_lock);
			Protean::Lua::RegisterFunc("backlog_unlock", backlog_unlock);
			Protean::Lua::RegisterFunc("backlog_blocklua", backlog_blocklua);
			Protean::Lua::RegisterFunc("backlog_unblocklua", backlog_unblocklua);
			Protean::Lua::RegisterFunc("backlog_open", backlog_open);
			Protean::Lua::RegisterFunc("backlog_flush", backlog_flush);
			Protean::Lua::RegisterFunc("backlog_setautoflushinterval", backlog_setautoflushinterval);

			Protean::Lua::RunText(R"(
LogLevel = {
	None = 0,
	Default = 1,
	Warning = 2,
	Error = 3,
	Success = 4,
}
)");
		}
	}
}
