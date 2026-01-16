# UI Components

UI components are used for 2D rendering overlay.

## UITransformComponent
Defines 2D rect on screen.
*   `glm::vec2 position`: Screen coordinates (pixels).
*   `glm::vec2 size`: Width/Height.
*   `int zIndex`: Rendering order.

## UIRendererComponent
Renders a colored 2D quad/image.
*   `UIModel* model`
*   `Shader* shader`
*   `glm::vec4 color`

## UITextComponent
Renders text strings.
*   `std::string text`
*   `Font* font`
*   `glm::vec3 color`
*   `float scale`

## UIAnimationComponent
Simple hover animations.
*   `bool isAnimating`
*   `float targetScale`
*   `glm::vec4 hoverColor`
*   `glm::vec4 normalColor`

## UIInteractiveComponent
Handles mouse interaction.
*   `bool isHovered`
*   `bool isPressed`
*   `std::function<void(entity)> onClick`
*   `std::function<void(entity)> onHoverEnter`
