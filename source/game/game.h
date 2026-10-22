#pragma once
#include "game_module.h"

namespace Protean
{
	// The game itself: sets up a small scene and animates it
	struct Game : GameModule
	{
		void OnStart(const GameContext& context) override;
		void OnStop() override;
		void OnUpdate(f32 dt) override;

	private:
		GameContext context;
		Entity cubeEntity = INVALID_ENTITY;
		Entity cameraEntity = INVALID_ENTITY;
		f32 time = 0;
	};
}
