# Arc Engine Advanced Roadmap

This roadmap tracks the path from the current BGFX/PBR sandbox to a more advanced photorealistic engine foundation.

## Current Foundation

- SDL2 windowing and input.
- BGFX renderer initialized with Vulkan.
- Release-only Visual Studio 2022 CMake preset.
- Free camera.
- Procedural cube, sphere, plane, skybox, and sun marker.
- Starter PBR shader with GGX, Fresnel, roughness, metallic, emissive, ACES tonemapping, and gamma correction.
- Projected planar shadows for the demo scene.
- Scene object list with cube/sphere primitives.
- FPS and render stats overlay.

## Milestone 1: Real Asset Pipeline

- Add `AssetManager`.
- Load `.glb/.gltf` meshes.
- Support indexed vertex buffers from assets.
- Add texture loading and GPU texture cache.
- Add material instances with texture slots.

## Milestone 2: Real PBR Textures

- Add UV and tangent vertex layouts.
- Add albedo, normal, roughness, metallic, AO, and emissive maps.
- Add default fallback textures.
- Add normal mapping in shaders.

## Milestone 3: Real Shadows

- Add directional light shadow map framebuffer.
- Render shadow caster depth pass.
- Sample shadow map in lighting pass.
- Add PCF filtering.
- Upgrade to cascaded shadow maps for large outdoor scenes.

## Milestone 4: HDR And Post Processing

- Render scene into HDR framebuffer.
- Add exposure control.
- Add bloom.
- Add color grading LUT.
- Add SSAO/GTAO.

## Milestone 5: Image-Based Lighting

- Load HDR environment maps.
- Convert equirectangular HDRI to cubemap.
- Generate irradiance map.
- Generate prefiltered reflection map.
- Add BRDF LUT.

## Milestone 6: Scene Runtime

- Add entity IDs/components.
- Add transform hierarchy.
- Add cameras, lights, mesh renderers as components.
- Add JSON scene serialization.
- Add config file for startup scene/window/renderer settings.

## Milestone 7: Production Systems

- Physics integration.
- Audio integration.
- Input action maps.
- Asset hot reload.
- Shader hot reload.
- Packaging scripts.
