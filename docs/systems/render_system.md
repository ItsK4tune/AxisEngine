# RenderSystem

**Include:** `<engine/ecs/system.h>`

Handles the 3D rendering pipeline, including shadowing, lighting, and model rendering.

## Responsibilities
*   **Shadow Maps**: Generates shadow maps for directional and point lights with support for multiple directional shadows (up to 4).
*   **Forward Rendering**: Renders `MeshRendererComponent` entities.
*   **Lights**: Uploads light data (SSBO) to shaders.
*   **State Management**: Manages Depth Testing and Face Culling states.
*   **Bucket Rendering**: Implements a layered rendering approach by clearing the depth buffer between groups of entities with different `RenderOrder`.
*   **Layer Filtering**: Provides global and camera-specific visibility masks for entities.
*   **Transparency Pass**: Implements a dedicated rendering pass for transparent objects with back-to-front sorting and alpha blending.
*   **Material Overrides**: Supports overriding model textures with custom textures defined in `MaterialComponent`.

## Rendering Hierarchy

The engine uses the following priority rules to determine which pixels are drawn on top:

1.  **Entity Type**: **Opaque** objects are always rendered first, and **Transparent** objects (opacity < 1.0) are rendered last.
    *   *Result: Transparent objects will always appear in front of opaque objects unless occluded.*
2.  **Render Order**: Within the same group (Opaque or Transparent), entities with a **higher** `Order` value are rendered later.
    *   *Result: Order 2 renders on top of Order 1.*
3.  **Depth**: 
    *   Opaque: Uses Depth Testing (closer objects are drawn).
    *   Transparent: Automatically sorted **Back-to-Front** (by distance to camera) to ensure correct alpha blending.

## Anti-Aliasing
The RenderSystem supports multiple Anti-Aliasing techniques to reduce jagged edges:

### Supported Modes
- **NONE (0)**: No anti-aliasing.
- **FXAA (1)**: Fast Approximate Anti-Aliasing. Low cost, effectively smooths edges but may slightly blur textures.
- **TAA (2)**: Temporal Anti-Aliasing. High quality, resolves sub-pixel detail using jittering and history. Can cause slight ghosting on fast moving objects.

## Shadow System

The RenderSystem supports three shadow rendering modes:

### Shadow Modes
- **Mode 0 (None)**: No shadows are rendered. All shadow calculations are skipped.
- **Mode 1 (Once)**: Renders shadow from the first directional light with `isCastShadow = true`. This is the default and most performant mode.
- **Mode 2 (All)**: Renders shadows from up to 4 directional lights with `isCastShadow = true`. If more than 4 lights have shadow casting enabled, only the first 4 will cast shadows and a warning will be logged.

### Shadow Casting Lights
Lights can be configured to cast shadows using the `isCastShadow` property. The direction and position of lights are determined by their **TransformComponent**:
- **DirectionalLightComponent**: Uses `TransformComponent` rotation (Default forward: `(0, -1, 0)`). Set `isCastShadow = true` (Max 2).
- **PointLightComponent**: Uses `TransformComponent` position. Set `isCastShadow = true` (Max 2).
- **SpotLightComponent**: Uses `TransformComponent` position and rotation. Set `isCastShadow = true` (Max 2).

> **Note**: In shadow mode 1 (Once), only the first light with `isCastShadow = true` will cast shadows, regardless of how many lights have this property enabled.

## Public API
*   `void Render(Scene &scene)`: Main render pass.
*   `void RenderShadows(Scene &scene)`: Shadow map generation pass.
*   `void SetEnableShadows(bool enable)`: Toggles shadow casting on/off.
*   `void SetShadowMode(int mode)`: Sets shadow rendering mode (0=None, 1=Once, 2=All).
*   `int GetShadowMode() const`: Returns current shadow mode.
*   `void SetInstanceBatching(bool enable)`: Toggles instance batching for static meshes. Batching reduces draw calls by combining multiple entities with the same model into a single draw call.
*   `void SetFaceCulling(bool enabled, int mode)`: Configures GL_CULL_FACE.
*   `void SetDepthTest(bool enabled, int func)`: Configures GL_DEPTH_TEST.
*   `void SetShadowProjectionSize(float size)`: Sets orthographic projection size for directional shadows.
*   `void SetShadowFrustumCulling(bool enable)`: Enables/disables frustum culling for shadow rendering.
*   `void SetShadowDistanceCulling(float distance)`: Sets maximum distance for shadow casting.

*   `void SetFilterLayerMask(uint32_t mask)`: Sets the global layer filter mask (bitwise AND with entity layer).
*   `void SetRenderOrderEnabled(bool enable)`: Enables/disables bucket rendering based on `MeshRendererComponent::order`.

*   `void SetAntiAliasingMode(AntiAliasingMode mode)`: Sets AA mode (NONE, FXAA, TAA).

## Configuration
Anti-Aliasing can be configured via `.axs` files:
```yaml
axis_scene:
  Config:
    ANTIALIASING: TAA
```

### Transparency Requirements
Shaders that support transparency must:
1. Accept `Material` struct with `float opacity` field.
2. Use `material.opacity` to calculate the final `FragColor.a`.
3. Optionally handle `material.alphaCutoff` for masked transparency.

See `phong_lit_shadow.fs` and `pbr_lit_shadow.fs` for reference implementations.
