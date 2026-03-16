# User Interface (UI) Guide

AXIS Engine provides a responsive 2D overlay system for menus, HUDs, and interactive elements.

---

## 1. UI Components

### UITransformComponent
Defines the bounding box and layout of a UI element using a resolution-independent anchoring system.
- **Anchors (`anchorMin`, `anchorMax`)**: Normalized points (0.0 to 1.0) in the parent's space that define the element's origin and stretching behavior.
- **Offsets (`offsetMin`, `offsetMax`)**: Pixel offsets from the calculated anchor points.
- **Pivot**: The point (0.0 to 1.0) within the element about which it rotates and scales.
- **ZOrder**: Determines the rendering stack; higher values appear on top.

### UIRendererComponent
Draws a colored quad or textured image.
- Supports tinting and custom UI shaders.

### UITextComponent
Renders dynamic text using pre-loaded fonts.
- **Alignment**: `Left`, `Center`, `Right`.
- **Word Wrap**: Enable `wordWrap` and set `maxWidth` to constrain text within a specific pixel width.

### UIFlexLayoutComponent
Provides automated horizontal or vertical arrangement of child elements.
- **Direction**: `Row` or `Column`.
- **Spacing**: Pixel gap between elements.
- **Padding**: Internal spacing (`Left`, `Top`, `Right`, `Bottom`).

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
