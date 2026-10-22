#pragma once
#include "render/RenderPath2D.h"
#include "math/Color.h"
#include "core/JobSystem.h"
#include "core/Vector.h"
#include "render/FadeManager.h"

#include <functional>

namespace Protean
{

	class Application;

	class LoadingScreen :
		public RenderPath2D
	{
	protected:
		Protean::JobSystem::context ctx;
		Protean::vector<std::function<void(Protean::JobSystem::JobArgs)>> tasks;
		std::function<void()> finish;
		uint32_t launchedTasks = 0;
		bool finished = false;
	public:
		Protean::Resource backgroundTexture;

		enum BackgroundMode {
			BackgroundMode_Fill,	// fill the whole screen, will cut off parts of the image if aspects don't match
			BackgroundMode_Fit,	// fit the image completely inside the screen, will result in black bars on screen if aspects don't match
			BackgroundMode_Stretch	// fill the whole screen, and stretch the image if needed
		} background_mode = BackgroundMode_Fill;

		//Add a loading task which should be executed
		void addLoadingFunction(const std::function<void(Protean::JobSystem::JobArgs)>& loadingFunction);
		//Helper for loading a whole renderable component
		void addLoadingComponent(RenderPath* component, Application* main, float fadeSeconds = 0, Protean::Color fadeColor = Protean::Color(0, 0, 0, 255), Protean::FadeManager::FadeType fadetype = Protean::FadeManager::FadeType_FadeToColor);
		//Set a function that should be called when the loading finishes
		void onFinished(const std::function<void()>& finishFunction);
		//See if the loading is currently running
		bool isActive() const;
		// See if there are any loading tasks that are still not finished
		bool isFinished() const;
		// Returns the percentage of loading tasks that are finished (0% - 100%)
		int getProgress() const;

		//Start Executing the tasks and mark the loading as active
		void Start() override;

		// Stop called when swapped out usually at end of loading
		void Stop() override;

		void Compose(Protean::Graphics::CommandList cmd) const override;

		// This is an identifier of RenderPath subtype that is used for lua binding.
		static constexpr const auto script_check_identifier = relative_path_storage(__FILE__);
		const char* GetScriptBindingID() const override { return script_check_identifier.c_str(); }
	};

}
