#include "game.h"

namespace Protean
{
	void Game::OnStart(const GameContext& gameContext)
	{
		context = gameContext;
		time = 0;
		Scene& scene = *context.Scene;

		// In the editor the game plays inside the scene that is being edited,
		// standalone it starts from an empty scene and builds its own
		if (!context.InEditor)
		{
			scene.weather.ambient = XMFLOAT3(0.2f, 0.2f, 0.25f);

			Entity sun = scene.Entity_CreateLight("Sun", XMFLOAT3(0, 10, 0), XMFLOAT3(1, 0.95f, 0.9f), 8, 1000, LightComponent::DIRECTIONAL);
			if (TransformComponent* transform = scene.transforms.GetComponent(sun))
			{
				transform->RotateRollPitchYaw(XMFLOAT3(XM_PIDIV4, 0, XM_PIDIV4));
				transform->UpdateTransform();
			}
			if (LightComponent* light = scene.lights.GetComponent(sun))
			{
				light->SetCastShadow(true);
			}

			Entity ground = scene.Entity_CreatePlane("Ground");
			if (TransformComponent* transform = scene.transforms.GetComponent(ground))
			{
				transform->Scale(XMFLOAT3(20, 1, 20));
				transform->UpdateTransform();
			}

			cameraEntity = CreateEntity();
			scene.transforms.Create(cameraEntity);
			if (TransformComponent* transform = scene.transforms.GetComponent(cameraEntity))
			{
				transform->Translate(XMFLOAT3(0, 3, -8));
				transform->RotateRollPitchYaw(XMFLOAT3(0.3f, 0, 0));
				transform->UpdateTransform();
			}
		}

		cubeEntity = scene.Entity_CreateCube("Game Cube");
	}

	void Game::OnStop()
	{
		cubeEntity = INVALID_ENTITY;
		cameraEntity = INVALID_ENTITY;
	}

	void Game::OnUpdate(f32 dt)
	{
		time += dt;
		Scene& scene = *context.Scene;

		if (TransformComponent* transform = scene.transforms.GetComponent(cubeEntity))
		{
			transform->ClearTransform();
			transform->Translate(XMFLOAT3(0, 1.5f + std::sin(time) * 0.25f, 0));
			transform->RotateRollPitchYaw(XMFLOAT3(0, time, 0));
			transform->UpdateTransform();
		}

		// The editor keeps control of its own camera
		if (!context.InEditor && context.Camera != nullptr)
		{
			if (const TransformComponent* transform = scene.transforms.GetComponent(cameraEntity))
			{
				context.Camera->TransformCamera(*transform);
				context.Camera->UpdateCamera();
			}
		}
	}
}

PROTEAN_GAME_API Protean::GameModule* ProteanCreateGameModule()
{
	return new Protean::Game;
}

PROTEAN_GAME_API void ProteanDestroyGameModule(Protean::GameModule* module)
{
	delete module;
}
