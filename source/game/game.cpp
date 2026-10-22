#include "game.h"
#include "game_module.h"

namespace Protean
{
	void GameRenderPath::Start()
	{
		RenderPath3D::Start();

		Scene& scene = GetScene();
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

		cubeEntity = scene.Entity_CreateCube("Cube");

		cameraEntity = CreateEntity();
		scene.transforms.Create(cameraEntity);
		if (TransformComponent* transform = scene.transforms.GetComponent(cameraEntity))
		{
			transform->Translate(XMFLOAT3(0, 3, -8));
			transform->RotateRollPitchYaw(XMFLOAT3(0.3f, 0, 0));
			transform->UpdateTransform();
		}
	}

	void GameRenderPath::Update(f32 dt)
	{
		time += dt;

		Scene& scene = GetScene();
		if (TransformComponent* transform = scene.transforms.GetComponent(cubeEntity))
		{
			transform->ClearTransform();
			transform->Translate(XMFLOAT3(0, 1.5f + std::sin(time) * 0.25f, 0));
			transform->RotateRollPitchYaw(XMFLOAT3(0, time, 0));
			transform->UpdateTransform();
		}

		if (const TransformComponent* transform = scene.transforms.GetComponent(cameraEntity))
		{
			CameraComponent& camera = GetCamera();
			camera.TransformCamera(*transform);
			camera.UpdateCamera();
		}

		RenderPath3D::Update(dt);
	}

	void Game::Initialize()
	{
		Application::Initialize();

		infoDisplay.active = true;
		infoDisplay.watermark = true;
		infoDisplay.resolution = true;
		infoDisplay.fpsinfo = true;

		ActivatePath(&renderPath);
	}
}

PROTEAN_GAME_API int ProteanGameMain(int argc, char** argv)
{
	return Protean::RunGame(argc, argv);
}
