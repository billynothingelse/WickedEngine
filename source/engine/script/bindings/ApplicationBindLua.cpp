#include "script/bindings/ApplicationBindLua.h"
#include "script/bindings/RenderPathBindLua.h"
#include "script/bindings/RenderPath3DBindLua.h"
#include "script/bindings/RenderPath2DBindLua.h"
#include "script/bindings/LoadingScreenBindLua.h"
#include "debug/Profiler.h"
#include "platform/Platform.h"

namespace Protean::Lua
{


	Luna<Application_BindLua>::FunctionType Application_BindLua::methods[] = {
		lunamethod(Application_BindLua, GetActivePath),
		lunamethod(Application_BindLua, SetActivePath),
		lunamethod(Application_BindLua, SetFrameSkip),
		lunamethod(Application_BindLua, SetFullScreen),
		lunamethod(Application_BindLua, SetTargetFrameRate),
		lunamethod(Application_BindLua, SetFrameRateLock),
		lunamethod(Application_BindLua, SetInfoDisplay),
		lunamethod(Application_BindLua, SetWatermarkDisplay),
		lunamethod(Application_BindLua, SetFPSDisplay),
		lunamethod(Application_BindLua, SetResolutionDisplay),
		lunamethod(Application_BindLua, SetLogicalSizeDisplay),
		lunamethod(Application_BindLua, SetColorSpaceDisplay),
		lunamethod(Application_BindLua, SetPipelineCountDisplay),
		lunamethod(Application_BindLua, SetHeapAllocationCountDisplay),
		lunamethod(Application_BindLua, SetVRAMUsageDisplay),
		lunamethod(Application_BindLua, SetColorGradingHelper),
		lunamethod(Application_BindLua, IsHDRSupported),
		lunamethod(Application_BindLua, SetHDR),
		lunamethod(Application_BindLua, GetCanvas),
		lunamethod(Application_BindLua, SetCanvas),
		lunamethod(Application_BindLua, Exit),
		lunamethod(Application_BindLua, IsFaded),
		lunamethod(Application_BindLua, Fade),
		lunamethod(Application_BindLua, CrossFade),
		{ NULL, NULL }
	};
	Luna<Application_BindLua>::PropertyType Application_BindLua::properties[] = {
		{ NULL, NULL }
	};

	Application_BindLua::Application_BindLua(Application* component) :component(component)
	{
	}

	Application_BindLua::Application_BindLua(lua_State* L)
	{
		component = nullptr;
	}

	int Application_BindLua::GetActivePath(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "GetActivePath() component is empty!");
			return 0;
		}

		Protean::RenderPath* renderpath = component->GetActivePath();
		if (renderpath == nullptr)
		{
			Protean::Lua::SError(L, "GetActivePath() returned null! No path was registered yet!");
			return 0;
		}

		//return 3d component if the active one is of that type
		if (renderpath->GetScriptBindingID() == Protean::RenderPath3D::script_check_identifier)
		{
			Luna<RenderPath3D_BindLua>::push(L, (Protean::RenderPath3D*)renderpath);
			return 1;
		}

		//return loading component if the active one is of that type
		if (renderpath->GetScriptBindingID() == Protean::LoadingScreen::script_check_identifier)
		{
			Luna<LoadingScreen_BindLua>::push(L, (Protean::LoadingScreen*)renderpath);
			return 1;
		}

		//return 2d component if the active one is of that type
		if (renderpath->GetScriptBindingID() == Protean::RenderPath2D::script_check_identifier)
		{
			Luna<RenderPath2D_BindLua>::push(L, (Protean::RenderPath2D*)renderpath);
			return 1;
		}

		//return component if the active one is of that type
		if (renderpath->GetScriptBindingID() == Protean::RenderPath::script_check_identifier)
		{
			Luna<RenderPath_BindLua>::push(L, (Protean::RenderPath*)renderpath);
			return 1;
		}

		Protean::Lua::SError(L, "GetActivePath() Warning: type of active component not registered!");
		return 0;
	}
	int Application_BindLua::SetActivePath(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetActivePath(RenderPath component) component is empty!");
			return 0;
		}

		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			float fadeSeconds = 0;
			Protean::Color fadeColor = Protean::Color(0, 0, 0, 255);
			Protean::FadeManager::FadeType fadetype = Protean::FadeManager::FadeType_FadeToColor;
			if (argc > 1)
			{
				fadeSeconds = Protean::Lua::SGetFloat(L, 2);
				if (argc > 2)
				{
					fadeColor.setR((uint8_t)Protean::Lua::SGetInt(L, 3));
					if (argc > 3)
					{
						fadeColor.setG((uint8_t)Protean::Lua::SGetInt(L, 4));
						if (argc > 4)
						{
							fadeColor.setB((uint8_t)Protean::Lua::SGetInt(L, 5));
							if (argc > 5)
							{
								fadetype = (Protean::FadeManager::FadeType)Protean::Lua::SGetInt(L, 6);
							}
						}
					}
				}
			}

			RenderPath3D_BindLua* comp3D = Luna<RenderPath3D_BindLua>::lightcheck(L, 1);
			if (comp3D != nullptr)
			{
				component->ActivatePath(comp3D->component, fadeSeconds, fadeColor, fadetype);
				return 0;
			}

			LoadingScreen_BindLua* compLoad = Luna<LoadingScreen_BindLua>::lightcheck(L, 1);
			if (compLoad != nullptr)
			{
				component->ActivatePath(compLoad->component, fadeSeconds, fadeColor, fadetype);
				return 0;
			}

			RenderPath2D_BindLua* comp2D = Luna<RenderPath2D_BindLua>::lightcheck(L, 1);
			if (comp2D != nullptr)
			{
				component->ActivatePath(comp2D->component, fadeSeconds, fadeColor, fadetype);
				return 0;
			}

			RenderPath_BindLua* comp = Luna<RenderPath_BindLua>::lightcheck(L, 1);
			if (comp != nullptr)
			{
				component->ActivatePath(comp->component, fadeSeconds, fadeColor, fadetype);
				return 0;
			}

			// If first param is neither, it can be nil to unset path:
			component->ActivatePath(nullptr);
		}
		else
		{
			Protean::Lua::SError(L, "SetActivePath(RenderPath component, opt int fadeFrames,fadeColorR,fadeColorG,fadeColorB) not enought arguments!");
			return 0;
		}
		return 0;
	}
	int Application_BindLua::SetFrameSkip(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFrameSkip(bool enabled) component is empty!");
			return 0;
		}

		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->setFrameSkip(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetFrameSkip(bool enabled) not enought arguments!");
		return 0;
	}
	int Application_BindLua::SetFullScreen(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFullScreen(bool enabled) component is empty!");
			return 0;
		}

		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->SetFullScreen(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetFullScreen(bool enabled) not enought arguments!");
		return 0;
	}
	int Application_BindLua::SetTargetFrameRate(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetTargetFrameRate(float value) component is empty!");
			return 0;
		}

		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->setTargetFrameRate(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetTargetFrameRate(float value) not enought arguments!");
		return 0;
	}
	int Application_BindLua::SetFrameRateLock(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFrameRateLock(bool enabled) component is empty!");
			return 0;
		}

		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->setFrameRateLock(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetFrameRateLock(bool enabled) not enought arguments!");
		return 0;
	}
	int Application_BindLua::SetInfoDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetInfoDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.active = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetInfoDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetWatermarkDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetWatermarkDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.watermark = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetWatermarkDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetFPSDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFPSDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.fpsinfo = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetFPSDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetResolutionDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetResolutionDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.resolution = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetResolutionDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetLogicalSizeDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetLogicalSizeDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.logical_size = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetLogicalSizeDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetColorSpaceDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetColorSpaceDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.colorspace = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetColorSpaceDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetPipelineCountDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetPipelineCountDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.pipeline_count = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetPipelineCountDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetHeapAllocationCountDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetHeapAllocationCountDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.heap_allocation_counter = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetHeapAllocationCountDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetVRAMUsageDisplay(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetVRAMUsageDisplay() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.vram_usage = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetVRAMUsageDisplay(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::SetColorGradingHelper(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetColorGradingHelper() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->infoDisplay.colorgrading_helper = Protean::Lua::SGetBool(L, 1);
		}
		else
			Protean::Lua::SError(L, "SetColorGradingHelper(bool active) not enough arguments!");
		return 0;
	}
	int Application_BindLua::Fade(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "Fade(float fadeSeconds = 1, opt int fadeColorR = 0, fadeColorG = 0, fadeColorB = 0, FadeType fadetype = FadeType.FadeToColor) component is empty!");
			return 0;
		}

		float fadeSeconds = 1;
		Protean::Color fadeColor = Protean::Color(0, 0, 0, 255);
		Protean::FadeManager::FadeType fadetype = Protean::FadeManager::FadeType_FadeToColor;

		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			fadeSeconds = Protean::Lua::SGetFloat(L, 1);
			if (argc > 1)
			{
				fadeColor.setR((uint8_t)Protean::Lua::SGetInt(L, 2));
				if (argc > 2)
				{
					fadeColor.setG((uint8_t)Protean::Lua::SGetInt(L, 3));
					if (argc > 3)
					{
						fadeColor.setB((uint8_t)Protean::Lua::SGetInt(L, 4));
						if (argc > 4)
						{
							fadetype = (Protean::FadeManager::FadeType)Protean::Lua::SGetInt(L, 5);
						}
					}
				}
			}
		}

		component->Fade(fadeSeconds, fadeColor, fadetype);
		return 0;
	}
	int Application_BindLua::CrossFade(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "CrossFade(float fadeSeconds = 1) component is empty!");
			return 0;
		}

		float fadeSeconds = 1;

		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			fadeSeconds = Protean::Lua::SGetFloat(L, 1);
		}

		component->Fade(fadeSeconds, Protean::Color::Black(), Protean::FadeManager::FadeType_CrossFade);
		return 0;
	}

	int Application_BindLua::IsHDRSupported(lua_State* L)
	{
		Protean::Lua::SSetBool(L, Protean::Graphics::GetDevice()->IsSwapChainSupportsHDR(&component->swapChain));
		return 1;
	}
	int Application_BindLua::SetHDR(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetHDR() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			component->allow_hdr = Protean::Lua::SGetBool(L, 1);
			component->swapChain.desc.allow_hdr = component->allow_hdr;
			bool success = Protean::Graphics::GetDevice()->CreateSwapChain(&component->swapChain.desc, nullptr, &component->swapChain);
			assert(success);
		}
		else
			Protean::Lua::SError(L, "SetHDR(bool active) not enough arguments!");
		return 0;
	}

	int Application_BindLua::GetCanvas(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "GetCanvas() component is empty!");
			return 0;
		}
		Luna<Canvas_BindLua>::push(L, component->canvas);
		return 1;
	}
	int Application_BindLua::SetCanvas(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetCanvas() component is empty!");
			return 0;
		}
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Canvas_BindLua* canvas = Luna<Canvas_BindLua>::lightcheck(L, 1);
			if (canvas != nullptr)
			{
				component->canvas = canvas->canvas;
			}
			else
			{
				Protean::Lua::SError(L, "SetCanvas(canvas canvas) first parameter is not a Canvas!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetCanvas(canvas canvas) not enough arguments!");
		}
		return 1;
	}

	int Application_BindLua::Exit(lua_State* L)
	{
		if (component)
		{
			component->Exit();
			return 0;
		}

		return 1;
	}
	int Application_BindLua::IsFaded(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "IsFaded() component is empty!");
			return 0;
		}
		Protean::Lua::SSetBool(L, component->IsFaded());
		return 1;
	}


	int SetProfilerEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Profiler::SetEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetProfilerEnabled(bool active) not enough arguments!");

		return 0;
	}
	int prof(lua_State* L)
	{
		Protean::Profiler::SetEnabled(!Protean::Profiler::IsEnabled());
		return 0;
	}
	int exit(lua_State* L)
	{
		lua_getglobal(L, "application");
		Application_BindLua* app = Luna<Application_BindLua>::lightcheck(L, -1);
		if (app && app->component)
		{
			app->component->Exit();
		}
		return 0;
	}

	void Application_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Application_BindLua>::Register(Protean::Lua::GetLuaState());

			Protean::Lua::RegisterFunc("SetProfilerEnabled", SetProfilerEnabled);
			Protean::Lua::RegisterFunc("prof", prof);
			Protean::Lua::RegisterFunc("exit", exit);

			Protean::Lua::RunText(R"(
FadeType = {
	FadeToColor = 0,
	CrossFade = 1,
}
)");

		}
	}











	Luna<Canvas_BindLua>::FunctionType Canvas_BindLua::methods[] = {
		lunamethod(Canvas_BindLua, GetDPI),
		lunamethod(Canvas_BindLua, GetDPIScaling),
		lunamethod(Canvas_BindLua, GetCustomScaling),
		lunamethod(Canvas_BindLua, SetCustomScaling),
		lunamethod(Canvas_BindLua, GetPhysicalWidth),
		lunamethod(Canvas_BindLua, GetPhysicalHeight),
		lunamethod(Canvas_BindLua, GetLogicalWidth),
		lunamethod(Canvas_BindLua, GetLogicalHeight),
		{ NULL, NULL }
	};
	Luna<Canvas_BindLua>::PropertyType Canvas_BindLua::properties[] = {
		{ NULL, NULL }
	};


	int Canvas_BindLua::GetDPI(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, canvas.GetDPI());
		return 1;
	}
	int Canvas_BindLua::GetDPIScaling(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, canvas.GetDPIScaling());
		return 1;
	}
	int Canvas_BindLua::GetCustomScaling(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, canvas.scaling);
		return 1;
	}
	int Canvas_BindLua::SetCustomScaling(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			canvas.scaling = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetCustomScaling(float scaling) not enough arguments!");
		}
		return 0;
	}
	int Canvas_BindLua::GetPhysicalWidth(lua_State* L)
	{
		Protean::Lua::SSetInt(L, canvas.GetPhysicalWidth());
		return 1;
	}
	int Canvas_BindLua::GetPhysicalHeight(lua_State* L)
	{
		Protean::Lua::SSetInt(L, canvas.GetPhysicalHeight());
		return 1;
	}
	int Canvas_BindLua::GetLogicalWidth(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, canvas.GetLogicalWidth());
		return 1;
	}
	int Canvas_BindLua::GetLogicalHeight(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, canvas.GetLogicalHeight());
		return 1;
	}

	void Canvas_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Canvas_BindLua>::Register(Protean::Lua::GetLuaState());
		}
	}

}
