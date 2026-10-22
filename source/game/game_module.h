#pragma once
#include "protean.h"

// Contract between the game module and whatever hosts it.
//
//	Debug / Development: the game is a shared library (Game.dll / libGame.so) that links the engine
//	                     shared library. It is hosted by the Launcher (standalone) or by the Editor
//	                     (Play / Pause / Stop), all of them share the single engine instance.
//	Shipping:            the game is linked into a standalone executable together with the engine.

#if defined(PROTEAN_GAME_DLL)
#if defined(_WIN32)
#define PROTEAN_GAME_API extern "C" __declspec(dllexport)
#else
#define PROTEAN_GAME_API extern "C" __attribute__((visibility("default")))
#endif
#else
#define PROTEAN_GAME_API extern "C"
#endif

#define PROTEAN_GAME_MODULE_NAME "Game"
#define PROTEAN_GAME_CREATE_FUNCTION "ProteanCreateGameModule"
#define PROTEAN_GAME_DESTROY_FUNCTION "ProteanDestroyGameModule"

namespace Protean
{
	// Everything the game gets from its host when play starts
	struct GameContext
	{
		Protean::Scene* Scene = nullptr;
		Protean::CameraComponent* Camera = nullptr;
		Protean::RenderPath3D* RenderPath = nullptr;
		bool InEditor = false;
	};

	// Implemented by the game. The host calls OnStart when play begins, OnUpdate / OnFixedUpdate every
	// frame while playing (not while paused) and OnStop when play ends. In the editor the scene is
	// restored after OnStop, so the game does not need to undo its scene changes, but it must release
	// anything it registered with the engine (event subscriptions, callbacks) because the module can be
	// unloaded afterwards.
	struct GameModule
	{
		virtual ~GameModule() = default;

		virtual void OnStart(const GameContext& context) {}
		virtual void OnStop() {}
		virtual void OnUpdate(f32 dt) {}
		virtual void OnFixedUpdate() {}
	};
}

typedef Protean::GameModule* (*ProteanCreateGameModuleFn)();
typedef void (*ProteanDestroyGameModuleFn)(Protean::GameModule* module);

PROTEAN_GAME_API Protean::GameModule* ProteanCreateGameModule();
PROTEAN_GAME_API void ProteanDestroyGameModule(Protean::GameModule* module);
