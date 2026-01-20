# RenderSystem

**Include:** `<engine/ecs/system.h>`

Handles the 3D rendering pipeline, including shadowing, lighting, and model rendering.

## Responsibilities
*   **Shadow Maps**: Generates shadow maps for directional and point lights.
*   **Forward Rendering**: Renders `MeshRendererComponent` entities.
*   **Lights**: Uploads light data (UBO) to shaders.
*   **State Management**: Manages Depth Testing and Face Culling states.

## Public API
*   `void Render(Scene &scene)`: Main render pass.
*   `void RenderShadows(Scene &scene)`: Shadow map generation pass.
*   `void SetEnableShadows(bool enable)`: Toggles shadow casting.
*   `void SetInstanceBatching(bool enable)`: Toggles instance batching for static meshes. Batching reduces draw calls by combining multiple entities with the same model into a single draw call.
*   `void SetFaceCulling(bool enabled, int mode)`: Configures GL_CULL_FACE.
*   `void SetDepthTest(bool enabled, int func)`: Configures GL_DEPTH_TEST.
