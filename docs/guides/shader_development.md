# Shader Development Guide

This guide details how to write shaders that are compatible with the AXIS Engine's `RenderSystem`.

## 1. Engine Data Binding

The AXIS Engine automatically injects several uniforms and buffers into your shaders depending on the render pass and component configuration.

### Vertex Shader Inputs
Standard layouts expected by `Mesh` and `Model` classes:
- `layout (location = 0) in vec3 aPos;`
- `layout (location = 1) in vec3 aNormal;`
- `layout (location = 2) in vec2 aTexCoords;`
- `layout (location = 3) in vec3 aTangent;`
- `layout (location = 4) in vec3 aBitangent;`
- `layout (location = 5) in ivec4 aBoneIDs;` (Skeletal/Animated)
- `layout (location = 6) in vec4 aWeights;` (Skeletal/Animated)

### Standard Uniforms (Global)
- `mat4 projection`: Projection matrix.
- `mat4 view`: View matrix.
- `vec3 viewPos`: Camera position for lighting calculations.
- `vec4 tintColor`: Global color multiplier from `MeshRendererComponent`.

### Material Uniforms
The `RenderSystem` binds a `Material` struct:
```glsl
struct Material {
    sampler2D texture_diffuse1;  // Primary albedo map
    sampler2D texture_normal1;   // Normal map
    sampler2D texture_specular1; // Specular map (Phong)
    sampler2D texture_metallic1; // Metallic map (PBR)
    sampler2D texture_roughness1;// Roughness map (PBR)
    sampler2D texture_ao1;        // AO map (PBR)
    sampler2D texture_emission1; // Emission map
    
    float shininess;             // Phong
    vec3 specular;               // Phong
    float roughness;             // PBR
    float metallic;              // PBR
    float ao;                    // PBR
    vec3 emission;               // PBR/Phong
    float opacity;               // Transparency
};
uniform Material material;
```

## 2. Light Data (SSBO)

Lights are passed via Shader Storage Buffer Objects (SSBOs) to support a high number of lights.

### Binding points:
- `layout(std430, binding = 0) buffer DirLightBuffer`: Array of `DirLight`.
- `layout(std430, binding = 1) buffer PointLightBuffer`: Array of `PointLight`.
- `layout(std430, binding = 2) buffer SpotLightBuffer`: Array of `SpotLight`.

### Light counts:
- `uniform int numDirLights;`
- `uniform int nrPointLights;`
- `uniform int nrSpotLights;`

## 3. Shadow Mapping

If your shader supports shadows, you must handle:
- `uniform sampler2D shadowMapDir[4]`: Directional shadow maps.
- `uniform samplerCube shadowMapPoint[2]`: Point shadow maps.
- `uniform mat4 lightSpaceMatrix[4]`: Transformation to light space.
- `uniform bool u_ReceiveShadow`: Global flag to toggle shadow sampling.

## 4. Transparency and Blending

When `material.opacity < 1.0`, the `RenderSystem` automatically:
1. Moves the object to the **Transparent Queue**.
2. Sorts it **back-to-front**.
3. Enables OpenGL Blending (`GL_BLEND`).

**Best Practice**: Always multiply your final fragment alpha by `material.opacity`:
```glsl
void main() {
    // ... lighting calculations ...
    FragColor = vec4(resultColor, material.opacity);
}
```

## 5. Built-in Flags
- `uniform bool debug_noTexture`: Set when global "No Textures" debug mode is active.
- `uniform vec2 uvScale`: UV tiling multiplier.
- `uniform vec2 uvOffset`: UV panning offset.
