# Arc Engine Advanced Roadmap: Path to UE3/UE4 Photorealism

This roadmap details the path from the current BGFX/PBR foundation to a high-fidelity rendering engine capable of matching the visual quality, shading, and lighting features of industry-standard engines like **Unreal Engine 3** and **Unreal Engine 4**.

## Current Foundation

- SDL2 windowing and input.
- BGFX renderer initialized with Vulkan.
- Release-only Visual Studio 2022 CMake preset.
- Free camera with smooth controls.
- Procedural cube, sphere, plane, skybox, and sun marker.
- Starter PBR shader with GGX, Fresnel, roughness, metallic, emissive, ACES tonemapping, and gamma correction.
- Scene object list with primitive support.
- FPS and render stats overlay.
- Disabled VSync & precise 1000 FPS hybrid limiter.

---

## Completed Milestones

### Milestone 1: Real Asset Pipeline [DONE]
- **Asset Manager**: Added asset loading using `cgltf`.
- **Model Loader**: Support loading `.glb/.gltf` meshes (positions, normals, UVs, tangents, and indices).
- **GPU Resource Management**: Renderer static mesh handles with 32-bit indices.
- **Texture Loading**: `stb_image` texture upload/cache, supporting embedded GLB image data.
- **Material Instances**: Materials with slots for albedo, normal, metallic/roughness, AO, and emissive.

### Milestone 2: Real PBR Textures [DONE]
- **Vertex Layouts**: Expanded to support UV coordinates and tangents.
- **PBR Maps**: Full shader support for albedo, normal, roughness, metallic, AO, and emissive maps.
- **Default fallbacks**: Created 1x1 flat textures (white, black, flat normal) for material slot fallbacks.
- **Normal Mapping**: Implemented tangent-space normal mapping to reconstruct high-frequency surface details.

### Milestone 3: Real Shadows (Cascaded Shadow Maps) [DONE]
- **Directional Shadow Framebuffer**: Upgraded to 8192x8192 D32F depth texture for sub-millimeter precision.
- **4-Cascade Shadow Maps**: Splitting camera frustum at `10m, 30m, 75m, 160m` to focus resolution near the camera.
- **Shadow Stabilization**: Snapping frustum bounding spheres to texel grid to prevent shimmering/crawling edges under camera movement.
- **16-Sample Rotated Poisson Disk PCF**: Replaced box PCF with a high-fidelity Poisson Disk filter rotated per-pixel using Interleaved Gradient Noise (IGN) for organic, soft shadow penumbras.
- **Slope-Scaled Bias & Normal Offset**: Dynamic depth bias adjustment to eliminate shadow acne and "peter-panning".

---

## Planned Roadmap for UE3/UE4 Photorealism

To achieve the look of **Unreal Engine 3** (known for its strong contrast, bloom, prebaked lighting, and early post-processing) and **Unreal Engine 4** (famous for physically based rendering, temporal anti-aliasing, screen-space reflections, and dynamic GI), we plan the following milestones:

### Milestone 4: HDR, Color Science & Post-Processing (UE3/UE4 Standard) [IN PROGRESS]
*Target: Match the iconic UE3 bloom/depth-of-field and UE4's clean filmic look.*
- [x] **High Dynamic Range (HDR) Framebuffer**: Render the scene into an RGBA16F floating-point render target to preserve colors brighter than 1.0.
- [ ] **Auto Exposure & Eye Adaptation**: Compute average scene luminance over time using compute shaders or downsampled mipmaps, adjusting exposure dynamically when moving between dark and bright areas.
- [x] **Dual-Filtering Convolution Bloom**: Generate high-quality bloom using multi-pass downsampling and upsampling with Gaussian-like filters to simulate lens scattering without performance bottlenecks.
- [ ] **Filmic Color Grading (3D LUT)**: Implement a color grading post-process shader using a 3D lookup table (LUT) for saturation, contrast, lift, gamma, gain, and color balance correction.
- [ ] **Temporal Anti-Aliasing (TAA)**: Implement camera jittering (Halton sequence) and history-buffer blending (neighborhood clamping) to eliminate specular aliasing and sub-pixel noise (crucial for UE4's smooth appearance).
- [ ] **Bokeh Depth of Field (DoF)**: Add a depth-of-field shader to simulate physical camera lenses with realistic background blur.
- [ ] **Velocity Buffer & Motion Blur**: Render a per-pixel velocity buffer to apply camera and object motion blur.

### Milestone 5: Image-Based Lighting & Reflections (UE4 Standard)
*Target: Match UE4's specular reflection models and ambient light probes.*
- [ ] **Specular Image-Based Lighting (IBL)**:
  - Generate pre-filtered environment maps (roughness mips) using importance sampling.
  - Implement Split-Sum Approximation in shaders, pre-integrating the BRDF into a LUT texture.
- [ ] **Diffuse IBL (Ambient Probes)**: Bake ambient environment lighting into Spherical Harmonics (SH) coefficients for cheap, low-frequency diffuse ambient fill.
- [ ] **Reflection Captures**: Support local box and sphere reflection probes. Project local cubemaps with parallax correction to provide realistic static reflections in indoor environments.
- [ ] **Screen Space Reflections (SSR)**: Raymarch the depth buffer in screen space to produce dynamic reflections of on-screen geometry, fading out based on material roughness and screen-edge proximity.

### Milestone 6: Ambient Occlusion & Global Illumination (UE3/UE4 Hybrid)
*Target: Replicate UE3's Lightmass GI and UE4's Screen-Space Ambient Occlusion / Distance Field AO.*
- [ ] **Screen Space Ambient Occlusion (SSAO / GTAO)**:
  - Implement Ground-Truth Ambient Occlusion (GTAO) or Horizon-Based Ambient Occlusion (HBAO) to add contact shadows in corners, crevices, and joints.
  - Apply a bilateral blur filter to smooth AO noise without bleeding over depth edges.
- [ ] **Pre-computed Lightmaps (Offline GI - UE3 Lightmass style)**:
  - Add offline light baking for static meshes to bake high-fidelity diffuse bounce light, soft shadows, and ambient occlusion into lightmap textures.
- [ ] **Dynamic Global Illumination (Real-Time GI - UE4 SVOGI/LPV style)**:
  - Implement Light Propagation Volumes (LPV) or Voxel Cone Tracing (VCT) for dynamic diffuse indirect lighting and color bleeding.

### Milestone 7: Advanced Shading & Material Models (UE4 Standard)
*Target: Support realistic skin, cloth, detail maps, and complex geometries.*
- [ ] **Parallax Occlusion Mapping (POM)**: Raymarch heightmaps in the fragment shader to simulate 3D surface relief (such as deep stone crevices or brick mortar joints) that shifts realistically with the view angle.
- [ ] **Subsurface Scattering (SSS) Shading**: Implement screen-space subsurface scattering for organic materials like skin, wax, and marble to simulate light penetrating and scattering inside the surface.
- [ ] **Clear Coat & Cloth Shading Models**:
  - Add a second specular lobe for clear coat materials (e.g. car paint, carbon fiber).
  - Implement a micro-fiber fuzz shading model for fabrics.
- [ ] **GPU Particle Systems**: Simulate thousands of particles on the GPU with vector fields, collision against depth buffers, and light-emission properties.

### Milestone 8: Volumetrics & Atmospheric Effects (UE4 Standard)
*Target: Match UE4's atmospheric scattering and volumetric fog.*
- [ ] **Physically-Based Atmospheric Scattering**: Model Rayleigh and Mie scattering in real-time to render dynamic sky colors, realistic sunsets, and aerial perspective based on altitude.
- [ ] **Volumetric Fog**: Construct a 3D frustum voxel grid to participate in light scattering, enabling shafts of light (god rays) and localized fog density.
- [ ] **Volumetric Clouds**: Raymarch 3D noise textures to render volumetric, self-shadowing clouds.

### Milestone 9: Editor & Engine Tooling
*Target: Provide an editor viewport similar to Unreal's Editor for placing objects and configuring lights.*
- [ ] **ImGui Integration**: Integrate Dear ImGui for debug windows, asset browsing, and property inspectors.
- [ ] **Gizmos & Entity Transformation**: Allow selecting, translating, rotating, and scaling meshes directly in the viewport.
- [ ] **Post-Processing Volumes**: Define spatial volumes that automatically blend post-processing parameters (exposure, LUTs, bloom intensity) as the camera enters or exits.
- [ ] **Scene Serialization**: Save and load levels using JSON or a binary format.
