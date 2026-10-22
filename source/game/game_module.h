#pragma once

// Contract between the game module and whatever hosts it.
//
//  Debug / Development: the game is a shared library (Game.dll / libGame.so) that statically
//                       links the engine. The Launcher executable loads it and calls the entry point.
//  Shipping:            the game is a standalone executable, main() calls the entry point directly.
//
// This header must not include engine headers, the Launcher does not link the engine.

#if defined(PROTEAN_GAME_DLL)
#if defined(_WIN32)
#define PROTEAN_GAME_API extern "C" __declspec(dllexport)
#else
#define PROTEAN_GAME_API extern "C" __attribute__((visibility("default")))
#endif
#else
#define PROTEAN_GAME_API extern "C"
#endif

#define PROTEAN_GAME_ENTRY_NAME "ProteanGameMain"

#if defined(_WIN32)
#define PROTEAN_GAME_MODULE_NAME "Game.dll"
#elif defined(__APPLE__)
#define PROTEAN_GAME_MODULE_NAME "libGame.dylib"
#else
#define PROTEAN_GAME_MODULE_NAME "libGame.so"
#endif

typedef int (*ProteanGameMainFn)(int argc, char** argv);

// Runs the game until the window is closed, returns the process exit code.
PROTEAN_GAME_API int ProteanGameMain(int argc, char** argv);
