# Architecture

Arc Engine is split into two main targets:

- `ArcEngine`: static engine library with SDL2 windowing, input, free camera support, BGFX renderer lifecycle, simple scene drawing, and shader compilation.
- `ArcGame`: game executable that depends on the public `Arc` API.

The game should include headers from `engine/include/Arc` and avoid depending on engine source files directly.

Current engine modules:

- `Application`: SDL2 window, main loop, resize handling, BGFX Vulkan initialization.
- `AssetManager`: `.gltf/.glb` mesh loading through `cgltf`, including PBR material texture references and embedded GLB image data.
- `Input`: keyboard, mouse buttons, mouse delta.
- `Camera` and `FreeCamera`: basic first-person camera movement.
- `Renderer`: BGFX shader programs, procedural cube/sphere/plane geometry, uploaded static meshes, texture cache, fallback textures, PBR material texture binding, directional light, skybox, FPS/debug overlay.

Systems roadmap for advanced game engine development:

- Custom memory management & profiling (Tracy).
- Entity Component System (ECS) & Scene Graph.
- 3D Physics (Jolt/PhysX integration).
- Skeletal animation & vertex skinning.
- Gameplay scripting (Lua/sol2).
- Spatial audio (SoLoud/OpenAL).
- Virtual File System (VFS) & Asset Packaging.

See docs/AdvancedRoadmap.md for the milestone plan.
