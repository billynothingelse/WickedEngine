-- Protean workspace build script: shared settings for every project.
-- Each project lives in the premake5.lua next to its sources and is included at the bottom.
--
-- Requires premake 5.0.0 or newer (bootstrap downloads it to build/premake5[.exe]).
--
--   Windows: bootstrap.bat            (generates Protean.slnx in the repository root and builds it)
--   Linux:   ./bootstrap.sh           (generates the Makefile in the repository root and builds it)
--
-- Manual usage from this folder: premake5 vs2026 | premake5 vs2022 | premake5 gmake | premake5 xcode4
--
-- Layout:
--   Protean.slnx / Makefile             generated solution, in the repository root
--   build/projects                     generated project files
--   build/temp/<system>-<arch>-<config> intermediate object files
--   binaries/<system>-<arch>-<config>   executables and libraries, e.g. binaries/windows-x64-debug
--
-- Configurations:
--   Debug        no optimisation, game is a DLL loaded by the launcher
--   Development  optimised, game is a DLL loaded by the launcher
--   Shipping     optimised, game is a single executable with the engine linked statically

-- Globals shared with the project scripts
ROOT = path.getabsolute("..")
SOURCE = ROOT .. "/source"
EXTERNAL = ROOT .. "/external"
DATA = ROOT .. "/data"
PROJECTS = ROOT .. "/build/projects"
OUTPUT = "%{cfg.system}-%{cfg.platform}-%{cfg.buildcfg:lower()}"
BIN = ROOT .. "/binaries/" .. OUTPUT
TEMP = ROOT .. "/build/temp/" .. OUTPUT .. "/%{prj.name}"
DEV = "configurations:Debug or configurations:Development"
SHIPPING = "configurations:Shipping"

workspace "Protean"
	location (ROOT)
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

APPLE_FRAMEWORKS = {
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

-- Projects
group "External"
	include (EXTERNAL)
group "Engine"
	include (SOURCE .. "/engine")
group "Tools"
	include (SOURCE .. "/tools/def_generator")
	include (SOURCE .. "/tools/shader_compiler")
group "Applications"
	include (SOURCE .. "/editor")
	include (SOURCE .. "/game")
group "Samples"
	include (SOURCE .. "/samples/tests")
group ""
