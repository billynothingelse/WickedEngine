# Protean

Protean is a modern, performant, data-oriented game engine. It started as a fork of
[Wicked Engine](https://github.com/turanszkij/WickedEngine) and is being reshaped into its own
codebase: new layout, new build, new code style, and a rebuilt editor.

- [Code style](docs/CodeStyle.md)
- [Features inherited from Wicked Engine](features.txt)
- [C++ documentation (Wicked Engine)](docs/WickedEngine-Documentation.md)
- [Lua scripting API](docs/ScriptingAPI-Documentation.md)

## Repository layout

```
Protean.slnx      generated solution (Makefile on Linux)          (generated, not committed)
build/
  premake5.lua    workspace build script: shared settings, includes every project's premake5.lua
  projects/       generated project files                          (generated, not committed)
  temp/           intermediate object files          (generated, not committed)
binaries/
  <system>-<arch>-<config>/  executables and libraries, e.g. windows-x64-debug (generated, not committed)
data/
  editor/         editor runtime data, editor working directory
  content/        sample content (models, scripts, terrain)
docs/             documentation
external/         third party code, one folder per library (stb/, pugixml/, vma/, volk/, Jolt/, lua/, ...)
source/
  engine/         the engine (static library)
    audio/ core/ debug/ gui/ input/ io/ math/ network/ physics/ platform/
    render/ resource/ rhi/ rhi/{d3d12,vulkan,metal}/ scene/ script/
  editor/         the editor
  game/           the game module
  tools/          offline tools (shader compiler)
  samples/        tests and samples
bootstrap.bat     generate + build on Windows
bootstrap.sh      generate + build on Linux
```

## Building

### Windows

Requirements: Visual Studio 2026 (or 2022) with the "Desktop development with C++" workload.

```
bootstrap.bat                     :: Development build
bootstrap.bat Debug
bootstrap.bat Shipping
bootstrap.bat --generate-only     :: only generate the solution
```

`bootstrap.bat` downloads premake 5.0.0 to `build/premake5.exe` on first run, detects the
Visual Studio version (override with `--vs2022` / `--vs2026`), generates
`Protean.slnx` in the repository root (`Protean.sln` with `--vs2022`) and builds it with MSBuild. Open the solution to work in the IDE;
the Editor is the startup project and runs with `data/editor` as its working directory.

### Linux

Requirements: GCC with C++17, make, SDL2 development package (`sudo apt install libsdl2-dev`).

```
./bootstrap.sh                    # development build
./bootstrap.sh debug
./bootstrap.sh shipping
./bootstrap.sh --generate-only
```

Run the editor from its data folder:

```
cd data/editor && ../../binaries/linux-x64-development/ProteanEditor
```

### Configurations

| Configuration | Optimised | Engine                         | Game module                                         |
|---------------|-----------|--------------------------------|-----------------------------------------------------|
| Debug         | no        | shared library (`Protean.dll`) | `Game.dll`, hosted by the Launcher or the Editor    |
| Development   | yes       | shared library (`Protean.dll`) | `Game.dll`, hosted by the Launcher or the Editor    |
| Shipping      | yes       | static library                 | linked into the standalone `ProteanGame` executable |

In Debug and Development the editor, the launcher and the game module all link the same engine
shared library, so they share one engine state (scene registry, graphics device, job system,
allocators, ...). On Windows the DLL exports are generated from the engine's object files by
`source/tools/def_generator` (no `__declspec(dllexport)` annotations needed); on Linux the
`libProtean.so` exports everything by default.

### Game module

Game code lives in `source/game`:

- `game_module.h`: the interface between the game and its host. A game implements
  `Protean::GameModule` (`OnStart`, `OnStop`, `OnUpdate`, `OnFixedUpdate`) and exports
  `ProteanCreateGameModule` / `ProteanDestroyGameModule`. `OnStart` receives a `GameContext` with the
  scene, camera and render path to use.
- `game.h/.cpp`: the game itself.
- `host/`: the standalone host (window, main loop, `Application`) used by the launcher and by Shipping.
- `launcher/`: `ProteanGame` for Debug/Development, loads `Game.dll` / `libGame.so` and hosts it.
- `main/`: the Shipping entry point.

Run it from the `data` folder: `cd data && ../binaries/linux-x64-development/ProteanGame`.

### Playing the game in the editor

In Debug and Development builds the editor's top bar has **Play**, **Pause** and **Stop**:

- **Play** loads `Game.dll` / `libGame.so` from the editor's folder, snapshots the current scene and calls
  `OnStart` with that scene (`GameContext::InEditor` is true; the editor keeps control of its camera).
  Physics simulation is switched on while playing. Pressing Play while paused resumes.
- **Pause** stops calling the game and the physics simulation.
- **Stop** calls `OnStop`, unloads the game module and restores the scene snapshot, so the game can be
  rebuilt while the editor keeps running. Play also stops when switching, closing or opening a scene.

The game must release anything it registered with the engine (event subscriptions, callbacks, running
jobs) in `OnStop`, because its code is unloaded afterwards. The script buttons next to them run and stop
the last Lua script, as before.

## Projects

Every project has its own `premake5.lua` next to its sources (`external/`, `source/engine/`,
`source/editor/`, `source/game/`, `source/tools/*/`, `source/samples/tests/`). `build/premake5.lua`
defines the workspace, configurations, compiler settings and the `UseEngine()` helper, and includes
them. To add a project, create its folder with a `premake5.lua` and add an `include` line there.

| Project        | Kind                         | Notes                                        |
|----------------|------------------------------|----------------------------------------------|
| Engine         | `Protean` shared library (Debug/Development), static library (Shipping) | `source/engine` |
| External       | static library               | small third party libraries + Lua            |
| Jolt           | static library               | physics                                      |
| FAudio         | static library (Linux only)  | audio backend                                |
| Editor         | executable (`ProteanEditor`) | `source/editor`                              |
| Game           | shared library / executable  | `source/game`                                |
| Launcher       | executable (`ProteanGame`)   | hosts the Game module, Debug/Development only |
| DefGenerator   | executable (`def_generator`) | writes the engine DLL exports on Windows      |
| ShaderCompiler | executable                   | offline shader compilation                   |
| Tests          | executable                   | engine feature tests                         |

## Graphics API

The default renderer is D3D12 on Windows, Vulkan on Linux and Metal on Apple platforms. The
abstraction (render hardware interface) lives in `source/engine/rhi`; include `rhi/rhi.h`.

Command line arguments (no prefix): `vulkan` (use Vulkan on Windows), `debugdevice`,
`gpuvalidation`, `gpu_verbose`, `igpu`, `amdgpu`, `nvidiagpu`, `intelgpu`, `alwaysactive`.

## License

MIT, see [LICENSE.txt](LICENSE.txt). Third party licenses are listed in
[third_party_software.txt](third_party_software.txt).
