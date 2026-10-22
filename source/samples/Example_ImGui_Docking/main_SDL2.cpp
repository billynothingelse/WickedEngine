#include "stdafx.h"
#include "platform/SDL2.h"
#include "ImGui/imgui_impl_sdl.h"

Example_ImGui exampleImGui;

int sdl_loop()
{
    bool quit = false;
    while (!quit)
    {
        exampleImGui.Run();
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_CLOSE: // exit tests
                            quit = true;
                            break;
                        case SDL_WINDOWEVENT_RESIZED:
                            // Tells the engine to reload window configuration (size and dpi)
                            exampleImGui.SetWindow(exampleImGui.window);
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST: //TODO
                            exampleImGui.is_window_active = false;
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            exampleImGui.is_window_active = true;
                            if (Protean::ShaderCompiler::GetRegisteredShaderCount() > 0 && !Protean::Renderer::IsPipelineCreationActive())
                            {
                                std::thread([] {
                                    Protean::Backlog::post("[Shader check] Started checking " + std::to_string(Protean::ShaderCompiler::GetRegisteredShaderCount()) + " registered shaders for changes...");
                                    if (Protean::ShaderCompiler::CheckRegisteredShadersOutdated())
                                    {
                                        Protean::Backlog::post("[Shader check] Changes detected, initiating reload...");
                                        Protean::EventHandler::Subscribe_Once(Protean::EventHandler::EVENT_THREAD_SAFE_POINT, [](uint64_t userdata) {
                                            Protean::Renderer::ReloadShaders();
                                            });
                                    }
                                    else
                                    {
                                        Protean::Backlog::post("[Shader check] All up to date");
                                    }
                                    }).detach();
                            }
                            break;
                        default:
                            break;
                    }
                default:
                    break;
            }
            Protean::Input::SDL::ProcessEvent(event);
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    // TODO: Place code here.

    Protean::Arguments::Parse(argc, argv);

    sdl2::sdlsystem_ptr_t system = sdl2::make_sdlsystem(SDL_INIT_EVERYTHING | SDL_INIT_EVENTS);
    if (*system) {
		wilog_error("Error creating SDL2 system");
    }

    sdl2::window_ptr_t window = sdl2::make_window(
            "Wicked Engine Tests",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            1280, 800,
            SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
		wilog_error("Error creating window");
    }

    exampleImGui.SetWindow(window.get());

    int ret = sdl_loop();

    SDL_Quit();
    return ret;
}
