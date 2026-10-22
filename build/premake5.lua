-- Protean project-wide build script.
--
--   Windows: bootstrap.bat            (generates build/projects/Protean.sln and builds it)
--   Linux:   ./bootstrap.sh           (generates build/projects/Makefile and builds it)
--
-- Manual usage from this folder: premake5 vs2022 | premake5 gmake | premake5 xcode4
--
-- Layout:
--   build/projects   generated solutions / makefiles
--   build/temp       intermediate object files
--   build/bin        executables and libraries
--
-- Configurations:
--   Debug        no optimisation, game is a DLL loaded by the launcher
--   Development  optimised, game is a DLL loaded by the launcher
--   Shipping     optimised, game is a single executable with the engine linked statically

ROOT = path.getabsolute("..")
local SOURCE = ROOT .. "/source"
local EXTERNAL = ROOT .. "/external"
local DATA = ROOT .. "/data"
local BIN = ROOT .. "/build/bin/%{cfg.buildcfg}"
local TEMP = ROOT .. "/build/temp/%{cfg.buildcfg}/%{prj.name}"

workspace "Protean"
	location (ROOT .. "/build/projects")
	configurations { "Debug", "Development", "Shipping" }
	platforms { "x64" }
	startproject "Editor"

	language "C++"
	cppdialect "C++17"
	cdialect "C99"
	exceptionhandling "Off"
	rtti "Off"
	staticruntime "Off"
	characterset "Unicode"
	warnings "Default"
	flags { "MultiProcessorCompile" }

	targetdir (BIN)
	objdir (TEMP)
	debugdir (DATA .. "/editor")

	-- Must match between Jolt and the engine, Jolt validates this at runtime
	defines {
		"JPH_DEBUG_RENDERER",
		"JPH_USE_SSE4_1", "JPH_USE_SSE4_2", "JPH_USE_AVX",
		"JPH_USE_LZCNT", "JPH_USE_TZCNT", "JPH_USE_F16C", "JPH_USE_FMADD",
		"_XM_SSE4_INTRINSICS_", "_XM_AVX_INTRINSICS_", "_XM_F16C_INTRINSICS_", "_XM_FMA3_INTRINSICS_",
	}

	filter "platforms:x64"
		architecture "x86_64"

	filter "configurations:Debug"
		defines { "_DEBUG", "PROTEAN_DEBUG", "PROTEAN_DEVELOPMENT" }
		symbols "On"
		optimize "Off"

	filter "configurations:Development"
		defines { "NDEBUG", "PROTEAN_DEVELOPMENT" }
		symbols "On"
		optimize "Speed"

	filter "configurations:Shipping"
		defines { "NDEBUG", "PROTEAN_SHIPPING" }
		symbols "On"
		optimize "Speed"

	filter "system:windows"
		systemversion "latest"
		defines { "WIN32=1", "_HAS_EXCEPTIONS=0", "UNICODE", "_UNICODE", "NOMINMAX" }
		buildoptions { "/utf-8", "/bigobj" }
		vectorextensions "AVX"
		disablewarnings { "4996" }

	filter { "system:windows", "configurations:Shipping" }
		buildoptions { "/GS-" }

	filter "system:linux"
		toolset "gcc"
		pic "On" -- static libraries are linked into the game shared library
		defines { "SDL2=1", "_GLIBCXX_USE_CXX11_ABI=1", "WI_UNORDERED_MAP_TYPE=2" }
		buildoptions {
			"-mavx", "-mpopcnt", "-mlzcnt", "-mbmi", "-mf16c", "-mfma", "-mfpmath=sse",
			"-pthread",
			"-Wno-unused-variable", "-Wno-unused-function", "-Wno-unused-but-set-variable",
			"-Wno-sign-compare", "-Wno-strict-aliasing",
		}
		includedirs { "/usr/include/SDL2" }
		linkoptions { "-pthread" }

	filter { "system:linux", "configurations:Shipping" }
		buildoptions { "-fno-stack-protector", "-fcf-protection=none", "-fno-stack-clash-protection", "-fno-stack-check" }

	filter "system:macosx"
		defines { "_GLIBCXX_USE_CXX11_ABI=1" }
		buildoptions { "-mavx", "-mpopcnt", "-mlzcnt", "-mbmi", "-mf16c", "-mfma" }

	filter {}

-- Shared settings for anything that links the engine
function UseEngine()
	includedirs { SOURCE .. "/engine", EXTERNAL }
	links { "Engine", "Jolt", "External" }

	filter "system:windows"
		-- system libraries are pulled in with #pragma comment(lib) in the engine sources
		postbuildcommands {
			'{COPYFILE} "' .. EXTERNAL .. '/dxc/bin/dxcompiler.dll" "%{cfg.targetdir}"',
		}
	filter "system:linux"
		links { "FAudio", "SDL2", "dl", "pthread" }
		linkoptions { "-Wl,--export-dynamic", "-Wl,-rpath,'$$ORIGIN'" }
		postbuildcommands {
			'{COPYFILE} "' .. EXTERNAL .. '/dxc/bin/libdxcompiler.so" "%{cfg.targetdir}"',
		}
	filter "system:macosx"
		links {
			"Metal.framework", "MetalFX.framework", "QuartzCore.framework", "Cocoa.framework",
			"IOKit.framework", "GameController.framework", "AudioToolbox.framework",
			"CoreAudio.framework", "VideoToolbox.framework", "CoreMedia.framework", "CoreVideo.framework",
		}
	filter {}
end

group "External"

project "External"
	kind "StaticLib"
	location (ROOT .. "/build/projects/External")
	warnings "Off"
	files {
		EXTERNAL .. "/*.h", EXTERNAL .. "/*.hpp",
		EXTERNAL .. "/*.cpp", EXTERNAL .. "/*.c",
		EXTERNAL .. "/meshoptimizer/*.cpp", EXTERNAL .. "/meshoptimizer/*.h",
		EXTERNAL .. "/lua/*.c", EXTERNAL .. "/lua/*.h",
	}
	removefiles {
		EXTERNAL .. "/D3D12MemAlloc.cpp", -- included by the D3D12 backend
		EXTERNAL .. "/volk.c",            -- included by volk.h
		EXTERNAL .. "/lua/lua.c",         -- standalone interpreter
		-- editor-only third party code, compiled by the editor
		EXTERNAL .. "/ufbx.c", EXTERNAL .. "/xatlas.cpp", EXTERNAL .. "/miniply.cpp",
	}
	filter "system:linux or system:macosx"
		defines { "LUA_USE_POSIX=1" }
	filter {}

project "Jolt"
	kind "StaticLib"
	location (ROOT .. "/build/projects/External")
	includedirs { EXTERNAL }
	files { EXTERNAL .. "/Jolt/**.cpp", EXTERNAL .. "/Jolt/**.h", EXTERNAL .. "/Jolt/**.inl" }
	filter "system:linux"
		buildoptions { "-Wno-stringop-overflow" }
	filter {}

if os.target() == "linux" then
project "FAudio"
	kind "StaticLib"
	language "C"
	location (ROOT .. "/build/projects/External")
	warnings "Off"
	includedirs { EXTERNAL .. "/FAudio/include", EXTERNAL .. "/FAudio/src" }
	files { EXTERNAL .. "/FAudio/src/*.c", EXTERNAL .. "/FAudio/src/*.h", EXTERNAL .. "/FAudio/include/*.h" }
end

group "Engine"

project "Engine"
	kind "StaticLib"
	location (ROOT .. "/build/projects/Engine")
	targetname "Protean"
	includedirs { SOURCE .. "/engine", EXTERNAL }
	-- Used to locate shader sources for runtime compilation in non-shipping builds
	defines { 'PROTEAN_SHADER_SOURCE_PATH="' .. SOURCE .. '/engine/render/shaders/"' }
	files {
		SOURCE .. "/engine/**.h",
		SOURCE .. "/engine/**.cpp",
		SOURCE .. "/engine/**.hlsl",
		SOURCE .. "/engine/**.hlsli",
		SOURCE .. "/engine/**.natvis",
	}
	-- Shaders are compiled at runtime (or offline by the ShaderCompiler tool)
	filter "files:**.hlsl or files:**.hlsli"
		buildaction "None"
	filter "system:linux"
		includedirs { EXTERNAL .. "/FAudio/include" }
	filter "system:macosx"
		files { SOURCE .. "/engine/**.mm" }
	filter {}

group "Tools"

project "ShaderCompiler"
	kind "ConsoleApp"
	location (ROOT .. "/build/projects/Tools")
	debugdir (SOURCE .. "/engine")
	files { SOURCE .. "/tools/ShaderCompiler/**.cpp" }
	UseEngine()

group "Applications"

project "Editor"
	kind "WindowedApp"
	location (ROOT .. "/build/projects/Editor")
	targetname "ProteanEditor"
	includedirs { SOURCE .. "/editor" }
	files {
		SOURCE .. "/editor/*.h", SOURCE .. "/editor/*.cpp",
		EXTERNAL .. "/xatlas.cpp", EXTERNAL .. "/miniply.cpp", -- ufbx.c is included by ModelImporter_FBX.cpp
	}
	removefiles { SOURCE .. "/editor/main_*" }
	pchheader "stdafx.h"
	pchsource (SOURCE .. "/editor/stdafx.cpp")
	UseEngine()
	filter "files:../../external/**"
		flags { "NoPCH" }
		warnings "Off"
	filter "system:windows"
		files { SOURCE .. "/editor/main_Windows.cpp", SOURCE .. "/editor/Editor.rc", SOURCE .. "/editor/Editor.manifest" }
	filter "system:linux"
		files { SOURCE .. "/editor/main_SDL2.cpp" }
	filter "system:macosx"
		files { SOURCE .. "/editor/main_MacOS.mm" }
	filter {}

-- The game module. In Debug/Development it is a shared library loaded (and reloadable) by the
-- Launcher, in Shipping it is a standalone executable that links the engine statically.
project "Game"
	location (ROOT .. "/build/projects/Game")
	debugdir (DATA)
	includedirs { SOURCE .. "/game" }
	files { SOURCE .. "/game/**.h", SOURCE .. "/game/**.cpp" }
	removefiles { SOURCE .. "/game/launcher/**" }
	UseEngine()
	filter "configurations:Debug or configurations:Development"
		kind "SharedLib"
		defines { "PROTEAN_GAME_DLL" }
		removefiles { SOURCE .. "/game/main/**" }
	filter "configurations:Shipping"
		kind "WindowedApp"
	filter {}

project "Launcher"
	location (ROOT .. "/build/projects/Game")
	targetname "ProteanGame"
	debugdir (DATA)
	includedirs { SOURCE .. "/engine", SOURCE .. "/game" }
	files { SOURCE .. "/game/launcher/**.h", SOURCE .. "/game/launcher/**.cpp" }
	dependson { "Game" }
	filter "configurations:Debug or configurations:Development"
		kind "WindowedApp"
	filter "configurations:Shipping"
		kind "None"
	filter "system:linux"
		links { "dl" }
		linkoptions { "-Wl,-rpath,'$$ORIGIN'" }
	filter {}

group "Samples"

project "Tests"
	kind "WindowedApp"
	location (ROOT .. "/build/projects/Samples")
	debugdir (SOURCE .. "/samples/Tests")
	files { SOURCE .. "/samples/Tests/*.h", SOURCE .. "/samples/Tests/*.cpp" }
	removefiles { SOURCE .. "/samples/Tests/main_*" }
	UseEngine()
	filter "system:windows"
		files { SOURCE .. "/samples/Tests/main_Windows.cpp", SOURCE .. "/samples/Tests/Tests.rc" }
	filter "system:linux"
		files { SOURCE .. "/samples/Tests/main_SDL2.cpp" }
	filter {}
