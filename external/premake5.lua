-- Third party libraries (static), included by build/premake5.lua

project "External"
	kind "StaticLib"
	location (PROJECTS .. "/External")
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
	location (PROJECTS .. "/External")
	includedirs { EXTERNAL }
	files { EXTERNAL .. "/Jolt/**.cpp", EXTERNAL .. "/Jolt/**.h", EXTERNAL .. "/Jolt/**.inl" }
	filter "system:linux"
		buildoptions { "-Wno-stringop-overflow" }
	filter {}

if os.target() == "linux" then
project "FAudio"
	kind "StaticLib"
	language "C"
	location (PROJECTS .. "/External")
	warnings "Off"
	includedirs { EXTERNAL .. "/FAudio/include", EXTERNAL .. "/FAudio/src" }
	files { EXTERNAL .. "/FAudio/src/*.c", EXTERNAL .. "/FAudio/src/*.h", EXTERNAL .. "/FAudio/include/*.h" }
end
