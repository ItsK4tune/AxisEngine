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
*   **Direction**: Defined by the entity's `TransformComponent` rotation (Forward: `(0, -1, 0)`).
*   `color` (vec3): Base color.
*   `intensity` (float): Brightness multiplier.
*   `active` (bool): Enable/Disable light.
*   `isCastShadow` (bool): Casts dynamic shadows.
*   **Lighting**: `ambient`, `diffuse`, `specular` (vec3).

### PointLightComponent
Omni-directional light (Lamp).
*   **Position**: Defined by the entity's `TransformComponent` position.
*   `color` (vec3)
*   `intensity` (float)
*   `radius` (float)
*   `active` (bool)
*   `isCastShadow` (bool)
*   `constant`, `linear`, `quadratic`: Attenuation factors.
*   **Lighting**: `ambient`, `diffuse`, `specular` (vec3).

### SpotLightComponent
Cone light (Flashlight).
*   **Position & Direction**: Defined by the entity's `TransformComponent` position and rotation.
*   `color`, `intensity`
*   `active` (bool)
*   `isCastShadow` (bool)
*   `cutOff`, `outerCutOff`: Cone angles (in cosines).
*   **Lighting**: `ambient`, `diffuse`, `specular` (vec3).

## SkyboxRenderComponent
**Struct:** `SkyboxRenderComponent`

*   `Skybox* skybox`: Pointer to cubemap texture data.
*   `Shader* shader`: Skybox shader.
