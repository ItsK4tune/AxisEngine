# UI Components

UI components are used for 2D rendering overlay.

## UITransformComponent
Defines 2D rect on screen.
*   `glm::vec2 position`: Screen coordinates (pixels or percentage).
*   `glm::vec2 size`: Width/Height (pixels or percentage).
*   `bool usePercentage`: If true, `position` and `size` are treated as percentage of screen dimensions.
*   `glm::vec2 anchor`: Normalized anchor point (e.g., `0,0` is Top-Left, `0.5,0.5` is Center).
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
