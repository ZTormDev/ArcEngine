# Architecture

Arc Engine is split into two main targets:

- `ArcEngine`: static engine library with SDL2 windowing, input, free camera support, BGFX renderer lifecycle, simple scene drawing, and shader compilation.
- `ArcGame`: game executable that depends on the public `Arc` API.

The game should include headers from `engine/include/Arc` and avoid depending on engine source files directly.

Current engine modules:

- `Application`: SDL2 window, main loop, resize handling, BGFX Vulkan initialization.
- `Input`: keyboard, mouse buttons, mouse delta.
- `Camera` and `FreeCamera`: basic first-person camera movement.
- `Renderer`: BGFX shader programs, procedural cube/sphere/plane geometry, starter PBR material model, directional light, skybox, projected planar shadow, FPS/debug overlay.

Rendering roadmap for more photorealistic projects:

- Real shadow maps for sun and local lights.
- HDR image-based lighting and reflection probes.
- Texture-backed materials: albedo, normal, roughness, metallic, ambient occlusion.
- glTF asset loading.
- Post-processing: bloom, exposure, color grading, SSAO, depth of field.

See `docs/AdvancedRoadmap.md` for the milestone plan.
