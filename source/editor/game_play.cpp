#include "stdafx.h"
#include "game_play.h"

#ifdef PROTEAN_EDITOR_GAME_MODULE

bool EditorGamePlay::LoadModule()
{
	const std::string path = Protean::Helper::GetDirectoryFromPath(Protean::Helper::GetExecutablePath()) +
		Protean::SharedLibrary::GetPlatformFileName(PROTEAN_GAME_MODULE_NAME);

	if (!library.Load(path))
	{
		lastError = "Failed to load " + path + ": " + library.GetError();
		return false;
	}

	auto createModule = library.GetFunction<ProteanCreateGameModuleFn>(PROTEAN_GAME_CREATE_FUNCTION);
	destroyModule = library.GetFunction<ProteanDestroyGameModuleFn>(PROTEAN_GAME_DESTROY_FUNCTION);
	if (createModule == nullptr || destroyModule == nullptr)
	{
		lastError = "Game module entry points not found in " + path;
		library.Unload();
		return false;
	}

	module = createModule();
	if (module == nullptr)
	{
		lastError = "The game module failed to create the game";
		library.Unload();
		return false;
	}
	return true;
}

void EditorGamePlay::UnloadModule()
{
	if (module != nullptr)
	{
		destroyModule(module);
		module = nullptr;
	}
	destroyModule = nullptr;
	// The module is responsible for finishing its own jobs in OnStop, nothing may run its code after this
	library.Unload();
}

bool EditorGamePlay::Play(Protean::Scene& playScene, Protean::CameraComponent& camera, Protean::RenderPath3D& renderPath)
{
	if (state == State_Playing)
		return true;

	if (state == State_Paused)
	{
		state = State_Playing;
		Protean::Physics::SetSimulationEnabled(true);
		return true;
	}

	lastError.clear();
	if (!LoadModule())
	{
		Protean::Backlog::post(lastError, Protean::Backlog::LogLevel_Error);
		return false;
	}

	scene = &playScene;
	snapshot = Protean::Archive();
	scene->Serialize(snapshot);

	physicsWasEnabled = Protean::Physics::IsSimulationEnabled();
	Protean::Physics::SetSimulationEnabled(true);

	Protean::GameContext context;
	context.Scene = scene;
	context.Camera = &camera;
	context.RenderPath = &renderPath;
	context.InEditor = true;
	module->OnStart(context);

	state = State_Playing;
	Protean::Backlog::post("[Editor] Game started");
	return true;
}

void EditorGamePlay::Pause()
{
	if (state != State_Playing)
		return;
	state = State_Paused;
	Protean::Physics::SetSimulationEnabled(false);
}

void EditorGamePlay::Stop()
{
	if (state == State_Stopped)
		return;

	module->OnStop();
	UnloadModule();

	// Restore the scene as it was before playing
	scene->Clear();
	snapshot.SetReadModeAndResetPos(true);
	scene->Serialize(snapshot);
	snapshot = Protean::Archive();
	scene = nullptr;

	Protean::Physics::SetSimulationEnabled(physicsWasEnabled);
	state = State_Stopped;
	Protean::Backlog::post("[Editor] Game stopped, scene restored");
}

void EditorGamePlay::Update(float dt)
{
	if (state == State_Playing)
	{
		module->OnUpdate(dt);
	}
}

void EditorGamePlay::FixedUpdate()
{
	if (state == State_Playing)
	{
		module->OnFixedUpdate();
	}
}

#else

bool EditorGamePlay::Play(Protean::Scene&, Protean::CameraComponent&, Protean::RenderPath3D&)
{
	lastError = "Playing the game in the editor requires a Debug or Development build";
	Protean::Backlog::post(lastError, Protean::Backlog::LogLevel_Warning);
	return false;
}
void EditorGamePlay::Pause() {}
void EditorGamePlay::Stop() {}
void EditorGamePlay::Update(float) {}
void EditorGamePlay::FixedUpdate() {}

#endif // PROTEAN_EDITOR_GAME_MODULE
