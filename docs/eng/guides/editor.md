# Editor Manual (`Axis::Editor`)

> [Tiếng Việt](../../vi/guides/editor.md) | [Editor Extensions](editor_extensions.md) | [Debug System](debug_system.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

`Axis::Editor` is an integrated **ImGui-based visual editor** compiled when `ENABLE_EDITOR=ON`. It provides hierarchy browsing, component inspectors, viewport gizmos, asset browsing, profiler timing metrics, and simulation controls.

---

## 2. How to Use

1. **Enable Editor Build**: Configure CMake with `ENABLE_EDITOR=ON`.
2. **Register Editor System**: Register `EditorSystem` into the engine's `SystemRegistry`.
3. **Inspect & Edit**: Click entities in Hierarchy to modify components in Inspector; use viewport 3D gizmos to translate/rotate/scale.
4. **Play / Pause / Stop**: Use top toolbar buttons to test game simulation.

---

## 3. Examples

### Launching Reference Editor Host Example
```cpp
#include <axis_sdk.h>

#if defined(AXIS_ENABLE_EDITOR)
#include <axis_editor.h>
#endif

int main() {
    auto app = std::make_shared<Application>();
    AppConfig config;
    config.title = "AxisEngine Editor Host";

    if (!app->Initialize(config)) return 1;

    #if defined(AXIS_ENABLE_EDITOR)
    app->GetSystemRegistry().RegisterSystem<EditorSystem>();
    #endif

    app->Run();
    return 0;
}
```

---

## 4. API & Configuration Reference

### Editor Panels Reference Matrix

| Editor Panel Name | Purpose & Capabilities |
| :--- | :--- |
| **Hierarchy Panel** | Scene entity tree browsing, entity creation, reparenting, deletion |
| **Inspector Panel** | Live component property editing, material tweaks, script attachment |
| **Project Browser** | Workspace file system tree, asset previewing, scene loading |
| **Viewport Window** | 3D render view with Translation, Rotation, and Scaling gizmos |
| **Tools Panel** | Live renderer toggles (shadows, post-processing, physics wireframes) |
| **Profiler Panel** | Real-time CPU/GPU frame timing breakdown and draw call counts |
| **Console Panel** | Filterable log output console (`VERBOSE`, `INFO`, `WARN`, `ERROR`) |
| **Play Controls** | Engine simulation state switcher (**Play**, **Pause**, **Stop**) |
