// Development launcher: loads the game module next to the executable and hosts it.
// The engine is a shared library here, so the launcher and the game share one engine instance.
#include "host/game_host.h"

namespace Protean
{
	static int RunLauncher(int argc, char** argv)
	{
		const std::string modulePath = Helper::GetDirectoryFromPath(Helper::GetExecutablePath()) + SharedLibrary::GetPlatformFileName(PROTEAN_GAME_MODULE_NAME);

		static SharedLibrary library;
		if (!library.Load(modulePath))
		{
			Helper::messageBox("Failed to load " + modulePath + "\n" + library.GetError(), "Protean Launcher");
			return 1;
		}

		auto create = library.GetFunction<ProteanCreateGameModuleFn>(PROTEAN_GAME_CREATE_FUNCTION);
		auto destroy = library.GetFunction<ProteanDestroyGameModuleFn>(PROTEAN_GAME_DESTROY_FUNCTION);
		if (create == nullptr || destroy == nullptr)
		{
			Helper::messageBox(std::string("Game module entry points not found in ") + modulePath, "Protean Launcher");
			return 1;
		}

		GameModule* module = create();
		int result = RunGameHost(argc, argv, module);
		destroy(module);

		// The library stays loaded until exit: engine threads and static objects may still reference it
		return result;
	}
}

#if defined(PLATFORM_WINDOWS_DESKTOP)
#include <cstdlib>

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	return Protean::RunLauncher(__argc, __argv);
}
#else
int main(int argc, char* argv[])
{
	return Protean::RunLauncher(argc, argv);
}
#endif
