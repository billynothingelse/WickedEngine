-- The editor
project "Editor"
	kind "WindowedApp"
	location (PROJECTS .. "/Editor")
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
	filter "files:../../external/**" -- file filters are relative to this script
		enablepch "Off"
		warnings "Off"
	filter "system:windows"
		files { SOURCE .. "/editor/main_windows.cpp", SOURCE .. "/editor/editor.rc", SOURCE .. "/editor/editor.manifest" }
	filter "system:linux"
		files { SOURCE .. "/editor/main_sdl2.cpp" }
	filter "system:macosx"
		files { SOURCE .. "/editor/main_macos.mm" }
	filter {}
