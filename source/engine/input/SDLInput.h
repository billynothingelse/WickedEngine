#pragma once
#include "core/CommonInclude.h"
#include "input/Input.h"
#include "core/Vector.h"

#ifdef SDL2
#include <SDL2/SDL.h>
#elif defined(SDL3)
#include <SDL3/SDL.h>
#endif

namespace Protean::Input::SDL
{
	// Call this once to register raw input devices
	void Initialize();

	// Updates the state of raw input devices, call once per frame
	void Update();

	// Writes the keyboard state into state parameter
	void GetKeyboardState(Protean::Input::KeyboardState* state);

	// Writes the mouse state into state parameter
	void GetMouseState(Protean::Input::MouseState* state);

	// Returns how many controller devices have received input ever. This doesn't correlate with which ones are currently available
	int GetMaxControllerCount();

	// Returns whether the controller identified by index parameter is available or not
	//	Id state parameter is not nullptr, and the controller is available, the state will be written into it
	bool GetControllerState(Protean::Input::ControllerState* state, int index);

	// Sends feedback data for the controller identified by index parameter to output
	void SetControllerFeedback(const Protean::Input::ControllerFeedback& data, int index);

	// External events can be used for events that is needed outside the engine library, like main_SDL2.cpp for example
#if defined(SDL2) || defined(SDL3)
	// Call this within the main.cpp program loop for the engine to be able handle the input
	void ProcessEvent(const SDL_Event &event);
#endif
}
