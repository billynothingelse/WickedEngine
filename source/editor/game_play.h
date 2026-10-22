#pragma once
#include "protean.h"

#ifdef PROTEAN_EDITOR_GAME_MODULE
#include "game_module.h"
#endif

// Runs the game module (Game.dll / libGame.so) inside the editor: Play / Pause / Stop.
//	Play snapshots the scene and starts the game in it, Stop ends the game, restores the snapshot and
//	unloads the module, so the game can be rebuilt while the editor keeps running.
//	Only available in Debug/Development builds, where the engine is a shared library that the editor
//	and the game module share.
class EditorGamePlay
{
public:
	enum State
	{
		State_Stopped,
		State_Playing,
		State_Paused,
	};

	static constexpr bool IsAvailable()
	{
#ifdef PROTEAN_EDITOR_GAME_MODULE
		return true;
#else
		return false;
#endif
	}

	State GetState() const { return state; }
	bool IsStopped() const { return state == State_Stopped; }
	const std::string& GetErrorMessage() const { return lastError; }

	// Starts playing, or resumes when paused. Returns false if the game module could not be loaded.
	bool Play(Protean::Scene& scene, Protean::CameraComponent& camera, Protean::RenderPath3D& renderPath);
	void Pause();
	// Ends the game and restores the scene to the state it had when Play was pressed
	void Stop();

	void Update(float dt);
	void FixedUpdate();

private:
	State state = State_Stopped;
	std::string lastError;

#ifdef PROTEAN_EDITOR_GAME_MODULE
	bool LoadModule();
	void UnloadModule();

	Protean::SharedLibrary library;
	Protean::GameModule* module = nullptr;
	ProteanDestroyGameModuleFn destroyModule = nullptr;
	Protean::Scene* scene = nullptr;
	Protean::Archive snapshot;
	bool physicsWasEnabled = false;
#endif
};
