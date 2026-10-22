// Shipping entry point: the game and the engine are linked into a single executable.
#include "host/game_host.h"

static int RunShipping(int argc, char** argv)
{
	Protean::GameModule* module = ProteanCreateGameModule();
	int result = Protean::RunGameHost(argc, argv, module);
	ProteanDestroyGameModule(module);
	return result;
}

#if defined(PLATFORM_WINDOWS_DESKTOP)
#include <cstdlib>

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	return RunShipping(__argc, __argv);
}
#else
int main(int argc, char* argv[])
{
	return RunShipping(argc, argv);
}
#endif
