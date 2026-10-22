// Development launcher: loads the game module next to the executable and runs it.
// Does not link the engine, everything lives in the game module.
#include "GameModule.h"

#include <cstdio>
#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdlib>
#else
#include <dlfcn.h>
#include <unistd.h>
#include <climits>
#endif

namespace Protean
{
	static std::string GetExecutableDirectory()
	{
#if defined(_WIN32)
		char path[MAX_PATH] = {};
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		std::string result = path;
#else
		char path[PATH_MAX] = {};
		ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
		std::string result = length > 0 ? std::string(path, length) : std::string();
#endif
		size_t slash = result.find_last_of("\\/");
		return slash == std::string::npos ? std::string() : result.substr(0, slash + 1);
	}

	static void ReportError(const std::string& message)
	{
#if defined(_WIN32)
		MessageBoxA(nullptr, message.c_str(), "Protean Launcher", MB_OK | MB_ICONERROR);
#else
		std::fprintf(stderr, "[Protean Launcher] %s\n", message.c_str());
#endif
	}

	static int RunLauncher(int argc, char** argv)
	{
		const std::string modulePath = GetExecutableDirectory() + PROTEAN_GAME_MODULE_NAME;

#if defined(_WIN32)
		HMODULE module = LoadLibraryA(modulePath.c_str());
		if (module == nullptr)
		{
			ReportError("Failed to load " + modulePath);
			return 1;
		}
		ProteanGameMainFn entry = (ProteanGameMainFn)GetProcAddress(module, PROTEAN_GAME_ENTRY_NAME);
#else
		void* module = dlopen(modulePath.c_str(), RTLD_NOW | RTLD_LOCAL);
		if (module == nullptr)
		{
			ReportError(std::string("Failed to load ") + modulePath + ": " + dlerror());
			return 1;
		}
		ProteanGameMainFn entry = (ProteanGameMainFn)dlsym(module, PROTEAN_GAME_ENTRY_NAME);
#endif
		if (entry == nullptr)
		{
			ReportError(std::string("Entry point ") + PROTEAN_GAME_ENTRY_NAME + " not found in " + modulePath);
			return 1;
		}

		// The module is intentionally not unloaded: engine threads may still be winding down.
		return entry(argc, argv);
	}
}

#if defined(_WIN32)
int APIENTRY WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int)
{
	return Protean::RunLauncher(__argc, __argv);
}
#else
int main(int argc, char* argv[])
{
	return Protean::RunLauncher(argc, argv);
}
#endif
