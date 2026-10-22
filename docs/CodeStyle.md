# Protean code style

This is the style all new code follows. Code inherited from Wicked Engine is being migrated
towards it step by step (see "Migration status" at the bottom).

## Layout

```
build/            premake5.lua, generated projects (build/projects), intermediates (build/temp), binaries (build/bin)
data/             runtime data: data/editor (editor working directory), data/content (sample content)
docs/             documentation
external/         third party code, never reformatted
source/engine/    the engine, one folder per subsystem
source/editor/    the editor
source/game/      the game module (DLL in Debug/Development, standalone exe in Shipping)
source/tools/     offline tools (shader compiler, ...)
source/samples/   samples and tests
```

Engine subsystems live in `source/engine/<subsystem>/`:
`audio`, `core`, `debug`, `gui`, `input`, `io`, `math`, `network`, `physics`, `platform`,
`render`, `resource`, `scene`, `script`.

The graphics API abstraction lives in `source/engine/render/GraphicsAPI/`. Include
`render/GraphicsAPI/GraphicsAPI.h` to get everything that is needed to talk to the GPU. Each
backend lives in its own folder and is split by responsibility:

```
GraphicsAPI/D3D12/D3D12Device.h      public device class
GraphicsAPI/D3D12/D3D12Internal.h    private, shared by the backend's .cpp files only
GraphicsAPI/D3D12/D3D12Device.cpp    device creation, submission, frame lifetime
GraphicsAPI/D3D12/D3D12SwapChain.cpp swap chains
GraphicsAPI/D3D12/D3D12Resources.cpp buffers, textures, shaders, pipelines, descriptors
GraphicsAPI/D3D12/D3D12CmdList.cpp   command list recording
```

Vulkan and Metal follow the same pattern.

## Files and includes

- File names are PascalCase, without prefixes: `render/Renderer.h`, `core/JobSystem.cpp`.
- Includes are relative to `source/engine` (`#include "render/Renderer.h"`) or to `external`
  (`#include "stb_image.h"`, `#include <Jolt/Jolt.h>`).
- `#pragma once` in every header.
- Applications include `Protean.h` to get the whole engine.

## Namespaces

- Everything lives in the root namespace `Protean`.
- Sub-namespaces are allowed for self-contained systems and are PascalCase:
  `Protean::Graphics`, `Protean::Renderer`, `Protean::Input`, `Protean::JobSystem`, ...
- Scene, ECS and math primitives live directly in `Protean` (`Protean::Scene`,
  `Protean::Entity`, `Protean::AABB`).

## Types

Use the fixed width aliases from `core/Types.h` in new code:

```cpp
i8 i16 i32 i64   u8 u16 u32 u64   f32 f64   usize
```

## Enums

Plain (unscoped) enums, values are prefixed with the enum name and an underscore:

```cpp
enum MyEnum
{
	MyEnum_Value1,
	MyEnum_Value2
};

enum BindFlag : u32
{
	BindFlag_NONE = 0,
	BindFlag_VERTEX_BUFFER = 1 << 0,
};
```

New enums use PascalCase value names (`MyEnum_Value1`). Enums converted from Wicked Engine kept
their original value spelling (`Format_R8G8B8A8_UNORM`).

Bit flag enums opt into the type safe operators with `enable_bitmask_operators` (see
`core/CommonInclude.h`).

## Structs

Data-only structs have public PascalCase members and no member functions:

```cpp
// Data-only
struct MyStruct
{
	i32 MyValue;
};
```

Structs with behaviour have PascalCase functions; private state is lower case and comes last:

```cpp
// struct with funcs/members
struct MyStructFunc
{
	void MyFunc();

private:
	i32 member;
};
```

Use `struct` rather than `class`.

## Formatting

- Tabs for indentation, braces on their own line (Allman), see `.clang-format`.
- No exceptions, no RTTI.
- Free functions are PascalCase.

## Migration status

Done:
- Folder layout, premake build, `bootstrap.bat` / `bootstrap.sh`.
- `wi` namespace renamed to `Protean`, sub-namespaces PascalCase.
- All engine/editor `enum class` types converted to prefixed plain enums.
- Graphics backends split into multiple translation units under `render/GraphicsAPI`.
- `core/Types.h` primitive aliases.
- Game module (`source/game`) written in the new style.

Still to do (in order of priority):
- Rename struct members to the PascalCase / private-lowercase convention, subsystem by subsystem
  (start with `render/GraphicsAPI`, its descriptor structs are the most widely used).
- Replace `uint32_t` & co with the `core/Types.h` aliases.
- Convert remaining `class` declarations to `struct`.
- PascalCase enum values in converted enums.
- Rebuild the editor UI.
