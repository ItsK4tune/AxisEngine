# Debug System Guide

The `DebugSystem` provides real-time introspection, control, and diagnostics for the AXIS Engine. It is enabled when `ENABLE_DEBUG_SYSTEM` is defined (default in Debug builds).

## Controls (Hotkeys)

| Key | Function | Description |
| :--- | :--- | :--- |
| **F1** | **Log Controls** | Prints this list of debug keys to the console. |
| **F2** | **Log Devices** | Lists all detected hardware (GPU, CPU, Monitors, Inputs, Audio). Active devices are marked with `[*]`. |
| **F3** | **Performance Stats** | Logs current FPS and Frame Time (ms). |
| **F4** | **Entity Stats** | Logs count of Entities, Renderables, UI Elements, and Physics Objects. |
| **F5** | **Scene Dump** | Logs the hierarchy of the current Scene (Scene Graph). |
| **F6** | **Wireframe Mode** | Toggles between Fill (Solid) and Line (Wireframe) rendering modes. |
| **F7** | **No Texture Mode** | Toggles "Clay Mode" (White textures) for all objects. Useful for checking lighting/geometry. |
| **F8** | **Physics Debug** | Toggles visual debugging of physics colliders (Wireframes). |
| **F9** | **Toggle UI** | Hides or Shows the UI layer (2D Overlay). |
| **F10** | **Stats Overlay** | Toggles an on-screen overlay showing FPS and Entity Count. |
| **F11** | **Pause Game** | Pauses logic and physics updates. Rendering continues (useful for inspecting visual states). |
| **F12** | **Time Scale** | Cycles time dilation: `0.25x` -> `0.5x` -> `1.0x` -> `1.5x` -> `2.0x`. |
| **Shift+F3** | **Entity Names** | Toggle Entity Name tags above objects. |
| **Shift+F4** | **Transform Gizmos** | Toggle Transform Gizmos (Position/Rotation/Scale visualizers). |
| **Shift+F5** | **Light Gizmos** | Toggle Light Gizmos (Dir/Point/Spot icons). |
| **Shift+F6** | **Toggle Skybox** | Enable or Disable Skybox rendering. |
| **Shift+F7** | **Toggle Shadows** | Globally enables or disables shadow mapping. |
| **Shift+F11** | **Toggle Debug Camera** | Switches between Main Camera and Free/Debug Camera (WASD "NoClip"). |
| **Shift+F12** | **Cycle Cursor Mode** | Cycles: Normal -> Hidden -> Locked (Clamped) -> LockedHidden -> LockedCenter -> LockedHiddenCenter. |

## Features Detail

### 1. Visual Debugging (F6, F7, F8)
*   **Wireframe (F6)**: Uses `glPolygonMode` to see mesh topology.
*   **No Texture (F7)**: Replaces all material textures with a 1x1 white texture. This works on PBR, Phong, and Unlit shaders.
    *   **Shadow Toggle (Shift+F7)**: Globally enables/disables shadow mapping.
*   **Physics Draw (F8)**: Renders the internal Bullet Physics collision shapes. Green = Static, Red = Dynamic (usually).

### 2. Time Control (F11, F12)
*   **Pause (F11)**: Freezes `deltaTime` for logic/physics systems. Camera movement may still work if independent of game time.
*   **Slow Motion (F12)**: Adjusts `deltaTime` multiplier.
    *   **Note**: Safety limits are in place to prevent "spiral of death" (physics engine hanging due to large time steps).

### 3. Diagnostics (F2, F3, F4, F5)
*   Use these keys to dump information to the console window.
*   **F2** is particularly useful for verifying which Monitor or Audio Device is currently selected by the configuration.

### 4. On-Screen Overlay (F10)
*   Displays real-time stats in the top-right corner.
*   Requires `resources/fonts/time.ttf` and valid UI shaders.
*   Includes: FPS, Frame Time, Total Entities, Rendered Count.
