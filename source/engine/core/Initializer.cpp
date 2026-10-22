#include "core/Initializer.h"
#include "Protean.h"

#include <thread>
#include <atomic>

#if !defined(_M_ARM64) && (defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX))
#define CPUINFO_AVAILABLE
#include "cpuinfo.hpp"
#endif // defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX)

namespace Protean::Initializer
{
	static std::atomic_bool initializationStarted{ false };
	static Protean::JobSystem::context ctx;
	static Protean::Timer timer;
	static std::atomic_bool systems[INITIALIZED_SYSTEM_COUNT]{};

	void InitializeComponentsImmediate()
	{
		if (IsInitializeFinished())
			return;
		if (!initializationStarted.load())
		{
			InitializeComponentsAsync();
		}
		WaitForInitializationsToFinish();
	}
	void InitializeComponentsAsync()
	{
		if (IsInitializeFinished())
			return;
		timer.record();

		initializationStarted.store(true);

#if defined(PLATFORM_WINDOWS_DESKTOP)
		static constexpr const char* platform_string = "Windows";
#elif defined(PLATFORM_LINUX)
		static constexpr const char* platform_string = "Linux";
#elif defined(PLATFORM_PS5)
		static constexpr const char* platform_string = "PS5";
#elif defined(PLATFORM_XBOX)
		static constexpr const char* platform_string = "Xbox";
#elif defined(PLATFORM_MACOS)
		static constexpr const char* platform_string = "MacOS";
#elif defined(PLATFORM_IOS)
		static constexpr const char* platform_string = "iOS";
#endif // PLATFORM

#if defined(_MSC_VER) && defined(__clang__)
		static constexpr const char* compiler_string = "CLANG-CL";
#elif defined(__clang__)
		static constexpr const char* compiler_string = "CLANG";
#elif defined(_MSC_VER)
		static constexpr const char* compiler_string = "MSVC";
#elif defined(__GNUC__)
		static constexpr const char* compiler_string = "GCC";
#else
		static constexpr const char* compiler_string = "UNKNOWN";
#endif

		wilog("\n[Protean::Initializer] Initializing Wicked Engine, please wait...\nVersion: %s\nPlatform: %s\nCompiler: %s", Protean::Version::GetVersionString(), platform_string, compiler_string);

		StackString<1024> cpustring;
#ifdef CPUINFO_AVAILABLE
		CPUInfo cpuinfo;
		cpustring.push_back("\nCPU: ");
		cpustring.push_back(cpuinfo.model().c_str());
		cpustring.push_back("\n\tFeatures available: ");
		if (cpuinfo.haveSSE())
		{
			cpustring.push_back("SSE; ");
		}
		if (cpuinfo.haveSSE2())
		{
			cpustring.push_back("SSE 2; ");
		}
		if (cpuinfo.haveSSE3())
		{
			cpustring.push_back("SSE 3; ");
		}
		if (cpuinfo.haveSSE41())
		{
			cpustring.push_back("SSE 4.1; ");
		}
		if (cpuinfo.haveSSE42())
		{
			cpustring.push_back("SSE 4.2; ");
		}
		if (cpuinfo.haveAVX())
		{
			cpustring.push_back("AVX; ");
		}
		if (cpuinfo.haveFMA3())
		{
			cpustring.push_back("FMA3; ");
		}
		if (cpuinfo.haveF16C())
		{
			cpustring.push_back("F16C; ");
		}
		if (cpuinfo.haveAVX2())
		{
			cpustring.push_back("AVX 2; ");
		}
		if (cpuinfo.haveAVX512F())
		{
			cpustring.push_back("AVX 512; ");
		}
#else
		cpustring.push_back("\nCPU: unrecognized");
#endif // CPUINFO_AVAILABLE

		cpustring.push_back("\n\tFeatures used: ");
#ifdef _XM_SSE_INTRINSICS_
		cpustring.push_back("SSE; ");
		cpustring.push_back("SSE 2; ");
#endif // _XM_SSE_INTRINSICS_
#ifdef _XM_SSE3_INTRINSICS_
		cpustring.push_back("SSE 3; ");
#endif // _XM_SSE3_INTRINSICS_
#ifdef _XM_SSE4_INTRINSICS_
		cpustring.push_back("SSE 4.1; ");
#endif // _XM_SSE4_INTRINSICS_
#ifdef _XM_AVX_INTRINSICS_
		cpustring.push_back("AVX; ");
#endif // _XM_AVX_INTRINSICS_
#ifdef _XM_FMA3_INTRINSICS_
		cpustring.push_back("FMA3; ");
#endif // _XM_FMA3_INTRINSICS_
#ifdef _XM_F16C_INTRINSICS_
		cpustring.push_back("F16C; ");
#endif // _XM_F16C_INTRINSICS_
#ifdef _XM_AVX2_INTRINSICS_
		cpustring.push_back("AVX 2; ");
#endif // _XM_AVX2_INTRINSICS_
#ifdef _XM_ARM_NEON_INTRINSICS_
		cpustring.push_back("NEON; ");
#endif // _XM_ARM_NEON_INTRINSICS_

		Protean::Backlog::post(cpustring.c_str());

		if (!XMVerifyCPUSupport())
		{
			wilog_messagebox("XMVerifyCPUSupport() failed! This means that your CPU doesn't support a required feature! %s", cpustring.c_str());
		}

		wilog("\nRAM: %s", Protean::Helper::GetMemorySizeText(Protean::Helper::GetMemoryUsage().total_physical).c_str());

		size_t shaderdump_count = Protean::Renderer::GetShaderDumpCount();
		if (shaderdump_count > 0)
		{
			wilog("\nEmbedded shaders found: %d", (int)shaderdump_count);
		}
		else
		{
			wilog("\nNo embedded shaders found, shaders will be compiled at runtime if needed.\n\tShader source path: %s\n\tShader binary path: %s", Protean::Renderer::GetShaderSourcePath().c_str(), Protean::Renderer::GetShaderPath().c_str());
		}

#ifdef _DEBUG
		wilog("\nNumber of shared allocators (there is one per object type): %d", (int)Protean::Allocator::get_shared_allocator_count());
#endif // _DEBUG

		Protean::Backlog::post("");
		Protean::JobSystem::Initialize();

		Protean::Backlog::post("");
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::Input::Initialize(); systems[INITIALIZED_SYSTEM_INPUT].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::Renderer::Initialize(); systems[INITIALIZED_SYSTEM_RENDERER].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::TextureHelper::Initialize(); systems[INITIALIZED_SYSTEM_TEXTUREHELPER].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::HairParticleSystem::Initialize(); systems[INITIALIZED_SYSTEM_HAIRPARTICLESYSTEM].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::EmittedParticleSystem::Initialize(); systems[INITIALIZED_SYSTEM_EMITTEDPARTICLESYSTEM].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::Ocean::Initialize(); systems[INITIALIZED_SYSTEM_OCEAN].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::GPUSortLib::Initialize(); systems[INITIALIZED_SYSTEM_GPUSORTLIB].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::GPUBVH::Initialize(); systems[INITIALIZED_SYSTEM_GPUBVH].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::Physics::Initialize(); systems[INITIALIZED_SYSTEM_PHYSICS].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::TrailRenderer::Initialize(); systems[INITIALIZED_SYSTEM_TRAILRENDERER].store(true); });
		Protean::JobSystem::Execute(ctx, [](Protean::JobSystem::JobArgs args) { Protean::GaussianSplatModel::Initialize(); systems[INITIALIZED_SYSTEM_GAUSSIAN_SPLAT].store(true); });

		// Initialize these immediately:
		Protean::Lua::Initialize(); systems[INITIALIZED_SYSTEM_LUA].store(true);
		Protean::Audio::Initialize(); systems[INITIALIZED_SYSTEM_AUDIO].store(true);
		Protean::Image::Initialize(); systems[INITIALIZED_SYSTEM_IMAGE].store(true);
		Protean::Font::Initialize(); systems[INITIALIZED_SYSTEM_FONT].store(true);

		std::thread([] {
			Protean::JobSystem::Wait(ctx);
			wilog("\n[Protean::Initializer] Wicked Engine Initialized (%d ms)", (int)std::round(timer.elapsed()));
		}).detach();

	}

	bool IsInitializeFinished(INITIALIZED_SYSTEM system)
	{
		if (system == INITIALIZED_SYSTEM_COUNT)
		{
			return initializationStarted.load() && !Protean::JobSystem::IsBusy(ctx);
		}
		else
		{
			return systems[system].load();
		}
	}

	void WaitForInitializationsToFinish()
	{
		Protean::JobSystem::Wait(ctx);
	}
}
