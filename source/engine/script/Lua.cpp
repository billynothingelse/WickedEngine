#include "script/Lua.h"
#include "script/LuaGlobals.h"
#include "debug/Backlog.h"
#include "core/Helper.h"
#include "script/bindings/ApplicationBindLua.h"
#include "script/bindings/RenderPathBindLua.h"
#include "script/bindings/RenderPath2DBindLua.h"
#include "script/bindings/LoadingScreenBindLua.h"
#include "script/bindings/RenderPath3DBindLua.h"
#include "script/bindings/TextureBindLua.h"
#include "script/bindings/RendererBindLua.h"
#include "script/bindings/AudioBindLua.h"
#include "script/bindings/VideoBindLua.h"
#include "script/bindings/SpriteBindLua.h"
#include "script/bindings/ImageParamsBindLua.h"
#include "script/bindings/SpriteAnimBindLua.h"
#include "script/bindings/SceneBindLua.h"
#include "script/bindings/MathBindLua.h"
#include "script/bindings/InputBindLua.h"
#include "script/bindings/SpriteFontBindLua.h"
#include "script/bindings/BacklogBindLua.h"
#include "script/bindings/NetworkBindLua.h"
#include "script/bindings/PrimitiveBindLua.h"
#include "script/bindings/PhysicsBindLua.h"
#include "script/bindings/VoxelGridBindLua.h"
#include "script/bindings/PathQueryBindLua.h"
#include "script/bindings/TrailRendererBindLua.h"
#include "script/bindings/AsyncBindLua.h"
#include "core/Timer.h"
#include "core/Vector.h"
#include "core/Version.h"
#include "platform/Platform.h"

#include <memory>

namespace Protean::Lua
{
	static constexpr const char* WILUA_ERROR_PREFIX = "[Lua Error] ";
	struct LuaInternal
	{
		lua_State* m_luaState = NULL;

		~LuaInternal()
		{
			if (m_luaState != NULL)
			{
				lua_close(m_luaState);
			}
		}
	};
	LuaInternal& lua_internal()
	{
		static LuaInternal luainternal;
		return luainternal;
	}

	Protean::Application* editorApplication = nullptr;
	Protean::RenderPath* editorRenderPath = nullptr;
	int IsThisEditor(lua_State* L)
	{
		bool ret = editorApplication != nullptr && editorRenderPath != nullptr;
		Protean::Lua::SSetBool(L, ret);
		return 1;
	}
	int ReturnToEditor(lua_State* L)
	{
		if (editorApplication != nullptr && editorRenderPath != nullptr)
		{
			KillProcesses();
			editorApplication->ActivatePath(editorRenderPath);
			Protean::Input::ResetCursors();
		}
		return 0;
	}

	int IsPlatformWindows(lua_State* L)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP
		Protean::Lua::SSetBool(L, true);
#else
		Protean::Lua::SSetBool(L, false);
#endif // PLATFORM_WINDOWS_DESKTOP
		return 1;
	}
	int IsPlatformLinux(lua_State* L)
	{
#ifdef PLATFORM_LINUX
		Protean::Lua::SSetBool(L, true);
#else
		Protean::Lua::SSetBool(L, false);
#endif // PLATFORM_LINUX
		return 1;
	}
	int IsPlatformMACOS(lua_State* L)
	{
#ifdef PLATFORM_MACOS
		Protean::Lua::SSetBool(L, true);
#else
		Protean::Lua::SSetBool(L, false);
#endif // PLATFORM_MACOS
		return 1;
	}
	int IsPlatformIOS(lua_State* L)
	{
#ifdef PLATFORM_IOS
		Protean::Lua::SSetBool(L, true);
#else
		Protean::Lua::SSetBool(L, false);
#endif // PLATFORM_IOS
		return 1;
	}
	int IsPlatformPS5(lua_State* L)
	{
#ifdef PLATFORM_PS5
		Protean::Lua::SSetBool(L, true);
#else
		Protean::Lua::SSetBool(L, false);
#endif // PLATFORM_PS5
		return 1;
	}
	int IsPlatformXBOX(lua_State* L)
	{
#ifdef PLATFORM_XBOX
		Protean::Lua::SSetBool(L, true);
#else
		Protean::Lua::SSetBool(L, false);
#endif // PLATFORM_XBOX
		return 1;
	}

	int FileExists(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc < 1)
		{
			Protean::Lua::SError(L, "FileExists(string name): not enough arguments!");
			return 0;
		}
		Protean::Lua::SSetBool(L, Protean::Helper::FileExists(Protean::Lua::SGetString(L, 1)));
		return 1;
	}
	int DirectoryExists(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc < 1)
		{
			Protean::Lua::SError(L, "DirectoryExists(string name): not enough arguments!");
			return 0;
		}
		Protean::Lua::SSetBool(L, Protean::Helper::DirectoryExists(Protean::Lua::SGetString(L, 1)));
		return 1;
	}
	int DirectoryCreate(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc < 1)
		{
			Protean::Lua::SError(L, "DirectoryCreate(string name): not enough arguments!");
			return 0;
		}
		Protean::Helper::DirectoryCreate(Protean::Lua::SGetString(L, 1));
		return 0;
	}
	int GetSaveDataPath(lua_State* L)
	{
		Protean::Lua::SSetString(L, Protean::Helper::GetSaveDataPath());
		return 1;
	}

	void PostErrorMsg(lua_State* L)
	{
		const char* str = lua_tostring(L, -1);

		if (str == nullptr)
			return;

		std::string ss;
		ss += WILUA_ERROR_PREFIX;
		ss += str;
		Protean::Backlog::post(ss, Protean::Backlog::LogLevel_Error);
		lua_pop(L, 1); // remove error message

		ReturnToEditor(L);
	}

	void PostErrorMsg()
	{
		PostErrorMsg(lua_internal().m_luaState);
	}

	uint32_t GeneratePID()
	{
		static std::atomic<uint32_t> scriptpid_next{ 0 + 1 };
		return scriptpid_next.fetch_add(1);
	}

	uint32_t AttachScriptParameters(std::string& script, const std::string& filename, uint32_t PID, const std::string& customparameters_prepend, const std::string& customparameters_append)
	{
		static const std::string persistent_inject =
			"local runProcess = function(func) "
			"	success, co = Internal_runProcess(script_file(), script_pid(), func);"
			"	return success, co;"
			"end;"
			"if _ENV.PROCESSES_DATA[script_pid()] == nil then"
			"	_ENV.PROCESSES_DATA[script_pid()] = { _INITIALIZED = -1 };"
			"end;"
			"if _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED < 1 then"
			"	_ENV.PROCESSES_DATA[script_pid()]._INITIALIZED = _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED + 1;"
			"end;";

		// Make sure the file path doesn't contain backslash characters, replace them with forward slash.
		//	- backslash would be recognized by lua as escape character
		//	- the path string could be coming from unknown location (content, programmer, filepicker), so always do this
		std::string filepath = filename;
		std::replace(filepath.begin(), filepath.end(), '\\', '/');

		std::string dynamic_inject = "--[[" + filepath + "--]]";
		dynamic_inject += "local function script_file() return \"" + filepath + "\" end;";
		dynamic_inject += "local function script_pid() return \"" + std::to_string(PID) + "\" end;";
		dynamic_inject += "local function script_dir() return \"" + Protean::Helper::GetDirectoryFromPath(filepath) + "\" end;";
		dynamic_inject += persistent_inject;
		script = dynamic_inject + customparameters_prepend + script + customparameters_append;

		return PID;
	}

	int Internal_DoFile(lua_State* L)
	{
		int argc = SGetArgCount(L);

		if (argc > 0)
		{
			uint32_t PID;

			std::string filename = SGetString(L, 1);
			if (argc >= 2)
			{
				PID = SGetInt(L, 2);
			}
			else
			{
				PID = GeneratePID();
			}
				
			std::string customparameters_prepend;
			if(argc >= 3) customparameters_prepend = SGetString(L, 3);
			std::string customparameters_append;
			if(argc >= 4) customparameters_prepend = SGetString(L, 4);

			Protean::vector<uint8_t> filedata;

			if (Protean::Helper::FileRead(filename, filedata))
			{
				std::string command = std::string(filedata.begin(), filedata.end());
				PID = AttachScriptParameters(command, filename, PID, customparameters_prepend, customparameters_append);

				lua_settop(L, 0);

				int status = luaL_loadstring(L, command.c_str());
				if (status == 0)
				{
					status = lua_pcall(L, 0, LUA_MULTRET, 0);
				}

				if (status == 0)
				{
					int argc = lua_gettop(L);
					SSetString(L, std::to_string(PID)); // the PID will be an extra return for dofile after all other returns
					return ++argc;
				}
				else
				{
					PostErrorMsg(L);
					return 0;
				}
			}
		}
		else
		{
			SError(L, "dofile(string filename) not enough arguments!");
		}

		return 0;
	}
	int Internal_DoBinaryFile(lua_State* L)
	{
		int argc = SGetArgCount(L);

		if (argc > 0)
		{
			std::string filename = SGetString(L, 1);
			if (Protean::Lua::RunBinaryFile(filename))
			{
				return 0;
			}
			Protean::Lua::SError(L, "dobinaryfile(string filename): File could not be read!");
		}
		Protean::Lua::SError(L, "dobinaryfile(string filename): Not enough arguments!");
		return 0;
	}
	int Internal_CompileBinaryFile(lua_State* L)
	{
		int argc = SGetArgCount(L);

		if (argc > 1)
		{
			std::string filename_src = SGetString(L, 1);
			std::string filename_dst = SGetString(L, 2);
			Protean::vector<uint8_t> data;
			if (Protean::Lua::CompileFile(filename_src, data))
			{
				Protean::Helper::FileWrite(filename_dst, data.data(), data.size());
				return 0;
			}
			Protean::Lua::SError(L, "compilebinaryfile(string filename_src, filename_dst): Source file could not be read, or compilation failed!");
		}
		Protean::Lua::SError(L, "compilebinaryfile(string filename_src, filename_dst): Not enough arguments!");
		return 0;
	}

	int IsThisDebugBuild(lua_State* L)
	{
#ifdef _DEBUG
		SSetBool(L, true);
#else
		SSetBool(L, false);
#endif
		return 1;
	}

	int GetVersionMajor(lua_State* L)
	{
		SSetInt(L, Protean::Version::GetMajor());
		return 1;
	}
	int GetVersionMinor(lua_State* L)
	{
		SSetInt(L, Protean::Version::GetMinor());
		return 1;
	}
	int GetVersionRevision(lua_State* L)
	{
		SSetInt(L, Protean::Version::GetRevision());
		return 1;
	}
	int GetVersionString(lua_State* L)
	{
		SSetString(L, Protean::Version::GetVersionString());
		return 1;
	}
	int GetCreditsString(lua_State* L)
	{
		SSetString(L, Protean::Version::GetCreditsString());
		return 1;
	}
	int GetSupportersString(lua_State* L)
	{
		SSetString(L, Protean::Version::GetSupportersString());
		return 1;
	}

	void Initialize()
	{
		if (lua_internal().m_luaState != nullptr)
			return; // already initialized

		Protean::Timer timer;

		lua_internal().m_luaState = luaL_newstate();
		luaL_openlibs(lua_internal().m_luaState);
		RegisterFunc("dofile", Internal_DoFile);
		RegisterFunc("dobinaryfile", Internal_DoBinaryFile);
		RegisterFunc("compilebinaryfile", Internal_CompileBinaryFile);
		RunText(wiLua_Globals);

		RegisterFunc("IsThisEditor", IsThisEditor);
		RegisterFunc("ReturnToEditor", ReturnToEditor);
		RegisterFunc("IsThisDebugBuild", IsThisDebugBuild);

		RegisterFunc("GetVersionMajor", GetVersionMajor);
		RegisterFunc("GetVersionMinor", GetVersionMinor);
		RegisterFunc("GetVersionRevision", GetVersionRevision);
		RegisterFunc("GetVersionString", GetVersionString);
		RegisterFunc("GetCreditsString", GetCreditsString);
		RegisterFunc("GetSupportersString", GetSupportersString);

		RegisterFunc("IsPlatformWindows", IsPlatformWindows);
		RegisterFunc("IsPlatformLinux", IsPlatformLinux);
		RegisterFunc("IsPlatformMACOS", IsPlatformMACOS);
		RegisterFunc("IsPlatformIOS", IsPlatformIOS);
		RegisterFunc("IsPlatformPS5", IsPlatformPS5);
		RegisterFunc("IsPlatformXBOX", IsPlatformXBOX);

		RegisterFunc("FileExists", FileExists);
		RegisterFunc("DirectoryExists", DirectoryExists);
		RegisterFunc("DirectoryCreate", DirectoryCreate);
		RegisterFunc("GetSaveDataPath", GetSaveDataPath);

		Vector_BindLua::Bind();
		Matrix_BindLua::Bind();
		Application_BindLua::Bind();
		Canvas_BindLua::Bind();
		RenderPath_BindLua::Bind();
		RenderPath2D_BindLua::Bind();
		LoadingScreen_BindLua::Bind();
		RenderPath3D_BindLua::Bind();
		Texture_BindLua::Bind();
		RendererBindings::Bind();
		Audio_BindLua::Bind();
		Video_BindLua::Bind();
		VideoInstance_BindLua::Bind();
		Sprite_BindLua::Bind();
		ImageParams_BindLua::Bind();
		SpriteAnim_BindLua::Bind();
		SceneBindings::Bind();
		Input_BindLua::Bind();
		SpriteFont_BindLua::Bind();
		BacklogBindings::Bind();
		Network_BindLua::Bind();
		PrimitiveBindings::Bind();
		Physics_BindLua::Bind();
		VoxelGrid_BindLua::Bind();
		PathQuery_BindLua::Bind();
		TrailRenderer_BindLua::Bind();
		Async_BindLua::Bind();

		wilog("Protean::Lua Initialized [Lua %s.%s.%s] (%d ms)", LUA_VERSION_MAJOR, LUA_VERSION_MINOR, LUA_VERSION_RELEASE, (int)std::round(timer.elapsed()));
	}

	lua_State* GetLuaState()
	{
		return lua_internal().m_luaState;
	}

	bool RunScript()
	{
		if(lua_pcall(lua_internal().m_luaState, 0, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
			return false;
		}
		return true;
	}
	bool RunFile(const char* filename)
	{
		Protean::vector<uint8_t> filedata;
		if (Protean::Helper::FileRead(filename, filedata))
		{
			std::string script = std::string(filedata.begin(), filedata.end());
			AttachScriptParameters(script, filename);
			return RunText(script);
		}
		return false;
	}
	bool RunBinaryFile(const char* filename)
	{
		Protean::vector<uint8_t> filedata;
		if (Protean::Helper::FileRead(filename, filedata))
		{
			return RunBinaryData(filedata.data(), filedata.size(), filename);
		}
		return false;
	}
	bool RunText(const char* script)
	{
		if(luaL_loadstring(lua_internal().m_luaState, script) == LUA_OK)
		{
			return RunScript();
		}

		PostErrorMsg();
		return false;
	}
	bool RunBinaryData(const void* data, size_t size, const char* debugname)
	{
		if(luaL_loadbuffer(lua_internal().m_luaState, (const char*)data, size, debugname) == LUA_OK)
		{
			return RunScript();
		}

		PostErrorMsg();
		return false;
	}
	void RegisterFunc(const char* name, lua_CFunction function)
	{
		lua_register(lua_internal().m_luaState, name, function);
	}

	void SetDeltaTime(double dt)
	{
		lua_getglobal(lua_internal().m_luaState, "setDeltaTime");
		SSetDouble(lua_internal().m_luaState, dt);
		if(lua_pcall(lua_internal().m_luaState, 1, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
		}
	}

	inline void SignalHelper(lua_State* L, const char* str)
	{
		lua_getglobal(L, "signal");
		lua_pushstring(L, str);
		if(lua_pcall(L, 1, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
		}
	}
	void FixedUpdate()
	{
		SignalHelper(lua_internal().m_luaState, "wickedengine_fixed_update_tick");
	}
	void Update()
	{
		SignalHelper(lua_internal().m_luaState, "wickedengine_update_tick");
	}
	void Render()
	{
		SignalHelper(lua_internal().m_luaState, "wickedengine_render_tick");
	}
	void Signal(const char* name)
	{
		SignalHelper(lua_internal().m_luaState, name);
	}

	void KillProcesses()
	{
		RunText("killProcesses();");
	}

	const char* SGetString(lua_State* L, int stackpos)
	{
		const char* str = lua_tostring(L, stackpos);
		if (str != nullptr)
			return str;
		return "";
	}
	bool SIsString(lua_State* L, int stackpos)
	{
		return lua_isstring(L, stackpos) != 0;
	}
	bool SIsNumber(lua_State* L, int stackpos)
	{
		return lua_isnumber(L, stackpos) != 0;
	}
	bool SIsBool(lua_State* L, int stackpos)
	{
		return lua_isboolean(L, stackpos);
	}
	bool SIsCFunction(lua_State* L, int stackpos)
	{
		return lua_iscfunction(L, stackpos);
	}
	bool SIsUserdata(lua_State* L, int stackpos)
	{
		return lua_isuserdata(L, stackpos);
	}
	bool SIsNil(lua_State* L, int stackpos) {
		return lua_isnil(L, stackpos);
	}
	int SGetType(lua_State* L, int stackpos)
	{
		return lua_type(L, stackpos);
	}
	int SGetInt(lua_State* L, int stackpos)
	{
		return static_cast<int>(SGetLongLong(L, stackpos));
	}
	long SGetLong(lua_State* L, int stackpos)
	{
		return static_cast<long>(SGetLongLong(L, stackpos));
	}
	long long SGetLongLong(lua_State* L, int stackpos)
	{
		return lua_tointeger(L, stackpos);
	}
	float SGetFloat(lua_State* L, int stackpos)
	{
		return static_cast<float>(SGetDouble(L, stackpos));
	}
	XMFLOAT2 SGetFloat2(lua_State* L, int stackpos)
	{
		return XMFLOAT2(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1));
	}
	XMFLOAT3 SGetFloat3(lua_State* L, int stackpos)
	{
		return XMFLOAT3(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1), SGetFloat(L, stackpos + 2));
	}
	XMFLOAT4 SGetFloat4(lua_State* L, int stackpos)
	{
		return XMFLOAT4(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1), SGetFloat(L, stackpos + 2), SGetFloat(L, stackpos + 3));
	}
	double SGetDouble(lua_State* L, int stackpos)
	{
		return lua_tonumber(L, stackpos);
	}
	bool SGetBool(lua_State* L, int stackpos)
	{
		return lua_toboolean(L, stackpos) != 0;
	}
	int SGetArgCount(lua_State* L)
	{
		return lua_gettop(L);
	}
	void* SGetUserData(lua_State* L)
	{
		return lua_touserdata(L, 1);
	}

	void SSetInt(lua_State* L, int data)
	{
		lua_pushinteger(L, (lua_Integer)data);
	}
	void SSetLong(lua_State* L, long data)
	{
		lua_pushinteger(L, (lua_Integer)data);
	}
	void SSetLongLong(lua_State* L, long long data)
	{
		lua_pushinteger(L, (lua_Integer)data);
	}
	void SSetFloat(lua_State* L, float data)
	{
		lua_pushnumber(L, (lua_Number)data);
	}
	void SSetFloat2(lua_State* L, const XMFLOAT2& data)
	{
		SSetFloat(L, data.x);
		SSetFloat(L, data.y);
	}
	void SSetFloat3(lua_State* L, const XMFLOAT3& data)
	{
		SSetFloat(L, data.x);
		SSetFloat(L, data.y);
		SSetFloat(L, data.z);
	}
	void SSetFloat4(lua_State* L, const XMFLOAT4& data)
	{
		SSetFloat(L, data.x);
		SSetFloat(L, data.y);
		SSetFloat(L, data.z);
		SSetFloat(L, data.w);
	}
	void SSetDouble(lua_State* L, double data)
	{
		lua_pushnumber(L, (lua_Number)data);
	}
	void SSetString(lua_State* L, const char* data)
	{
		lua_pushstring(L, data);
	}
	void SSetBool(lua_State* L, bool data)
	{
		lua_pushboolean(L, static_cast<int>(data));
	}
	void SSetPointer(lua_State* L, void* data)
	{
		lua_pushlightuserdata(L, data);
	}
	void SSetNull(lua_State* L)
	{
		lua_pushnil(L);
	}

	int IntProperty::Get(lua_State* L)
	{
		SSetInt(L, *data);
		return 1;
	}
	int IntProperty::Set(lua_State* L)
	{
		*data = SGetInt(L, 1);
		return 0;
	}
	int LongProperty::Get(lua_State* L)
	{
		SSetLong(L, *data);
		return 1;
	}
	int LongProperty::Set(lua_State* L)
	{
		*data = SGetLong(L, 1);
		return 0;
	}
	int LongLongProperty::Get(lua_State* L)
	{
		SSetLongLong(L, *data);
		return 1;
	}
	int LongLongProperty::Set(lua_State* L)
	{
		*data = SGetLongLong(L, 1);
		return 0;
	}
	int FloatProperty::Get(lua_State* L)
	{
		SSetFloat(L, *data);
		return 1;
	}
	int FloatProperty::Set(lua_State* L)
	{
		*data = SGetFloat(L, 1);
		return 0;
	}
	int DoubleProperty::Get(lua_State* L)
	{
		SSetDouble(L, *data);
		return 1;
	}
	int DoubleProperty::Set(lua_State* L)
	{
		*data = SGetDouble(L, 1);
		return 0;
	}
	int StringProperty::Get(lua_State* L)
	{
		SSetString(L, *data);
		return 1;
	}
	int StringProperty::Set(lua_State* L)
	{
		*data = SGetString(L, 1);
		return 0;
	}
	int BoolProperty::Get(lua_State* L)
	{
		SSetBool(L, *data);
		return 1;
	}
	int BoolProperty::Set(lua_State* L)
	{
		*data = SGetBool(L, 1);
		return 0;
	}


	void SError(lua_State* L, const std::string& error)
	{
		//retrieve line number for error info
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;

		std::string ss;
		ss += WILUA_ERROR_PREFIX;
		ss += "Line " + std::to_string(line) + ": ";
		if (!error.empty())
		{
			ss += error;
		}
		Protean::Backlog::post(ss, Protean::Backlog::LogLevel_Error);

		ReturnToEditor(L);
	}

	bool CompileFile(const char* filename, Protean::vector<uint8_t>& dst)
	{
		Protean::vector<uint8_t> filedata;
		if (Protean::Helper::FileRead(filename, filedata))
		{
			std::string script = std::string(filedata.begin(), filedata.end());
			return CompileText(script.c_str(), dst);
		}
		return false;
	}

	int writer(lua_State* L, const void* p, size_t sz, void* ud)
	{
		Protean::vector<uint8_t>& dst = *(Protean::vector<uint8_t>*)ud;
		for (size_t i = 0; i < sz; ++i)
		{
			dst.push_back(((uint8_t*)p)[i]);
		}
		return LUA_OK;
	}
	bool CompileText(const char* script, Protean::vector<uint8_t>& dst)
	{
		if(luaL_loadstring(lua_internal().m_luaState, script) != LUA_OK)
		{
			PostErrorMsg();
			return false;
		}
		dst.clear();
		if(lua_dump(lua_internal().m_luaState, writer, &dst, 0) != LUA_OK)
		{
			PostErrorMsg();
			lua_pop(lua_internal().m_luaState, 1); // lua_dump does not pop the dumped function from stack
			return false;
		}
		lua_pop(lua_internal().m_luaState, 1); // lua_dump does not pop the dumped function from stack
		return true;
	}

	void EnableEditorFunctionality(Protean::Application* application, Protean::RenderPath* renderpath)
	{
		editorApplication = application;
		editorRenderPath = renderpath;
	}

}
