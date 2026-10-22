-- The engine, included by build/premake5.lua
--	Debug/Development: shared library (Protean.dll / libProtean.so), shared by the editor, the launcher and the
--	game module. Shipping: static library.

project "Engine"
	location (PROJECTS .. "/Engine")
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
