# User Interface (UI) System Guide

> [Tiếng Việt](../../vi/guides/ui.md) | [Components Reference](components_reference.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine features a hardware-accelerated **Canvas-based UI System** for creating 2D HUDs, interactive menus, health bars, and text overlays. Layouts rely on `RectTransform` anchor mathematics to support resolution-independent scaling.

---

## 2. How to Use

1. **Creating UI Entities**: Create a root Canvas entity and child UI element entities.
2. **Configuring Layout**: Add `RectTransformComponent` to set anchors (`anchorMin`, `anchorMax`), `sizeDelta`, and `pivot`.
3. **Adding Visual Components**: Attach `UIImageComponent` for textures/panels and `UITextComponent` for string labels.
4. **Handling Clicks**: Attach `UIButtonComponent` and set the `onClick` std::function callback.

---

## 3. Examples

### Creating an Interactive UI Button Example
```cpp
#include <axis_sdk.h>

void CreateUIButton(Scene& scene) {
    auto canvas = scene.CreateEntity("Canvas");
    auto button = scene.CreateEntity("Play Button");

    auto& rect = button.AddComponent<RectTransformComponent>();
    rect.anchorMin = Vector2(0.5f, 0.5f);
    rect.anchorMax = Vector2(0.5f, 0.5f);
    rect.sizeDelta = Vector2(200.0f, 50.0f);

    auto& image = button.AddComponent<UIImageComponent>();
    image.color = Vector4(0.1f, 0.5f, 0.9f, 1.0f);

    auto& text = button.AddComponent<UITextComponent>();
    text.text = "PLAY NOW";

    auto& btn = button.AddComponent<UIButtonComponent>();
    btn.onClick = []() {
        AXIS_LOG_INFO("Play Button Clicked!");
    };
}
```

---

## 4. API & Configuration Reference

### UI Components Reference Table

| Component Name | Key Properties | Purpose |
| :--- | :--- | :--- |
| `RectTransformComponent` | `anchorMin`, `anchorMax`, `anchoredPosition`, `sizeDelta`, `pivot` | 2D screen-space bounds and anchor layout math |
| `UIImageComponent` | `texturePath`, `color` | Renders textured sprites or solid colored panels |
| `UITextComponent` | `text`, `fontSize`, `color` | Renders TrueType font text strings |
| `UIButtonComponent` | `onClick` (`std::function<void()>`) | Handles mouse hover and click interaction callbacks |
