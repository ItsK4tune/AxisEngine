# Debug Tools, Logging & Diagnostics Guide

> [Tiếng Việt](../../vi/guides/debug_system.md) | [Editor Manual](editor.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine provides a comprehensive suite of debugging tools, loggers, runtime assertion diagnostics, and ImGui debug GUI panels. The debug subsystem enables developers to catch contract violations via assertions (`AXIS_ASSERT`), record severity-filtered log traces (`AXIS_LOG_*`), inspect live render states (`ToolsPanel`), monitor performance metrics (`ProfilerPanel`), and visualize physics colliders.

---

## 2. How to Use

1. **Logging**: Use `AXIS_LOG_INFO("msg")`, `AXIS_LOG_WARN("msg")`, `AXIS_LOG_ERROR("msg")`, `AXIS_LOG_DEBUG("msg")`, or `AXIS_LOG_VERBOSE("msg")`.
2. **Runtime Assertions**: Insert `AXIS_ASSERT(ptr != nullptr, "Pointer must not be null!")` to enforce invariant contracts in Debug/Release builds.
3. **Keyboard Shortcuts**: Use F1-F12 to toggle entity names, gizmos, light bounds, shadows, post-processing, physics wireframes, debug camera, pause, and time scale.
4. **Debug GUI Panels**: Open `ToolsPanel` for live toggles, `ProfilerPanel` for FPS/draw calls, and `ConsolePanel` for live log output.

---

## 3. Examples

### 1. `AXIS_LOG_*` Logging & `AXIS_ASSERT` Diagnostics Example
```cpp
#include <axis_sdk.h>

void ProcessEntityDamage(Entity player, int damageAmount) {
    // 1. Enforce invariant contract with AXIS_ASSERT
    AXIS_ASSERT(damageAmount >= 0, "Damage amount cannot be negative!");

    // 2. Structured logging by severity level
    AXIS_LOG_DEBUG("ProcessEntityDamage called for entity ID: " + std::to_string(static_cast<uint32_t>(player)));

    if (damageAmount > 50) {
        AXIS_LOG_WARN("Heavy damage taken: " + std::to_string(damageAmount) + " HP!");
    } else {
        AXIS_LOG_INFO("Player took " + std::to_string(damageAmount) + " damage.");
    }
}
```

### 2. Scope Profiling Example
```cpp
#include <axis_sdk.h>

void ProfileHeavyTask() {
    AXIS_PROFILE_SCOPE("ProfileHeavyTask");

    for (int i = 0; i < 1000; ++i) {
        // Computational work
    }
}
```

---

## 4. API & Configuration Reference

### Logging Macros Reference (`AXIS_LOG_*`)

| Macro Name | Severity Level | Output Purpose |
| :--- | :--- | :--- |
| `AXIS_LOG_VERBOSE(msg)` | `VERBOSE` | High-frequency trace messages and verbose diagnostics |
| `AXIS_LOG_DEBUG(msg)` | `DEBUG` | Developer debugging information during development |
| `AXIS_LOG_INFO(msg)` | `INFO` | General operational milestone events and informational status |
| `AXIS_LOG_WARN(msg)` | `WARNING` | Non-fatal warnings, missing fallback assets, performance hints |
| `AXIS_LOG_ERROR(msg)` | `ERROR` | Fatal failures, missing required resources, hardware errors |

### Assertions & Diagnostic Macros Reference

| Diagnostic Macro | Contract Behavior |
| :--- | :--- |
| `AXIS_ASSERT(condition, message)` | Validates condition expression; if false, logs error trace with file/line info and triggers breakpoint in debug build |

### Keyboard Shortcuts Reference

| Shortcut | Function | Description |
| :--- | :--- | :--- |
| **F1** | Entity Names | Toggle 3D floating entity label overlays |
| **F2** | Gizmos | Toggle transform gizmos |
| **F3** | Light Gizmos | Toggle light source icons |
| **F4** | Skybox | Toggle skybox rendering |
| **F5** | Shadows | Toggle dynamic shadow map rendering |
| **F6** | Post Process | Toggle HDR, Bloom, FXAA/TAA |
| **F7** | Physics Debug | Toggle Bullet 3D collision shape wireframes |
| **F8** | Audio Debug | Toggle 3D audio emitter radius spheres |
| **F9** | Particle Debug | Toggle particle emitter bounding boxes |
| **F10** | Editor Cursor | Toggle cursor capture between game viewport and GUI |
| **Shift+F10** | Debug Camera | Toggle free flying debug camera |
| **F11** | Pause Game | Pause or resume engine tick loop |
| **F12** | Time Scale | Cycle simulation speed (0.1x, 1.0x, 2.0x) |
