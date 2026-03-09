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

### 6. Shader "Gate" (Global Data)
The engine provides a global `UBO` at **binding slot 2** for system constants. Declare it in your shader like this:
```glsl
layout (std140, binding = 2) uniform GlobalData {
    float u_Time;
    float u_DeltaTime;
    vec2  u_Resolution;
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
Accessible via `m_App->GetRenderSystem()`.

- `Render(scene)`: Executes the main pass.
- `RenderShadows(scene)`: Generates shadow maps.
- `SetFaceCulling()`, `SetDepthTest()`: Manual state management.
- `SetFilterLayerMask()`: Global visibility control.

---

## 9. Skeletal Animation (Skinning)
For animated models, you must handle bone weights and matrices in the vertex shader. Use the snippet below:

### Vertex Shader Skinning Snippet
```glsl
#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
// Bone data at locations 5 and 6
layout (location = 5) in ivec4 aBoneIds; 
layout (location = 6) in vec4 aWeights;

uniform mat4 model;
uniform mat4 finalBonesMatrices[200]; // Max 200 bones
uniform bool isInstanced;
layout(location = 10) in mat4 instanceMatrix;

void main() {
    vec4 totalPosition = vec4(0.0f);
    bool hasBones = false;
    
    for(int i = 0 ; i < 4 ; i++) {
        if(aBoneIds[i] == -1) continue;
        if(aBoneIds[i] >= 200) break;
        
        vec4 localPosition = finalBonesMatrices[aBoneIds[i]] * vec4(aPos, 1.0f);
        totalPosition += localPosition * aWeights[i];
        hasBones = true;
    }
    
    if (!hasBones) totalPosition = vec4(aPos, 1.0f);

    mat4 modelMatrix = isInstanced ? instanceMatrix : model;
    
    // Use totalPosition for world space calculations
    gl_Position = camera.projection * camera.view * modelMatrix * totalPosition;
}
```

---

## See Also
- [Scene Format (.axs)](scene_format.md)
- [Architecture Overview](../core/architecture.md)
