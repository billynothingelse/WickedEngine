// Shipping entry point: the game is linked into a single executable.
#include "game_module.h"
#include "platform/platform.h"

#if defined(PLATFORM_WINDOWS_DESKTOP)
#include <cstdlib>

int APIENTRY wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
	return ProteanGameMain(__argc, __argv);
}
#else
int main(int argc, char* argv[])
{
	return ProteanGameMain(argc, argv);
}
#endif
