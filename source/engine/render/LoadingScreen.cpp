#include "render/LoadingScreen.h"
#include "core/Application.h"
#include "core/EventHandler.h"

#include <thread>

using namespace Protean::Graphics;

namespace Protean
{

	bool LoadingScreen::isActive() const
	{
		return Protean::JobSystem::IsBusy(ctx);
	}

	bool LoadingScreen::isFinished() const
	{
		return tasks.empty() && finish == nullptr && finished;
	}

	int LoadingScreen::getProgress() const
	{
		if (launchedTasks == 0)
			return 100;
		uint32_t counter = Protean::JobSystem::GetRemainingJobCount(ctx);
		float percent = 1 - float(counter) / float(launchedTasks);
		return (int)std::round(percent * 100);
	}

	void LoadingScreen::addLoadingFunction(const std::function<void(Protean::JobSystem::JobArgs)>& loadingFunction)
	{
		if (loadingFunction != nullptr)
		{
			tasks.push_back(loadingFunction);
		}
	}

	void LoadingScreen::addLoadingComponent(RenderPath* component, Application* main, float fadeSeconds, Protean::Color fadeColor, Protean::FadeManager::FadeType fadetype)
	{
		addLoadingFunction([=](Protean::JobSystem::JobArgs args) {
			component->Load();
			});
		onFinished([=] {
			main->ActivatePath(component, fadeSeconds, fadeColor, fadetype);
			});
	}

	void LoadingScreen::onFinished(const std::function<void()>& finishFunction)
	{
		if (finishFunction != nullptr)
			finish = finishFunction;
	}

	void LoadingScreen::Start()
	{
		finished = false;
		launchedTasks = (uint32_t)tasks.size();
		for (auto& x : tasks)
		{
			Protean::JobSystem::Execute(ctx, x);
		}
		std::thread([this]() {
			Protean::JobSystem::Wait(ctx);
			Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [this](uint64_t) {
				if (finish != nullptr)
					finish();
				tasks.clear();
				launchedTasks = 0;
				finish = nullptr;
			});
		}).detach();

		RenderPath2D::Start();
	}

	void LoadingScreen::Stop()
	{
		RenderPath2D::Stop();
		finished = true;
	}

	void LoadingScreen::Compose(Protean::Graphics::CommandList cmd) const
	{
		if (backgroundTexture.IsValid())
		{
			Protean::Image::Params fx;
			const Texture& tex = backgroundTexture.GetTexture();
			const TextureDesc& desc = tex.GetDesc();

			const float canvas_aspect = GetLogicalWidth() / GetLogicalHeight();
			const float image_aspect = float(desc.width) / float(desc.height);

			switch (background_mode)
			{
			default:
			case Protean::LoadingScreen::BackgroundMode_Fill:
				if (canvas_aspect > image_aspect)
				{
					// display aspect is wider than image:
					fx.siz.x = GetLogicalWidth();
					fx.siz.y = GetLogicalHeight() / image_aspect * canvas_aspect;
				}
				else
				{
					// image aspect is wider or equal to display
					fx.siz.x = GetLogicalWidth() / canvas_aspect * image_aspect;
					fx.siz.y = GetLogicalHeight();
				}
				fx.pos = XMFLOAT3(GetLogicalWidth() * 0.5f, GetLogicalHeight() * 0.5f, 0);
				fx.pivot = XMFLOAT2(0.5f, 0.5f);
				break;
			case Protean::LoadingScreen::BackgroundMode_Fit:
				if (canvas_aspect > image_aspect)
				{
					// display aspect is wider than image:
					fx.siz.x = GetLogicalWidth() / canvas_aspect * image_aspect;
					fx.siz.y = GetLogicalHeight();
				}
				else
				{
					// image aspect is wider or equal to display
					fx.siz.x = GetLogicalWidth();
					fx.siz.y = GetLogicalHeight() * canvas_aspect / image_aspect;
				}
				fx.pos = XMFLOAT3(GetLogicalWidth() * 0.5f, GetLogicalHeight() * 0.5f, 0);
				fx.pivot = XMFLOAT2(0.5f, 0.5f);
				break;
			case Protean::LoadingScreen::BackgroundMode_Stretch:
				fx.enableFullScreen();
				break;
			}

			fx.blendFlag = Protean::BLENDMODE_ALPHA;
			if (colorspace != ColorSpace_SRGB)
			{
				fx.enableLinearOutputMapping(hdr_scaling);
			}

			Protean::Image::Draw(&tex, fx, cmd);
		}

		RenderPath2D::Compose(cmd);
	}

}
