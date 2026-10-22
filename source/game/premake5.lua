-- The game module.
--	Debug/Development: shared library (Game.dll / libGame.so) linking the engine shared library, hosted by
--	the Launcher or by the Editor (Play / Pause / Stop).
--	Shipping: standalone executable with the engine linked statically.
project "Game"
	location (PROJECTS .. "/Game")
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
	location (PROJECTS .. "/Game")
	targetname "ProteanGame"
	kind "WindowedApp"
	debugdir (DATA)
	includedirs { SOURCE .. "/game" }
	files { SOURCE .. "/game/host/**", SOURCE .. "/game/launcher/**" }
	dependson { "Game" }
	UseEngine()
	-- Shipping builds the game as a standalone executable, no launcher needed
	removeconfigurations { "Shipping" }
