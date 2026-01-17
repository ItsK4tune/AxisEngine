# Debug System Guide

The **Debug System** provides real-time insights into the engine's performance and state. It is conditionally compiled and can be enabled/disabled via the build script.

## Enabling the Debug System

When running `build_engine.bat`, select option `1. Enable Debug System` when prompted.

## Features & Controls

The Debug System offers several overlays and logging tools controlled by Function keys:

| Key | Feature | Description |
| :--- | :--- | :--- |
| **F1** | **Physics Debug** | Toggles wireframe rendering of physics colliders. Useful for verifying collision shapes. |
| **F2** | **Device Logs** | Logs detailed information about connected Input devices (Keyboard, Mouse), Audio devices, and Monitors to the console. |
| **F3** | **Performance Log** | Logs current FPS and Frame Time to the console. |
| **F4** | **Entity Stats Log** | Logs the total number of alive entities and the number of entities currently being rendered. |
| **F12** | **Stats Overlay** | Toggles an on-screen overlay displaying FPS, Frame Time, Total Entities, and Rendered Entities. |

## Implementation Details

The Debug System is implemented as a standalone system (`DebugSystem`) initialized in `Application`. It uses a dedicated UI Model (`debug_sys_model`) to prevent conflicts with game UI elements.

- **Source**: `src/engine/core/debug_system.cpp`
- **Header**: `includes/engine/core/debug_system.h`

> [!NOTE]
> The Debug System is excluded from Release builds if the `ENABLE_DEBUG_SYSTEM` flag is not set.
