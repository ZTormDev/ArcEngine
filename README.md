# Arc Engine

Arc Engine is a basic C++ game engine template for Windows projects. The engine code and game code are intentionally separated so new games can replace the `game/` folder without changing engine internals.

## Requirements

- Visual Studio Community 2022 with the C++ desktop workload
- CMake 3.25 or newer
- vcpkg with `VCPKG_ROOT` set
- Vulkan runtime/driver support
- VS Code, optional but configured for `Ctrl+Shift+B`

## Build And Run

From VS Code, press `Ctrl+Shift+B`. The default task configures the Release preset, builds `ArcGame`, and runs it.

From a terminal:

```powershell
cmake --preset vs2022-release
cmake --build --preset build-release
.\build\vs2022-release\bin\Release\ArcGame.exe
```

## Layout

```text
engine/        Arc Engine library: platform, window, renderer lifecycle
game/          Example game executable using only the public Arc API
assets/        Game assets placeholder
docs/          Architecture notes
scripts/       Build/run helper scripts
.vscode/       VS Code build task and CMake settings
```

## Notes

- Builds are Release-only by default.
- BGFX is initialized with `bgfx::RendererType::Vulkan`.
- Dependencies are managed by vcpkg manifest mode through `vcpkg.json`.
