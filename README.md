# Arc Engine

Arc Engine is a basic C++ game engine template for Windows projects. The engine code and game code are intentionally separated so new games can replace the `game/` folder without changing engine internals.

The example game renders a small BGFX sandbox scene with a procedural skybox, sun marker, ground, walls, cubes, spheres, planar projected shadows, FPS overlay, and a free camera.

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
engine/        Arc Engine library: application, input, camera, scene, renderer, shaders
game/          Example game executable using only the public Arc API
assets/        Game assets placeholder
docs/          Architecture notes
scripts/       Build/run helper scripts
.vscode/       VS Code build task and CMake settings
```

## Example Controls

- Hold right mouse button: look around
- `WASD`: move horizontally
- `Q` / `E`: move down/up
- `Shift`: faster movement
- `Esc`: quit

## Notes

- Builds are Release-only by default.
- BGFX is initialized with `bgfx::RendererType::Vulkan`.
- Mesh shading uses a starter PBR model: GGX specular, Schlick Fresnel, roughness, metallic, emissive, ambient fill, ACES tonemapping, and gamma correction.
- The current shadow is a projected planar silhouette for the example scene. Full photorealistic projects should replace this with shadow maps or cascaded shadow maps.
- The example scene now uses `Arc::Scene` objects instead of direct per-object draw calls from the game.
- The debug overlay shows FPS, draw calls, mesh draws, and shadow draws.
- Shaders are compiled during the CMake build with `bgfx[tools]` from vcpkg.
- Dependencies are managed by vcpkg manifest mode through `vcpkg.json`.

This is a rendering foundation, not an Unreal Engine replacement yet. The next major realism steps are shadow maps, image-based lighting, HDR environment maps, texture/material loading, normal maps, post-processing, and asset import.

See `docs/AdvancedRoadmap.md` for the staged implementation plan.
