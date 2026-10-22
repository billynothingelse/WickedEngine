// Platform window + main loop for the game. Shared by the DLL and the Shipping executable.
#include "Game.h"

#if defined(PLATFORM_WINDOWS_DESKTOP)

namespace Protean
{
	static Game* game = nullptr;

	static LRESULT CALLBACK GameWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_SIZE:
		case WM_DPICHANGED:
			if (game != nullptr && game->is_window_active)
				game->SetWindow(hWnd);
			break;
		case WM_CHAR:
			switch (wParam)
			{
			case VK_BACK:
				Gui::TextInputField::DeleteFromInput();
				break;
			case VK_RETURN:
				break;
			default:
				Gui::TextInputField::AddInput((wchar_t)wParam);
				break;
			}
			break;
		case WM_INPUT:
			Input::RawInput::ParseMessage((void*)lParam);
			break;
		case WM_KILLFOCUS:
			if (game != nullptr)
				game->is_window_active = false;
			break;
		case WM_SETFOCUS:
			if (game != nullptr)
				game->is_window_active = true;
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	i32 RunGame(i32 argc, char** argv)
	{
		static Game instance;
		game = &instance;

		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		HINSTANCE hInstance = GetModuleHandle(nullptr);
		WNDCLASSEXW wcex = {};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = GameWndProc;
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszClassName = L"ProteanGame";
		RegisterClassExW(&wcex);

		HWND hWnd = CreateWindowW(wcex.lpszClassName, L"Protean", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 1920, 1080, nullptr, nullptr, hInstance, nullptr);
		ShowWindow(hWnd, SW_SHOWDEFAULT);

		game->SetWindow(hWnd);
		Arguments::Parse(argc, argv);

		MSG msg = {};
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				game->Run();
			}
		}

		JobSystem::ShutDown();
		game = nullptr;
		return (i32)msg.wParam;
	}
}

#elif defined(SDL2)

#include <SDL2/SDL.h>
#include "platform/SDL2.h"

namespace Protean
{
	i32 RunGame(i32 argc, char** argv)
	{
		static Game game;

		sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
		sdl2::window_ptr_t window = sdl2::make_window(
			"Protean",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			1920, 1080,
			SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
		if (!window)
			return 1;

		game.SetWindow(window.get());
		Arguments::Parse(argc, argv);

		bool quit = false;
		while (!quit)
		{
			SDL_PumpEvents();
			game.Run();

			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				switch (event.type)
				{
				case SDL_QUIT:
					quit = true;
					break;
				case SDL_WINDOWEVENT:
					if (event.window.event == SDL_WINDOWEVENT_CLOSE)
						quit = true;
					else if (event.window.event == SDL_WINDOWEVENT_RESIZED)
						game.SetWindow(game.window);
					break;
				default:
					break;
				}
				Input::SDL::ProcessEvent(event);
			}
		}

		JobSystem::ShutDown();
		return 0;
	}
}

#else
#error "Protean game: no platform layer for this target"
#endif
