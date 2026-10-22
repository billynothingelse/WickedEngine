#pragma once
#include "game_module.h"

namespace Protean
{
	// Render path of the standalone host, forwards the frame to the game module
	struct GameHostRenderPath : RenderPath3D
	{
		GameModule* Module = nullptr;

		void Start() override;
		void Stop() override;
		void Update(f32 dt) override;
		void FixedUpdate() override;
	};

	// Standalone application hosting a game module (Launcher in Debug/Development, Shipping executable)
	struct GameHost : Application
	{
		explicit GameHost(GameModule* module);
		void Initialize() override;

	private:
		GameHostRenderPath renderPath;
	};

	// Creates the platform window and runs the main loop until the window is closed
	i32 RunGameHost(i32 argc, char** argv, GameModule* module);
}
