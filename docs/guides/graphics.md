# Graphics & Rendering Guide

AXIS Engine utilizes a modern forward rendering pipeline designed for high performance and extensibility.

---

## 1. Components & Lighting

### Rendering Components
- **MeshRenderer**: The primary component for 3D models. Supports `Model`, `Shader`, and `Order` (rendering bucket).
- **Material**: Defines surface properties. Supports `PHONG` (Shininess, Specular) and `PBR` (Roughness, Metallic, AO).
- **LOD (Level of Detail)**: Dynamically swaps models based on distance.
- **SkyboxRenderer**: Renders environment cubemaps.
- **ParticleEmitter**: CPU-simulated but GPU-instanced 2D effects.

### Lighting System
Supports up to 4 lights of each type in the default shaders:
- **Directional**: Global sun-like light. Direction follows Transform rotation.
- **Point**: Omni-directional light. Range defined by `Radius` and attenuation.
- **Spot**: Cone-shaped light. Defined by `CutOff` and `OuterCutOff`.

---

## 2. Shadow System
Dynamic shadows are supported for all light types using shadow mapping.

### Constraints & Modes
- **Limits**: Shadow casting is limited to **2 lights per type** (2 Dir, 2 Point, 2 Spot) to conserve texture units.
- **Modes**:
  - `SHADOWS: 0`: Off.
  - `SHADOWS: 1`: First Directional Light only (Fastest).
  - `SHADOWS: 2`: All supported shadow casters.

### Configuration (`.axs`)
```yaml
Config:
  SHADOWS: 2
  SHADOW_SIZE: 100.0   # Orthographic area for Dir lights
  SHADOW_DISTANCE: 150.0 # Max rendering distance
```

---

## 3. Culling & Optimization
The engine employs multiple techniques to minimize draw calls and geometry throughput:

- **Frustum Culling**: Automatically hides objects outside the camera's view.
- **Occlusion Culling**: Hardware-based visibility testing (requires `Occlusion` component).
- **Instance Batching**: Groups static meshes with identical Model/Shader into single draw calls.
- **Render Order**: Bucket-based rendering (Higher `Order` = Render on top).
- **Layer Filtering**: Visibility masks (1-32) controlled by `FILTER_LAYER`.

---

## 4. Post-Processing & Anti-Aliasing
Applied via the `PostProcessPipeline` on a full-screen quad.

### Supported Effects
- **AA Modes**: `NONE`, `FXAA` (Approximation), `TAA` (Temporal/High quality).
- **Image Filters**: Grayscale, Inversion, Sharpen, Blur, Edge Detection.

### Configuration (`.axs`)
```yaml
Config:
  ANTIALIASING: TAA
```

---

## 5. Shader Development
Standard shaders must follow these conventions:

### Vertex Layout
- `location 0`: Position (`vec3`)
- `location 1`: Normal (`vec3`)
- `location 2`: TexCoords (`vec2`)
- `location 5-6`: Bone Data (Animated only)

### Standard Uniforms
- `mat4 projection`, `view`, `model`
- `vec3 viewPos` (Camera position)
- `sampler2D shadowMapDir[4]`, `shadowMapPoint[2]`
- **SSBO Bindings**: `0` (Dir), `1` (Point), `2` (Spot) for lights.

---

## 6. RenderSystem API
Accessible via `m_App->GetRenderSystem()`.

- `Render(scene)`: Executes the main pass.
- `RenderShadows(scene)`: Generates shadow maps.
- `SetFaceCulling()`, `SetDepthTest()`: Manual state management.
- `SetFilterLayerMask()`: Global visibility control.

---

## See Also
- [Scene Format (.axs)](scene_format.md)
- [Architecture Overview](../core/architecture.md)
