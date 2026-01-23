# RenderSystem

**Include:** `<engine/ecs/system.h>`

Handles the 3D rendering pipeline, including shadowing, lighting, and model rendering.

## Responsibilities
*   **Shadow Maps**: Generates shadow maps for directional and point lights with support for multiple directional shadows (up to 4).
*   **Forward Rendering**: Renders `MeshRendererComponent` entities.
*   **Lights**: Uploads light data (SSBO) to shaders.
*   **State Management**: Manages Depth Testing and Face Culling states.

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

## Shader Requirements

Shaders that support shadows must:
1. Accept `lightSpaceMatrix[4]` uniform array for directional light space transformations
2. Accept `shadowMapDir[4]` uniform sampler2D array for directional shadow maps
3. Accept `u_ReceiveShadow` uniform bool to enable/disable shadow reception
4. Calculate shadows for each active directional light using the corresponding shadow map index

See `phong_lit_shadow.fs` and `pbr_lit_shadow.fs` for reference implementations.
