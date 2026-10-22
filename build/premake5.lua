-- Protean project-wide build script.
--
-- Requires premake 5.0.0 or newer (bootstrap downloads it to build/premake5[.exe]).
--
--   Windows: bootstrap.bat            (generates build/projects/Protean.slnx and builds it)
--   Linux:   ./bootstrap.sh           (generates build/projects/Makefile and builds it)
--
-- Manual usage from this folder: premake5 vs2026 | premake5 vs2022 | premake5 gmake | premake5 xcode4
--
-- Layout:
--   build/projects                     generated solutions / makefiles
--   build/temp/<system>-<arch>-<config> intermediate object files
--   binaries/<system>-<arch>-<config>   executables and libraries, e.g. binaries/windows-x64-debug
--
-- Configurations:
--   Debug        no optimisation, game is a DLL loaded by the launcher
--   Development  optimised, game is a DLL loaded by the launcher
--   Shipping     optimised, game is a single executable with the engine linked statically

ROOT = path.getabsolute("..")
local SOURCE = ROOT .. "/source"
local EXTERNAL = ROOT .. "/external"
local DATA = ROOT .. "/data"
local OUTPUT = "%{cfg.system}-%{cfg.platform}-%{cfg.buildcfg:lower()}"
local BIN = ROOT .. "/binaries/" .. OUTPUT
local TEMP = ROOT .. "/build/temp/" .. OUTPUT .. "/%{prj.name}"

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
	multiprocessorcompile "On"

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
		defines { "WIN32=1", "UNICODE", "_UNICODE", "NOMINMAX" } -- exceptionhandling "Off" adds _HAS_EXCEPTIONS=0
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

local DEV = "configurations:Debug or configurations:Development"
local SHIPPING = "configurations:Shipping"

local APPLE_FRAMEWORKS = {
	"Metal.framework", "MetalFX.framework", "QuartzCore.framework", "Cocoa.framework",
	"IOKit.framework", "GameController.framework", "AudioToolbox.framework",
	"CoreAudio.framework", "VideoToolbox.framework", "CoreMedia.framework", "CoreVideo.framework",
}

-- Shared settings for anything that links the engine.
--	Debug/Development: the engine is a shared library (Protean.dll / libProtean.so), so the editor, the
--	launcher and the game module all share one engine instance.
--	Shipping: the engine and its dependencies are linked statically.
function UseEngine()
	includedirs { SOURCE .. "/engine", EXTERNAL }
	links { "Engine" }
	filter (SHIPPING)
		links { "Jolt" }
	filter {}
	-- Small stateless helper libraries (meshoptimizer, stb, ...) are also used directly by applications
	links { "External" }

	filter "system:windows"
		-- system libraries are pulled in with #pragma comment(lib) in the engine sources
		postbuildcommands {
			'{COPYFILE} "' .. EXTERNAL .. '/dxc/bin/dxcompiler.dll" "%{cfg.targetdir}"',
		}
	filter { "system:linux", SHIPPING }
		links { "FAudio" }
	filter "system:linux"
		links { "SDL2", "dl", "pthread" }
		linkoptions { "-Wl,--export-dynamic", "-Wl,-rpath,'$$ORIGIN'" }
		postbuildcommands {
			'{COPYFILE} "' .. EXTERNAL .. '/dxc/bin/libdxcompiler.so" "%{cfg.targetdir}"',
		}
	filter { "system:macosx", SHIPPING }
		links (APPLE_FRAMEWORKS)
	filter {}
end

group "External"

project "External"
	kind "StaticLib"
	location (ROOT .. "/build/projects/External")
	warnings "Off"
	includedirs { EXTERNAL }
	-- Header-only libraries are compiled by their <lib>/<lib>.cpp implementation files.
	-- Libraries only used by the editor (ufbx, xatlas, miniply) are compiled by the Editor project.
	files {
		EXTERNAL .. "/bluenoise/*.cpp",
		EXTERNAL .. "/lodepng/*.cpp", EXTERNAL .. "/lodepng/*.h",
		EXTERNAL .. "/lua/*.c", EXTERNAL .. "/lua/*.h",
		EXTERNAL .. "/meshoptimizer/*.cpp", EXTERNAL .. "/meshoptimizer/*.h",
		EXTERNAL .. "/minimp4/*.cpp", EXTERNAL .. "/minimp4/*.h",
		EXTERNAL .. "/offset_allocator/*.cpp", EXTERNAL .. "/offset_allocator/*.hpp",
		EXTERNAL .. "/pugixml/*.cpp", EXTERNAL .. "/pugixml/*.hpp",
		EXTERNAL .. "/spirv_reflect/*.c", EXTERNAL .. "/spirv_reflect/*.h",
		EXTERNAL .. "/stb/*.cpp", EXTERNAL .. "/stb/*.c", EXTERNAL .. "/stb/*.h",
		EXTERNAL .. "/zstd/zstd.cpp", EXTERNAL .. "/zstd/*.h",
	}
	removefiles {
		EXTERNAL .. "/lua/lua.c",         -- standalone interpreter
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

	filter (SHIPPING)
		kind "StaticLib"

	filter (DEV)
		kind "SharedLib"
		links { "Jolt", "External" }
	-- Windows: export every engine symbol through a generated .def file (no dllexport annotations needed)
	filter { DEV, "system:windows" }
		dependson { "DefGenerator" }
		prelinkcommands {
			'"' .. path.translate(BIN .. "/def_generator.exe", "\\") .. '" "' .. TEMP .. '/Protean.def" Protean "' .. TEMP .. '"',
		}
		linkoptions { '/DEF:"' .. TEMP .. '/Protean.def"' }
	filter { DEV, "system:linux" }
		links { "FAudio", "SDL2", "dl", "pthread" }
	filter { DEV, "system:macosx" }
		links (APPLE_FRAMEWORKS)
	filter {}

group "Tools"

-- Writes the export definitions of the engine DLL from its object files (Windows)
project "DefGenerator"
	kind "ConsoleApp"
	location (ROOT .. "/build/projects/Tools")
	targetname "def_generator"
	exceptionhandling "Default"
	rtti "Default"
	files { SOURCE .. "/tools/def_generator/**.cpp" }

project "ShaderCompiler"
	kind "ConsoleApp"
	location (ROOT .. "/build/projects/Tools")
	debugdir (SOURCE .. "/engine")
	files { SOURCE .. "/tools/shader_compiler/**.cpp" }
	UseEngine()

group "Applications"

project "Editor"
	kind "WindowedApp"
	location (ROOT .. "/build/projects/Editor")
	targetname "ProteanEditor"
	includedirs { SOURCE .. "/editor", SOURCE .. "/game" }
	files {
		SOURCE .. "/editor/*.h", SOURCE .. "/editor/*.cpp",
		EXTERNAL .. "/xatlas/xatlas.cpp", EXTERNAL .. "/miniply/miniply.cpp", -- ufbx.c is included by model_importer_fbx.cpp
	}
	removefiles { SOURCE .. "/editor/main_*" }
	pchheader "stdafx.h"
	pchsource (SOURCE .. "/editor/stdafx.cpp")
	UseEngine()
	-- Play / Pause / Stop of the game module needs the shared engine
	filter (DEV)
		defines { "PROTEAN_EDITOR_GAME_MODULE" }
		dependson { "Game" }
	filter {}
	filter "files:../external/**"
		enablepch "Off"
		warnings "Off"
	filter "system:windows"
		files { SOURCE .. "/editor/main_windows.cpp", SOURCE .. "/editor/editor.rc", SOURCE .. "/editor/editor.manifest" }
	filter "system:linux"
		files { SOURCE .. "/editor/main_sdl2.cpp" }
	filter "system:macosx"
		files { SOURCE .. "/editor/main_macos.mm" }
	filter {}

-- The game module.
--	Debug/Development: shared library (Game.dll / libGame.so) linking the engine shared library, hosted by
--	the Launcher or by the Editor (Play / Pause / Stop).
--	Shipping: standalone executable with the engine linked statically.
project "Game"
	location (ROOT .. "/build/projects/Game")
	debugdir (DATA)
	includedirs { SOURCE .. "/game" }
	files { SOURCE .. "/game/*.h", SOURCE .. "/game/*.cpp" }
	UseEngine()
	filter (DEV)
		kind "SharedLib"
		defines { "PROTEAN_GAME_DLL" }
	filter (SHIPPING)
		kind "WindowedApp"
		targetname "ProteanGame"
		files { SOURCE .. "/game/host/**", SOURCE .. "/game/main/**" }
	filter {}

project "Launcher"
	location (ROOT .. "/build/projects/Game")
	targetname "ProteanGame"
	kind "WindowedApp"
	debugdir (DATA)
	includedirs { SOURCE .. "/game" }
	files { SOURCE .. "/game/host/**", SOURCE .. "/game/launcher/**" }
	dependson { "Game" }
	UseEngine()
	-- Shipping builds the game as a standalone executable, no launcher needed
	removeconfigurations { "Shipping" }

group "Samples"

project "Tests"
	kind "WindowedApp"
	location (ROOT .. "/build/projects/Samples")
	debugdir (SOURCE .. "/samples/tests")
	files { SOURCE .. "/samples/tests/*.h", SOURCE .. "/samples/tests/*.cpp" }
	removefiles { SOURCE .. "/samples/tests/main_*", SOURCE .. "/samples/tests/custom_shader_lifecycle_test.cpp" }
	UseEngine()
	filter "system:windows"
		files { SOURCE .. "/samples/tests/main_windows.cpp", SOURCE .. "/samples/tests/tests.rc" }
	filter "system:linux"
		files { SOURCE .. "/samples/tests/main_sdl2.cpp" }
	filter {}

-- Headless unit test, run it from any directory
project "CustomShaderLifecycleTest"
	kind "ConsoleApp"
	location (ROOT .. "/build/projects/Samples")
	files { SOURCE .. "/samples/tests/custom_shader_lifecycle_test.cpp" }
	UseEngine()
