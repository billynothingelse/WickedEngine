#include "core/Application.h"
#include "render/RenderPath.h"
#include "render/Renderer.h"
#include "core/Helper.h"
#include "core/Timer.h"
#include "input/Input.h"
#include "debug/Backlog.h"
#include "script/bindings/ApplicationBindLua.h"
#include "core/Version.h"
#include "render/Enums.h"
#include "render/TextureHelper.h"
#include "debug/Profiler.h"
#include "core/Initializer.h"
#include "core/Arguments.h"
#include "render/Font.h"
#include "render/Image.h"
#include "core/EventHandler.h"
#include "platform/Platform.h"

#if defined(PLATFORM_PS5)
#include "wiGraphicsDevice_PS5.h"
#elif defined(PLATFORM_APPLE)
#include "render/GraphicsAPI/Metal/MetalDevice.h"
#else
#include "render/GraphicsAPI/D3D12/D3D12Device.h"
#include "render/GraphicsAPI/Vulkan/VulkanDevice.h"
#endif // PLATFORM_PS5

#include <string>
#include <algorithm>
#include <new>
#include <cstdlib>
#include <atomic>

//#define WICKED_ENGINE_HEAP_ALLOCATION_COUNTER

#ifdef WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
static std::atomic<uint32_t> number_of_heap_allocations{ 0 };
static std::atomic<size_t> size_of_heap_allocations{ 0 };
#endif // WICKED_ENGINE_HEAP_ALLOCATION_COUNTER

using namespace Protean::Graphics;

namespace Protean
{
	void Application::Initialize()
	{
		if (initialized)
		{
			return;
		}
		initialized = true;

		Protean::Initializer::InitializeComponentsAsync();

		alwaysactive = Protean::Arguments::HasArgument("alwaysactive");

		// Note: lua is always initialized immediately on main thread by Protean::Initializer, so this is safe to do:
		assert(Protean::Initializer::IsInitializeFinished(Protean::Initializer::INITIALIZED_SYSTEM_LUA));
		Luna<Protean::Lua::Application_BindLua>::push_global(Protean::Lua::GetLuaState(), "main", this);
		Luna<Protean::Lua::Application_BindLua>::push_global(Protean::Lua::GetLuaState(), "application", this);
	}

	void Application::ActivatePath(RenderPath* component, float fadeSeconds, Protean::Color fadeColor, FadeManager::FadeType fadetype)
	{
		if (component != nullptr)
		{
			component->init(canvas);
		}

		// Fade manager will activate on fadeout
		fadeManager.Start(fadeSeconds, fadeColor, [this, component]() {

			if (activePath != nullptr)
			{
				activePath->Stop();
			}

			if (component != nullptr)
			{
				component->Start();
			}
			activePath = component;
		}, fadetype);

		fadeManager.Update(0); // If user calls ActivatePath without fadeout, it will be instant
	}

	void Application::Run()
	{
		if (!initialized)
		{
			// Initialize in a lazy way, so the user application doesn't have to call this explicitly
			Initialize();
			initialized = true;
		}

		Protean::Font::UpdateAtlas(canvas.GetDPIScaling());

		ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
		if (colorspace == ColorSpace_HDR10_ST2084)
		{
			// In HDR10, we perform the compositing in a custom linear color space render target
			//	The reason is that blending doesn't look good in HDR10 color space
			//	In HDR10 the composition is done like:
			//	rendertargetPreHDR10:
			//		- RenderPath3D: linear space
			//		- RenderPath2D: SRGB space -> linear space with HDR scaling
			//	swapChain:
			//		- HDR10 composition: linear -> HDR10_ST2084
			if (!rendertargetPreHDR10.IsValid())
			{
				TextureDesc desc;
				desc.width = swapChain.desc.width;
				desc.height = swapChain.desc.height;
				desc.format = Format_R11G11B10_FLOAT;
				desc.bind_flags = BindFlag_RENDER_TARGET | BindFlag_SHADER_RESOURCE;
				// try to set background color for swapchain color as if it's using hdr scaling:
				desc.clear.color[0] = swapChain.desc.clear_color[0] * 9;
				desc.clear.color[1] = swapChain.desc.clear_color[1] * 9;
				desc.clear.color[2] = swapChain.desc.clear_color[2] * 9;
				desc.clear.color[3] = swapChain.desc.clear_color[3];
				bool success = graphicsDevice->CreateTexture(&desc, nullptr, &rendertargetPreHDR10);
				assert(success);
				graphicsDevice->SetName(&rendertargetPreHDR10, "Application::rendertargetPreHDR10");
			}
		}
		else
		{
			// If swapchain is SRGB or Linear HDR, it can be used for blending and rendertargetPreHDR10 is not needed
			//	- If it is SRGB, the render path will ensure tonemapping to SDR
			//	- If it is Linear HDR, we can blend trivially in linear space
			rendertargetPreHDR10 = {};
		}

		if (!Protean::Initializer::IsInitializeFinished())
		{
			// Until engine is not loaded, present initialization screen...
			CommandList cmd = graphicsDevice->BeginCommandList();
			if (rendertargetPreHDR10.IsValid())
			{
				graphicsDevice->RenderPassBegin(&rendertargetPreHDR10, cmd, true);
			}
			else
			{
				graphicsDevice->RenderPassBegin(&swapChain, cmd);
			}
			Viewport viewport;
			viewport.width = (float)swapChain.desc.width;
			viewport.height = (float)swapChain.desc.height;
			graphicsDevice->BindViewports(1, &viewport, cmd);
			static bool splash_screen_check = false;
			if (!splash_screen.IsValid() && !splash_screen_check)
			{
				splash_screen_check = true;
				std::string splash_screen_path = Protean::Helper::GetCurrentPath() + "/splash_screen.png";
				if (Protean::Helper::FileExists(splash_screen_path))
				{
					Protean::Resource resource = Protean::ResourceManager::Load(splash_screen_path);
					if (resource.IsValid())
					{
						splash_screen = resource.GetTexture();
						splash_screen_subresource = graphicsDevice->CreateSubresource(&splash_screen, SubresourceType_SRV, 0, 1, 0, 1); // only first mip! mipgen is not initialized at this point...
					}
				}
			}
			if (splash_screen.IsValid())
			{
				if (Protean::Initializer::IsInitializeFinished(Protean::Initializer::INITIALIZED_SYSTEM_IMAGE))
				{
					// Draw the splash screen while engine is initializing and image renderer is ready
					Protean::Image::SetCanvas(canvas);
					Protean::Image::Params fx;
					const TextureDesc& desc = splash_screen.GetDesc();
					const float canvas_aspect = canvas.GetLogicalWidth() / canvas.GetLogicalHeight();
					const float image_aspect = float(desc.width) / float(desc.height);
					if (canvas_aspect > image_aspect)
					{
						// display aspect is wider than image:
						fx.siz.x = canvas.GetLogicalWidth();
						fx.siz.y = canvas.GetLogicalHeight() / image_aspect * canvas_aspect;
					}
					else
					{
						// image aspect is wider or equal to display
						fx.siz.x = canvas.GetLogicalWidth() / canvas_aspect * image_aspect;
						fx.siz.y = canvas.GetLogicalHeight();
					}
					fx.pos = XMFLOAT3(canvas.GetLogicalWidth() * 0.5f, canvas.GetLogicalHeight() * 0.5f, 0);
					fx.pivot = XMFLOAT2(0.5f, 0.5f);
					if (colorspace != ColorSpace_SRGB)
					{
						fx.enableLinearOutputMapping(9);
					}
					fx.image_subresource = splash_screen_subresource;
					Protean::Image::Draw(&splash_screen, fx, cmd);
				}
			}
			else if (Protean::Initializer::IsInitializeFinished(Protean::Initializer::INITIALIZED_SYSTEM_FONT))
			{
				// If there is no splash screen, the log is rendered while engine is initializing
				ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
				Protean::Backlog::DrawOutputText(canvas, cmd, colorspace);
			}
			graphicsDevice->RenderPassEnd(cmd);

			if (rendertargetPreHDR10.IsValid() && Protean::Initializer::IsInitializeFinished(Protean::Initializer::INITIALIZED_SYSTEM_IMAGE))
			{
				// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
				graphicsDevice->RenderPassBegin(&swapChain, cmd);
				Protean::Image::Params fx;
				fx.enableFullScreen();
				fx.enableHDR10OutputMapping(); // this is doing the linear -> HDR10_ST2084 conversion
				Protean::Image::Draw(&rendertargetPreHDR10, fx, cmd);
				graphicsDevice->RenderPassEnd(cmd);
			}

			graphicsDevice->SubmitCommandLists();
			return;
		}

		splash_screen = {}; // splash screen no longer needed after initialization, it is deleted

		static bool startup_script = false;
		if (!startup_script)
		{
			startup_script = true;
			const std::string workingdir = Protean::Helper::GetCurrentPath() + "/";
			const std::string rewriteable_script_filename = workingdir + rewriteable_startup_script_text;
			if (Protean::Helper::FileExists(rewriteable_script_filename))
			{
				if (Protean::Lua::RunFile(rewriteable_script_filename))
				{
					Protean::Backlog::post("Executed startup file: " + rewriteable_script_filename);
				}
			}
			else
			{
				const std::string startup_lua_filename = workingdir + "startup.lua";
				if (Protean::Helper::FileExists(startup_lua_filename))
				{
					if (Protean::Lua::RunFile(startup_lua_filename))
					{
						Protean::Backlog::post("Executed startup file: " + startup_lua_filename);
					}
				}
				const std::string startup_luab_filename = workingdir + "startup.luab";
				if (Protean::Helper::FileExists(startup_luab_filename))
				{
					if (Protean::Lua::RunBinaryFile(startup_luab_filename))
					{
						Protean::Backlog::post("Executed startup file: " + startup_luab_filename);
					}
				}
			}
		}

		if (!is_window_active && !alwaysactive)
		{
			// If the application is not active, disable Update loops:
			deltaTimeAccumulator = 0;
			Protean::Helper::Sleep(10);
			Protean::Input::Update(window, canvas); // update input while inactive, this solves a problem with past inputs processed immediately after activation
			timer.record_elapsed_seconds(); // after application becomes active, delta time shouldn't spike, could blow up gameplay or physics
			return;
		}

		Protean::Profiler::BeginFrame();

		deltaTime = float(timer.record_elapsed_seconds());

		const float target_deltaTime = 1.0f / targetFrameRate;
		if (framerate_lock && deltaTime < target_deltaTime)
		{
			Protean::Helper::QuickSleep((target_deltaTime - deltaTime) * 1000);
			deltaTime += float(timer.record_elapsed_seconds());
		}

		// avoid instability caused by large delta time
		deltaTime = clamp(deltaTime, 0.0f, 0.5f);

		Protean::Input::Update(window, canvas);

		// Wake up the events that need to be executed on the main thread, in thread safe manner:
		Protean::EventHandler::FireEvent(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, 0);

		fadeManager.Update(deltaTime);

		if (activePath != nullptr)
		{
			ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
			activePath->colorspace = colorspace;
			activePath->init(canvas);
			activePath->PreUpdate();
		}

		// Fixed time update:
		auto range = Protean::Profiler::BeginRangeCPU("Fixed Update");
		{
			if (frameskip)
			{
				deltaTimeAccumulator += deltaTime;
				if (deltaTimeAccumulator > 10)
				{
					// application probably lost control, fixed update would take too long
					deltaTimeAccumulator = 0;
				}

				const float targetFrameRateInv = 1.0f / targetFrameRate;
				while (deltaTimeAccumulator >= targetFrameRateInv)
				{
					FixedUpdate();
					deltaTimeAccumulator -= targetFrameRateInv;
				}
			}
			else
			{
				FixedUpdate();
			}
		}
		Protean::Profiler::EndRange(range); // Fixed Update

		// Variable-timed update:
		Update(deltaTime);

		Render();

		// Begin final compositing:
		CommandList cmd = graphicsDevice->BeginCommandList();

		// CrossFade texture save:
		if (fadeManager.crossFadeTextureSaveRequired)
		{
			Texture backbuffer = rendertargetPreHDR10.IsValid() ? rendertargetPreHDR10 : graphicsDevice->GetBackBuffer(&swapChain);
			if (
				fadeManager.crossFadeTexture.desc.width != backbuffer.desc.width ||
				fadeManager.crossFadeTexture.desc.height != backbuffer.desc.height ||
				fadeManager.crossFadeTexture.desc.format != backbuffer.desc.format
				)
			{
				TextureDesc desc = backbuffer.desc;
				desc.bind_flags = BindFlag_SHADER_RESOURCE;
				bool success = graphicsDevice->CreateTexture(&desc, nullptr, &fadeManager.crossFadeTexture);
				assert(success);
				graphicsDevice->SetName(&fadeManager.crossFadeTexture, "wiFadeManager::crossFadeTexture");
			}
			Protean::Renderer::PushBarrier(GPUBarrier::Image(&backbuffer, backbuffer.desc.layout, ResourceState_COPY_SRC));
			Protean::Renderer::PushBarrier(GPUBarrier::Image(&fadeManager.crossFadeTexture, fadeManager.crossFadeTexture.desc.layout, ResourceState_COPY_DST));
			Protean::Renderer::FlushBarriers(cmd);
			graphicsDevice->CopyResource(&fadeManager.crossFadeTexture, &backbuffer, cmd);
			Protean::Renderer::PushBarrier(GPUBarrier::Image(&fadeManager.crossFadeTexture, ResourceState_COPY_DST, fadeManager.crossFadeTexture.desc.layout));
			Protean::Renderer::PushBarrier(GPUBarrier::Image(&backbuffer, ResourceState_COPY_SRC, backbuffer.desc.layout));
			Protean::Renderer::FlushBarriers(cmd);
			fadeManager.crossFadeTextureSaveRequired = false;
		}

		Protean::Image::SetCanvas(canvas);
		Protean::Font::SetCanvas(canvas);
		Viewport viewport;
		viewport.width = (float)swapChain.desc.width;
		viewport.height = (float)swapChain.desc.height;
		graphicsDevice->BindViewports(1, &viewport, cmd);

		if (rendertargetPreHDR10.IsValid())
		{
			graphicsDevice->RenderPassBegin(&rendertargetPreHDR10, cmd, true);
		}
		else
		{
			graphicsDevice->RenderPassBegin(&swapChain, cmd);
		}
		Compose(cmd);
		graphicsDevice->RenderPassEnd(cmd);

		if (rendertargetPreHDR10.IsValid())
		{
			// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
			graphicsDevice->RenderPassBegin(&swapChain, cmd);
			Protean::Image::Params fx;
			fx.enableFullScreen();
			fx.enableHDR10OutputMapping();
			Protean::Image::Draw(&rendertargetPreHDR10, fx, cmd);
			graphicsDevice->RenderPassEnd(cmd);
		}

		Protean::Input::ClearForNextFrame();
		Protean::Profiler::EndFrame(cmd);
		graphicsDevice->SubmitCommandLists();
		Protean::Renderer::UpdateGPUSuballocator();
	}

	void Application::Update(float dt)
	{
		auto range = Protean::Profiler::BeginRangeCPU("Update");

		infoDisplay.rect = {};

		Protean::Lua::SetDeltaTime(double(dt));
		Protean::Lua::Update();

		Protean::Backlog::Update(canvas, dt);

		Protean::ResourceManager::UpdateStreamingResources(dt);

		if (activePath != nullptr)
		{
			activePath->Update(dt);
			activePath->PostUpdate();
		}

		Protean::Profiler::EndRange(range); // Update
	}

	void Application::FixedUpdate()
	{
		Protean::Lua::FixedUpdate();

		if (activePath != nullptr)
		{
			activePath->FixedUpdate();
		}
	}

	void Application::Render()
	{
		auto range = Protean::Profiler::BeginRangeCPU("Render");

		Protean::Lua::Render();

		if (activePath != nullptr)
		{
			activePath->PreRender();
			activePath->Render();
			activePath->PostRender();
		}

		Protean::Profiler::EndRange(range); // Render
	}

	void Application::Compose(CommandList cmd)
	{
		auto range = Protean::Profiler::BeginRangeCPU("Compose");
		ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);

		if (activePath != nullptr)
		{
			activePath->Compose(cmd);
		}

		if (fadeManager.IsActive())
		{
			// display fade rect
			Protean::Image::Params fx;
			fx.enableFullScreen();
			fx.opacity = fadeManager.opacity;
			if (fadeManager.type == FadeManager::FadeType_FadeToColor)
			{
				fx.color = fadeManager.color;
				Protean::Image::Draw(nullptr, fx, cmd);
			}
			else if (fadeManager.type == FadeManager::FadeType_CrossFade)
			{
				Protean::Image::Draw(&fadeManager.crossFadeTexture, fx, cmd);
			}
		}
		else
		{
			fadeManager.crossFadeTexture = {};
		}

		// Draw the information display
		if (infoDisplay.active)
		{
			if (infoDisplay.rect.right > 0)
			{
				graphicsDevice->BindScissorRects(1, &infoDisplay.rect, cmd);
			}

			infodisplay_str.clear();
			if (infoDisplay.watermark)
			{
				infodisplay_str += "Wicked Engine ";
				infodisplay_str += Protean::Version::GetVersionString();
				infodisplay_str += " ";

#if defined(PLATFORM_WINDOWS_DESKTOP)
				infodisplay_str += "[Windows]";
#elif defined(PLATFORM_LINUX)
				infodisplay_str += "[Linux]";
#elif defined(PLATFORM_PS5)
				infodisplay_str += "[PS5]";
#elif defined(PLATFORM_XBOX)
				infodisplay_str += "[Xbox]";
#endif // PLATFORM

#if defined(_ARM)
				infodisplay_str += "[ARM]";
#elif defined(_WIN64)
				infodisplay_str += "[64-bit]";
#elif defined(_WIN32)
				infodisplay_str += "[32-bit]";
#endif // _ARM

				infodisplay_str += graphicsDevice->GetTag();

#ifdef _DEBUG
				infodisplay_str += "[DEBUG]";
#endif // _DEBUG
				if (graphicsDevice->IsDebugDevice())
				{
					infodisplay_str += "[debugdevice]";
				}
				infodisplay_str += "\n";
			}
			if (infoDisplay.device_name)
			{
				infodisplay_str += "Device: ";
				infodisplay_str += graphicsDevice->GetAdapterName();
				infodisplay_str += "\n";
			}
			if (infoDisplay.resolution)
			{
				infodisplay_str += "Resolution: ";
				infodisplay_str += std::to_string(canvas.GetPhysicalWidth());
				infodisplay_str += " x ";
				infodisplay_str += std::to_string(canvas.GetPhysicalHeight());
				infodisplay_str += " (";
				infodisplay_str += std::to_string(int(canvas.GetDPI()));
				infodisplay_str += " dpi)\n";
			}
			if (infoDisplay.logical_size)
			{
				infodisplay_str += "Logical Size: ";
				infodisplay_str += std::to_string(int(canvas.GetLogicalWidth()));
				infodisplay_str += " x ";
				infodisplay_str += std::to_string(int(canvas.GetLogicalHeight()));
				infodisplay_str += "\n";
			}
			if (infoDisplay.colorspace)
			{
				infodisplay_str += "Color Space: ";
				ColorSpace colorSpace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
				switch (colorSpace)
				{
				default:
				case Protean::Graphics::ColorSpace_SRGB:
					infodisplay_str += "sRGB";
					break;
				case Protean::Graphics::ColorSpace_HDR10_ST2084:
					infodisplay_str += "ST.2084 (HDR10)";
					break;
				case Protean::Graphics::ColorSpace_HDR_LINEAR:
					infodisplay_str += "Linear (HDR)";
					break;
				}
				infodisplay_str += "\n";
			}
			if (infoDisplay.fpsinfo)
			{
				deltatimes[fps_avg_counter++ % arraysize(deltatimes)] = deltaTime;
				float displaydeltatime = deltaTime;
				if (fps_avg_counter > arraysize(deltatimes))
				{
					float avg_time = 0;
					for (int i = 0; i < arraysize(deltatimes); ++i)
					{
						avg_time += deltatimes[i];
					}
					displaydeltatime = avg_time / arraysize(deltatimes);
				}

				infodisplay_str += std::to_string(int(std::round(1.0f / displaydeltatime))) + " FPS\n";
			}
			if (infoDisplay.heap_allocation_counter)
			{
				infodisplay_str += "Heap allocations per frame: ";
#ifdef WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
				infodisplay_str += std::to_string(number_of_heap_allocations.load());
				infodisplay_str += " (";
				infodisplay_str += std::to_string(size_of_heap_allocations.load());
				infodisplay_str += " bytes)\n";
				number_of_heap_allocations.store(0);
				size_of_heap_allocations.store(0);
#else
				infodisplay_str += "[disabled]\n";
#endif // WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
			}
			if (infoDisplay.pipeline_count)
			{
				infodisplay_str += "Graphics pipelines active: ";
				infodisplay_str += std::to_string(graphicsDevice->GetActivePipelineCount());
				infodisplay_str += "\n";
			}

			if (infoDisplay.pipeline_creation)
			{
				int pipeline_creation = Protean::Renderer::IsPipelineCreationActive();
				if (pipeline_creation > 0)
				{
					infodisplay_str += "Pending pipeline creations by graphics driver: " + std::to_string(pipeline_creation) + ". Some rendering will be skipped.\n";
				}
			}
			
			if (infoDisplay.mouse_info)
			{
				infodisplay_str += "Mouse = (" + std::to_string(Protean::Input::GetPointer().x) + ", " + std::to_string(Protean::Input::GetPointer().y) + ") ";
				if (Protean::Input::Down(Protean::Input::MOUSE_BUTTON_LEFT))
				{
					infodisplay_str += "| Left ";
				}
				if (Protean::Input::Down(Protean::Input::MOUSE_BUTTON_MIDDLE))
				{
					infodisplay_str += "| Middle ";
				}
				if (Protean::Input::Down(Protean::Input::MOUSE_BUTTON_RIGHT))
				{
					infodisplay_str += "| Right ";
				}
				infodisplay_str += "\n";
			}
			
			Protean::Font::Params params = Protean::Font::Params(
				4 + canvas.PhysicalToLogical((uint32_t)infoDisplay.rect.left),
				4 + canvas.PhysicalToLogical((uint32_t)infoDisplay.rect.top),
				infoDisplay.size,
				Protean::Font::WIFALIGN_LEFT,
				Protean::Font::WIFALIGN_TOP,
				Protean::Color::White(),
				Protean::Color::Shadow()
			);
			params.shadow_softness = 0.4f;

			// Explanation: this compose pass is in LINEAR space if display output is linear or HDR10
			//	If HDR10, the HDR10 output mapping will be performed on whole image later when drawing to swapchain
			if (colorspace != ColorSpace_SRGB)
			{
				params.enableLinearOutputMapping(9);
			}

			params.cursor = Protean::Font::Draw(infodisplay_str, params, cmd);

			// VRAM:
			{
				GraphicsDevice::MemoryUsage vram = graphicsDevice->GetMemoryUsage();
				bool warn = false;
				if (vram.usage > vram.budget)
				{
					params.color = Protean::Color::Error();
					warn = true;
				}
				else if (float(vram.usage) / float(vram.budget) > 0.9f)
				{
					params.color = Protean::Color::Warning();
					warn = true;
				}
				if (infoDisplay.vram_usage || warn)
				{
					params.cursor = Protean::Font::Draw("VRAM usage: " + std::to_string(vram.usage / 1024 / 1024) + "MB / " + std::to_string(vram.budget / 1024 / 1024) + "MB\n", params, cmd);
					params.color = Protean::Color::White();
				}
			}

			// Write warnings below:
			params.color = Protean::Color::Warning();
#ifdef _DEBUG
			params.cursor = Protean::Font::Draw("Warning: This is a [DEBUG] build, performance will be slow!\n", params, cmd);
#endif
			if (graphicsDevice->IsDebugDevice())
			{
				params.cursor = Protean::Font::Draw("Warning: Graphics is in [debugdevice] mode, performance will be slow!\n", params, cmd);
			}

			// Write errors below:
			params.color = Protean::Color::Error();
			if (Protean::Renderer::GetShaderMissingCount() > 0)
			{
				params.cursor = Protean::Font::Draw(std::to_string(Protean::Renderer::GetShaderMissingCount()) + " shaders missing! Check the backlog for more information!\n", params, cmd);
			}
			if (Protean::Renderer::GetShaderErrorCount() > 0)
			{
				params.cursor = Protean::Font::Draw(std::to_string(Protean::Renderer::GetShaderErrorCount()) + " shader compilation errors! Check the backlog for more information!\n", params, cmd);
			}
			if (Protean::Backlog::GetUnseenLogLevelMax() >= Protean::Backlog::LogLevel_Error)
			{
				params.cursor = Protean::Font::Draw("Errors found, check the backlog for more information!", params, cmd);
			}

			if (infoDisplay.colorgrading_helper)
			{
				Protean::Image::Draw(
					Protean::TextureHelper::getColorGradeDefault(),
					Protean::Image::Params(
						canvas.PhysicalToLogical((uint32_t)infoDisplay.rect.left),
						canvas.PhysicalToLogical((uint32_t)infoDisplay.rect.top),
						canvas.PhysicalToLogical(256),
						canvas.PhysicalToLogical(16)
					),
					cmd
				);
			}

			if (infoDisplay.rect.right > 0)
			{
				Protean::Graphics::Rect rect;
				rect.right = canvas.width;
				rect.bottom = canvas.height;
				graphicsDevice->BindScissorRects(1, &rect, cmd);
			}
		}

		Protean::Profiler::DrawData(canvas, 4, 10, cmd, colorspace);

		Protean::Backlog::Draw(canvas, cmd, colorspace);

		Protean::Profiler::EndRange(range); // Compose
	}

	void Application::Exit()
	{
		Protean::Platform::Exit();
	}

	void Application::SetWindow(Protean::Platform::window_type window)
	{
		this->window = window;

		// User can also create a graphics device if custom logic is desired, but they must do before this function!
		if (graphicsDevice == nullptr)
		{
			ValidationMode validationMode = ValidationMode_Disabled;
			if (Protean::Arguments::HasArgument("debugdevice"))
			{
				validationMode = ValidationMode_Enabled;
			}
			if (Protean::Arguments::HasArgument("gpuvalidation"))
			{
				validationMode = ValidationMode_GPU;
			}
			if (Protean::Arguments::HasArgument("gpu_verbose"))
			{
				validationMode = ValidationMode_Verbose;
			}

			GPUPreference preference = GPUPreference_Discrete;
			if (Protean::Arguments::HasArgument("igpu"))
			{
				preference = GPUPreference_Integrated;
			}
			else if (Protean::Arguments::HasArgument("nvidiagpu"))
			{
				preference = GPUPreference_Nvidia;
			}
			else if (Protean::Arguments::HasArgument("amdgpu"))
			{
				preference = GPUPreference_AMD;
			}
			else if (Protean::Arguments::HasArgument("intelgpu"))
			{
				preference = GPUPreference_Intel;
			}

#ifdef PLATFORM_PS5
			Protean::Renderer::SetShaderPath(Protean::Renderer::GetShaderPath() + "ps5/");
			graphicsDevice = std::make_unique<GraphicsDevice_PS5>(validationMode);
#elif defined(PLATFORM_APPLE)
			Protean::Renderer::SetShaderPath(Protean::Renderer::GetShaderPath() + "metal/");
			graphicsDevice = std::make_unique<MetalDevice>(validationMode, preference);

#else
			bool use_dx12 = Protean::Arguments::HasArgument("dx12");
			bool use_vulkan = Protean::Arguments::HasArgument("vulkan");

#ifndef WICKEDENGINE_BUILD_DX12
			if (use_dx12) {
				Protean::Helper::messageBox("The engine was built without DX12 support!", "Error");
				use_dx12 = false;
			}
#endif // WICKEDENGINE_BUILD_DX12
#ifndef WICKEDENGINE_BUILD_VULKAN
			if (use_vulkan) {
				Protean::Helper::messageBox("The engine was built without Vulkan support!", "Error");
				use_vulkan = false;
			}
#endif // WICKEDENGINE_BUILD_VULKAN

			if (!use_dx12 && !use_vulkan)
			{
#if defined(WICKEDENGINE_BUILD_DX12)
				use_dx12 = true;
#elif defined(WICKEDENGINE_BUILD_VULKAN)
				use_vulkan = true;
#else
				Protean::Backlog::post("No rendering backend is enabled! Please enable at least one so we can use it as default", Protean::Backlog::LogLevel_Error);
				assert(false);
#endif
			}
			assert(use_dx12 || use_vulkan);

			if (use_vulkan)
			{
#ifdef WICKEDENGINE_BUILD_VULKAN
				Protean::Renderer::SetShaderPath(Protean::Renderer::GetShaderPath() + "spirv/");
				graphicsDevice = std::make_unique<VulkanDevice>(window, validationMode, preference);
#endif
			}
			else if (use_dx12)
			{
#ifdef WICKEDENGINE_BUILD_DX12
#ifdef PLATFORM_XBOX
				Protean::Renderer::SetShaderPath(Protean::Renderer::GetShaderPath() + "hlsl6_xs/");
#else
				Protean::Renderer::SetShaderPath(Protean::Renderer::GetShaderPath() + "hlsl6/");
#endif // PLATFORM_XBOX
				graphicsDevice = std::make_unique<D3D12Device>(validationMode, preference);
#endif
			}
#endif // PLATFORM_PS5
		}
		Protean::Graphics::GetDevice() = graphicsDevice.get();

		if (renderWidth > 0 && renderHeight > 0)
		{
			Platform::WindowProperties windowprops;
			Platform::GetWindowProperties(window, &windowprops);
			canvas.init(renderWidth, renderHeight, windowprops.dpi);
		}
		else
		{
			canvas.init(window);
		}

		SwapChainDesc desc = swapChain.desc;
		if (!swapChain.IsValid())
		{
			// initialize for the first time
			desc.buffer_count = 3;
			if (graphicsDevice->CheckCapability(GraphicsDeviceCapability_R9G9B9E5_SHAREDEXP_RENDERABLE))
			{
				desc.format = Format_R9G9B9E5_SHAREDEXP;
			}
			else
			{
				desc.format = Format_R10G10B10A2_UNORM;
			}
		}
		desc.width = canvas.GetPhysicalWidth();
		desc.height = canvas.GetPhysicalHeight();
		desc.allow_hdr = allow_hdr;
		bool success = graphicsDevice->CreateSwapChain(&desc, window, &swapChain);
		assert(success);

		rendertargetPreHDR10 = {};

#ifdef PLATFORM_PS5
		// PS5 swapchain resolution was decided in CreateSwapchain(), so reinit canvas:
		canvas.init(swapChain.desc.width, swapChain.desc.height);
#endif // PLATFORM_PS5

		swapChainVsyncChangeEvent = Protean::EventHandler::Subscribe(Protean::EventHandler::EVENT_SET_VSYNC, [this](uint64_t userdata) {
			SwapChainDesc desc = swapChain.desc;
			desc.vsync = userdata != 0;
			bool success = graphicsDevice->CreateSwapChain(&desc, nullptr, &swapChain);
			assert(success);
			});

	}

	void Application::SetRenderResolution(uint32_t width, uint32_t height)
	{
		renderWidth = width;
		renderHeight = height;
	}

	void Application::GetRenderResolution(uint32_t& width, uint32_t& height) const
	{
		if (renderWidth > 0 && renderHeight > 0)
		{
			width = renderWidth;
			height = renderHeight;
		}
		else
		{
			width = canvas.GetPhysicalWidth();
			height = canvas.GetPhysicalHeight();
		}
	}

	void Application::SetFullScreen(bool fullscreen)
	{
		Protean::Platform::SetWindowFullScreen(window, fullscreen);
	}

	bool Application::IsFullScreen() const
	{
		return Protean::Platform::IsWindowFullScreen(window);
	}

	bool Application::IsScriptReplacement() const
	{
		return Protean::Helper::FileExists(rewriteable_startup_script_text);
	}
}


#ifdef WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
// Heap alloc replacements are used to count heap allocations:
//	It is good practice to reduce the amount of heap allocations that happen during the frame,
//	so keep an eye on the info display of the engine while Application::InfoDisplayer::heap_allocation_counter is enabled

void* operator new(std::size_t size) {
	number_of_heap_allocations.fetch_add(1);
	size_of_heap_allocations.fetch_add(size);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size) {
	number_of_heap_allocations.fetch_add(1);
	size_of_heap_allocations.fetch_add(size);
	void* p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}
void* operator new[](std::size_t size, const std::nothrow_t&) throw() {
	number_of_heap_allocations.fetch_add(1);
	size_of_heap_allocations.fetch_add(size);
	return malloc(size);
}
void* operator new(std::size_t size, const std::nothrow_t&) throw() {
	number_of_heap_allocations.fetch_add(1);
	size_of_heap_allocations.fetch_add(size);
	return malloc(size);
}
void operator delete(void* ptr) throw() { free(ptr); }
void operator delete (void* ptr, const std::nothrow_t&) throw() { free(ptr); }
void operator delete[](void* ptr) throw() { free(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) throw() { free(ptr); }
#endif // WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
