# UIRenderSystem & UIInteractSystem

**Include:** `<engine/ecs/system.h>`

Handles the 2D UI overlay.

## Responsibilities
*   **Rendering**: Draws quads and text on top of the 3D scene.
*   **Interaction**: Checks mouse intersection with UI Rects and triggers callbacks.
*   **Responsive Layout**: Supports percentage-based positioning and sizing relative to the current window dimensions.
*   **Anchoring**: Allows elements to be anchored to specific points (e.g., center, top-right) for dynamic resizing.
*   **Animation**: Interpolates UI scale/color for hover effects.

## Public API
*   `void UIRenderSystem::Render(Scene &scene, width, height)`
*   `void UIInteractSystem::Update(Scene &scene, dt, MouseManager)`
