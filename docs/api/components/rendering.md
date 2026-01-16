# Rendering Components

## MeshRendererComponent
**Struct:** `MeshRendererComponent`

Renders a 3D model.
*   `Model* model`: Pointer to the loaded model.
*   `Shader* shader`: Pointer to the shader used for rendering.
*   `bool castShadow`: If true, renders to shadow map.
*   `glm::vec4 color`: Base tint color.

## MaterialComponent
**Struct:** `MaterialComponent`

Defines the surface properties of the mesh.

*   **Type**: `PHONG` or `PBR`.
*   **Common**: `roughness`, `opacity`, `emission`, `ao` (Ambient Occlusion).
*   **Phong**: `shininess`, `specular` color, `ambient` color.
*   **PBR**: `metallic` factor.

## Light Components

### DirectionalLightComponent
Global light source (Sun).
*   `direction` (vec3)
*   `color` (vec3)
*   `intensity` (float)

### PointLightComponent
Omni-directional light (Lamp).
*   `color` (vec3)
*   `intensity` (float)
*   `radius` (float)
*   `constant`, `linear`, `quadratic`: Attenuation factors.

### SpotLightComponent
Cone light (Flashlight).
*   `color`, `intensity`
*   `cutOff`, `outerCutOff`: Cone angles (in cosines).

## SkyboxRenderComponent
**Struct:** `SkyboxRenderComponent`

*   `Skybox* skybox`: Pointer to cubemap texture data.
*   `Shader* shader`: Skybox shader.
