# User Interface (UI) Guide

AXIS Engine provides a responsive 2D overlay system for menus, HUDs, and interactive elements.

---

## 1. UI Components

### UITransformComponent
Defines the bounding box and layout of a UI element.
- **Position & Size**: Can be defined in raw pixels or percentages of the window dimensions.
- **Anchor**: Normalized point (0.0 to 1.0) for positioning. `(0.5, 0.5)` anchors the element to its center.
- **ZOrder**: Determines the rendering stack; higher values appear on top of lower ones.

### UIRendererComponent
Draws a colored quad or textured image to the screen.
- Supports tinting and custom UI shaders.

### UITextComponent
Renders dynamic text using pre-loaded FreeType fonts.
- **Properties**: `Text`, `Font`, `Color`, `Scale`.

---

## 2. Interaction & Animation

### UIInteractiveComponent
Enables mouse interaction for buttons and sliders.
- **States**: `isHovered`, `isPressed`.
- **Callbacks**: Hook into `onClick` or `onHoverEnter` via scripting to trigger game logic.

### UIAnimationComponent
Provides lightweight visual feedback.
- Automatically interpolates between `normalColor` and `hoverColor` when the entity is hovered.
- Can be used to scale elements smoothly during interaction.

---

## 3. Responsive Layout
The UI system is designed to handle dynamic window resizing:

- **Percentage Scaling**: Enable `UsePercentage` to keep elements size-relative to the screen (e.g., a map that always takes up 20% of the viewport).
- **Anchoring**: Ensures UI elements stay "stuck" to screen corners or centers regardless of aspect ratio changes.

---

## 4. UI Systems
Two systems manage the UI every frame:

1.  **UIInteractSystem**: Processes mouse positions and clicks, updating the state of `UIInteractiveComponent` and triggering user callbacks.
2.  **UIRenderSystem**: Renders all UI elements as a late pass on top of the 3D scene, ensuring the UI is never occluded by game objects.

---

## See Also
- [Graphics Guide](graphics.md)
- [Scriptable API](../scripting/scriptable_api.md)
- [Scene Format (.axs)](scene_format.md)
