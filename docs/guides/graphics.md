# Graphics & Rendering Guide

AXIS Engine supports forward and deferred render paths with a shared shader ABI.

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

TAA renders directly into alternating history targets; the current history is exposed to later bloom/HDR/custom passes without a full-screen history blit. A resize or invalid camera matrix resets temporal accumulation.

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

### Standard ABI

- Per-draw model transform: `uniform mat4 u_Model`.
- Camera UBO: binding `20` (`CameraData`).
- Light metadata UBO: binding `21` (`LightData`).
- Global frame UBO: binding `22` (`GlobalData`).
- Light SSBOs: binding `23` (directional), `24` (point), `25` (spot).
- Audio pulse SSBO: binding `26` (custom post-process only).
- Skinned shaders support at most `128` bones and four influences per vertex.

### 6. Global frame data

The engine provides a global UBO at binding `22`:
```glsl
layout (std140, binding = 22) uniform GlobalData {
    float u_Time;
    float u_DeltaTime;
    vec2  u_Resolution;
    vec4  _padding[3];
};
```

### 7. Custom Data Ports
Each material has **8 custom numeric ports** that can be used to pass arbitrary data to the shader (e.g., `isEnemy`, `vibrationScale`).
- **C++ Usage**: `material.desc.ports.data[0] = 1.0f;`
- **Shader Usage**: `uniform float u_CustomPorts[8];`

### 8. Shader Declaration (.axs)
Shaders support `vertex`, `fragment`, and an optional `geometry` stage.
```yaml
Shader:
  Name: myCustomShader
  vertex: shaders/custom.vs
  geometry: shaders/custom.gs  # Optional
  fragment: shaders/custom.fs
```
Runtime rendering is exposed through focused interfaces such as `IRenderService` and
`IRenderRuntimeControl`. States and scripts can resolve the interface they need with
`Resolve<T>()`; concrete renderer ownership and frame lifecycle remain internal.

### 9. Custom post-process ABI

Each effect declares a `PostProcessInput` mask. The default mask requests only the current color; additional inputs are opt-in so an ordinary effect does not force G-buffer or audio dependencies. Texture units and uniforms are:

| Input | Unit | Uniform |
| --- | ---: | --- |
| Current color | 0 | `u_ScreenTexture` (`screenTexture` remains a compatibility alias) |
| Capture depth | 1 | `u_DepthTexture` |
| G-buffer normal | 2 | `u_NormalTexture` |
| World position (optional direct texture) | 3 | `u_WorldPositionTexture` |

The pipeline also sets `u_InverseViewProjection`, `u_AudioLevel` (`rms`, `peak`, gated intensity, calibrated noise floor), `u_PulseCount`, and `u_Has*` availability flags. To avoid a full-resolution position MRT, the OpenGL backend normally provides world position as capture depth plus `u_InverseViewProjection`: `u_HasWorldPosition` and `u_WorldPositionFromDepth` are true while `u_HasWorldPositionTexture` is false. Reconstruct with `world = u_InverseViewProjection * vec4(uv * 2 - 1, depth * 2 - 1, 1)` and divide by `world.w`. A future backend may instead bind a direct texture on unit 3. Normal data is deferred-only, so shaders must check its flag.

Audio pulses use this std430 layout:

```glsl
struct AudioPulse {
    float intensity;
    float peak;
    float age;
    float duration;
};

layout(std430, binding = 26) readonly buffer AudioPulseBuffer {
    AudioPulse u_AudioPulses[];
};
```

The serialized effect mask uses bits: color `1`, depth `2`, normal `4`, world position `8`, camera matrices `16`, and audio pulses `32`.
Effects default to color-only for compatibility and cost control; request every additional input explicitly, or use the `PostProcessInput::Standard` mask when all six inputs are required.

---

## 10. Skeletal Animation (Skinning)
For animated models, you must handle bone weights and matrices in the vertex shader. Use the snippet below:

### Vertex Shader Skinning Snippet
```glsl
#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
// Bone data at locations 5 and 6
layout (location = 5) in ivec4 aBoneIds; 
layout (location = 6) in vec4 aWeights;

uniform mat4 u_Model;
uniform mat4 u_FinalBonesMatrices[128];
uniform bool u_IsInstanced;
layout(location = 10) in mat4 instanceMatrix;

void main() {
    vec4 totalPosition = vec4(0.0f);
    bool hasBones = false;
    
    for(int i = 0 ; i < 4 ; i++) {
        if(aBoneIds[i] == -1) continue;
        if(aBoneIds[i] >= 128) break;
        
        vec4 localPosition = u_FinalBonesMatrices[aBoneIds[i]] * vec4(aPos, 1.0f);
        totalPosition += localPosition * aWeights[i];
        hasBones = true;
    }
    
    if (!hasBones) totalPosition = vec4(aPos, 1.0f);

    mat4 modelMatrix = u_IsInstanced ? instanceMatrix : u_Model;
    
    // Use totalPosition for world space calculations
    gl_Position = camera.u_Projection * camera.u_View * modelMatrix * totalPosition;
}
```

---

## 11. Decal System
The engine supports a hybrid deferred/forward decal system for detailing surfaces.

- **Render Order**: Decals are sorted by `renderOrder` before drawing to ensure correct overlapping behavior.
- **Lifetime Management**: Decals can have a finite `lifetime`, after which they are automatically destroyed in order of their creation.
- **Tag Filtering**: (Deferred only) Decals can be masked to only affect surfaces with matching entity tags.

---

## See Also
- [Scene Format (.axs)](scene_format.md)
- [Architecture Overview](../core/architecture.md)
# Compressed textures

The built-in texture loader accepts 2D DDS files containing BC1, BC2, BC3,
BC4, or BC5 mip chains (legacy FourCC and DX10 headers). Compressed blocks are
uploaded directly; unsupported/corrupt formats fail through the normal strict
asset-loading policy. KTX2/Basis transcoding remains a provider concern rather
than being silently decoded as an ordinary image.
