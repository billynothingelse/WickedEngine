#pragma once
#include "Protean.h"

namespace Protean
{
	// The game's 3D view: owns the scene setup and per-frame game logic.
	struct GameRenderPath : RenderPath3D
	{
		void Start() override;
		void Update(f32 dt) override;

	private:
		Entity cameraEntity = INVALID_ENTITY;
		Entity cubeEntity = INVALID_ENTITY;
		f32 time = 0;
	};

	struct Game : Application
	{
		void Initialize() override;

	private:
		GameRenderPath renderPath;
	};

	// Creates the platform window and runs the main loop.
	i32 RunGame(i32 argc, char** argv);
}
