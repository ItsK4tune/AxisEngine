# Editor Extensions Guide

> [Tiếng Việt](../../vi/guides/editor_extensions.md) | [Editor Manual](editor.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine allows developers to write custom ImGui editor windows, wizard panels, and inspector extensions. Custom extensions inherit from `IEditorPanel` and register with `EditorSystem` to draw during the editor render phase.

---

## 2. How to Use

1. **Subclass `IEditorPanel`**: Inherit from `IEditorPanel` and implement `void OnImGuiRender() override`.
2. **Add ImGui Widgets**: Write ImGui code inside `OnImGuiRender()` (`ImGui::Begin`, `ImGui::Button`, etc.).
3. **Register Panel**: Call `editorSystem.RegisterPanel<MyPanel>("Window Title")`.

---

## 3. Examples

### Custom ImGui Editor Panel Example
```cpp
#include <axis_sdk.h>
#include <axis_editor.h>
#include <imgui.h>

class LevelGeneratorPanel final : public IEditorPanel {
private:
    int m_gridSize = 10;

public:
    void OnImGuiRender() override {
        ImGui::Begin("Level Generator");
        ImGui::SliderInt("Grid Size", &m_gridSize, 5, 50);

        if (ImGui::Button("Generate Tiles")) {
            AXIS_LOG_INFO("Generating level grid tiles...");
        }
        ImGui::End();
    }
};

void RegisterExtension(EditorSystem& editor) {
    editor.RegisterPanel<LevelGeneratorPanel>("Level Generator");
}
```

---

## 4. API & Configuration Reference

### Editor Extension API Reference Table

| Interface / Class | Key Method | Purpose |
| :--- | :--- | :--- |
| `IEditorPanel` | `virtual void OnImGuiRender() = 0` | Base interface for custom editor windows |
| `EditorSystem` | `RegisterPanel<T>(title)` | Registers panel instance into main editor dockspace |
