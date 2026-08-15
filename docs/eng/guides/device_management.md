# Device & Display Management Guide

> [Tiếng Việt](../../vi/guides/device_management.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine provides native multi-monitor display and input device management via GLFW. The subsystem controls display presentation modes (Fullscreen, Windowed, Borderless), monitor index selection, VSync throttling, and gamepad input hot-plugging.

---

## 2. How to Use

1. **Accessing Window Object**: Retrieve window handle via `Application::Get().GetWindow()`.
2. **Querying Monitors**: Call `window.GetMonitorCount()`.
3. **Switching Modes**: Call `window.SetDisplayMode(mode, width, height, monitorIndex)`.

---

## 3. Examples

### Switching Monitor Resolution Example
```cpp
#include <axis_sdk.h>

void SetSecondaryDisplay() {
    auto& window = Application::Get().GetWindow();
    if (window.GetMonitorCount() > 1) {
        window.SetDisplayMode(WindowMode::FULLSCREEN, 1920, 1080, 1);
    }
}
```

---

## 4. API & Configuration Reference

### Display Parameters Reference Table

| Property / Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `MONITOR` | `int` | `0` | Monitor target index (`0` for primary monitor) |
| `WINDOW_MODE` | `Enum` | `BORDERLESS_FULLSCREEN` | Presentation mode (`WINDOWED`, `FULLSCREEN`, `BORDERLESS`) |
| `REFRESH_RATE` | `int` | `60` | Monitor refresh rate target in Hz |
| `RENDER_SCALE` | `float` | `1.0` | Internal rendering resolution multiplier (`0.5` to `2.0`) |
