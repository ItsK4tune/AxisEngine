# Rendering Components

## MeshRendererComponent
**Struct:** `MeshRendererComponent`

Renders a 3D model.
*   `Model* model`: Pointer to the loaded model resource.
*   `Shader* shader`: Pointer to the shader resource.
*   `bool castShadow`: If true, this entity will be rendered into shadow maps. (AXS: `CastShadow`)
*   `glm::vec4 color`: Base tint color applied to the shader (`tintColor` uniform). (AXS: `Color`)
*   `int order`: Rendering priority bucket. (AXS: `Order`). Higher values render later (on top).

> **Note on Hierarchy**: **Transparent** objects (opacity < 1.0) always render after (on top of) **Opaque** objects. Within each group, objects with a higher `Order` render later.

## MaterialComponent
**Struct:** `MaterialComponent`

Defines the surface properties of the mesh.


*   **Type**: `PHONG` or `PBR`.
*   **Properties**:
    *   `opacity` (float): Degree of transparency (0.0 to 1.0).
    *   `alphaCutoff` (float): Threshold for pixel discarding (used for cutout transparency).
    *   `blendSrc`, `blendDst`: Blending factors (e.g., `SrcAlpha`, `OneMinusSrcAlpha`).
*   **Textures (Overrides)**:
    *   `albedoPath`, `normalPath`, `metallicPath`, `roughnessPath`, `aoPath`, `emissivePath`: File paths to texture assets that override model-defined textures.
    *   `albedoMap`, `normalMap`, etc. (Internal): Texture IDs loaded from the paths above.
*   **Phong Specific**: `shininess`, `specular` (vec3), `ambient` (vec3).
*   **PBR Specific**: `roughness` factor, `metallic` factor, `ao` factor, `emission` (vec3).
*   **UV**: `uvScale`, `uvOffset`.

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

## AnimationComponent
**Struct:** `AnimationComponent`

Enables skeletal animation for characters and objects.
*   `std::vector<std::string> animations`: List of animation resource names.
*   `float speed`: Playback speed multiplier.
*   `float startTime`: Initial time offset.
*   `float rate`: Update frequency (Hz).
*   **AXS Example**:
    ```yaml
    Component: Animator
      Animation: idle walk run
      Speed: 1.0
      Rate: 30.0
    ```

## LODComponent
**Struct:** `LODComponent`

Swaps models based on distance to the active camera.
*   `std::vector<Model*> lodModels`: List of models for different detail levels.
*   `std::vector<float> lodDistancesSq`: Squared distance thresholds for each level.
*   **AXS Example**:
    ```yaml
    Component: LOD
      Models: highPolyModel medPolyModel lowPolyModel
      Distances: 10 30 100
    ```

## SkyboxRenderComponent
**Struct:** `SkyboxRenderComponent`

*   `Skybox* skybox`: Pointer to cubemap texture data.
*   `Shader* shader`: Skybox shader.
