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
build/
  premake5.lua    project-wide build script
  projects/       generated solutions / makefiles   (generated, not committed)
  temp/           intermediate object files          (generated, not committed)
  bin/<Config>/   executables and libraries          (generated, not committed)
data/
  editor/         editor runtime data, editor working directory
  content/        sample content (models, scripts, terrain)
docs/             documentation
external/         third party code (DirectXMath, Jolt, Lua, Vulkan/D3D12/Metal headers, stb, ...)
source/
  engine/         the engine (static library)
    audio/ core/ debug/ gui/ input/ io/ math/ network/ physics/ platform/
    render/ render/GraphicsAPI/{D3D12,Vulkan,Metal}/ resource/ scene/ script/
  editor/         the editor
  game/           the game module
  tools/          offline tools (shader compiler)
  samples/        tests and samples
bootstrap.bat     generate + build on Windows
bootstrap.sh      generate + build on Linux
```

## Building

### Windows

Requirements: Visual Studio 2022 or newer with the "Desktop development with C++" workload.

```
bootstrap.bat                     :: Development build
bootstrap.bat Debug
bootstrap.bat Shipping
bootstrap.bat --generate-only     :: only generate build\projects\Protean.sln
```

`bootstrap.bat` downloads premake into `build/tools` on first run, generates
`build/projects/Protean.sln` and builds it with MSBuild. Open the solution to work in the IDE;
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
cd data/editor && ../../build/bin/Development/ProteanEditor
```

### Configurations

| Configuration | Optimised | Game module                                              |
|---------------|-----------|----------------------------------------------------------|
| Debug         | no        | `Game` shared library, started by the `Launcher` exe      |
| Development   | yes       | `Game` shared library, started by the `Launcher` exe      |
| Shipping      | yes       | `Game` is a standalone executable, engine linked statically |

### Game module

Game code lives in `source/game`. `Game.h/.cpp` hold the game itself (an `Application` plus a
`RenderPath3D`), `GamePlatform.cpp` owns the window and main loop, and `GameModule.h` defines the
`ProteanGameMain` entry point shared by both build modes:

- Debug / Development: `Game` builds as `Game.dll` / `libGame.so` (with the engine linked into it)
  and `ProteanGame` (the `Launcher` project) loads it and calls `ProteanGameMain`.
- Shipping: `Game` builds as a single executable; `source/game/main/Main.cpp` calls
  `ProteanGameMain` directly.

Run it from the `data` folder: `cd data && ../build/bin/Development/ProteanGame`.

## Projects

| Project        | Kind                         | Notes                                        |
|----------------|------------------------------|----------------------------------------------|
| Engine         | static library (`Protean`)   | `source/engine`                              |
| External       | static library               | small third party libraries + Lua            |
| Jolt           | static library               | physics                                      |
| FAudio         | static library (Linux only)  | audio backend                                |
| Editor         | executable (`ProteanEditor`) | `source/editor`                              |
| Game           | shared library / executable  | `source/game`                                |
| Launcher       | executable (`ProteanGame`)   | loads the Game module, Debug/Development only |
| ShaderCompiler | executable                   | offline shader compilation                   |
| Tests          | executable                   | engine feature tests                         |

## Graphics API

The default renderer is D3D12 on Windows, Vulkan on Linux and Metal on Apple platforms. The
abstraction lives in `source/engine/render/GraphicsAPI`; include `render/GraphicsAPI/GraphicsAPI.h`.

Command line arguments (no prefix): `vulkan` (use Vulkan on Windows), `debugdevice`,
`gpuvalidation`, `gpu_verbose`, `igpu`, `amdgpu`, `nvidiagpu`, `intelgpu`, `alwaysactive`.

## License

MIT, see [LICENSE.txt](LICENSE.txt). Third party licenses are listed in
[third_party_software.txt](third_party_software.txt).
