# Debug & Editor Controls Guide

> [Tiếng Việt](../../vi/guides/debug_system.md)

AXIS does not currently provide a standalone `DebugSystem` class. Runtime diagnostics and toggles are handled by editor modules, `DebugConfig`, `ToolsPanel`, and the active systems that read those config values.

## Controls

| Key | Function | Description |
| :--- | :--- | :--- |
| **F1** | **Entity Names** | Toggles entity name labels. |
| **F2** | **Gizmos** | Toggles general transform/editor gizmos. |
| **F3** | **Light Gizmos** | Toggles light gizmo rendering. |
| **F4** | **Skybox** | Toggles skybox rendering. |
| **F5** | **Shadows** | Toggles shadow rendering. |
| **F6** | **Post Process** | Toggles post-processing when available. |
| **F7** | **Physics Debug** | Toggles Bullet collider debug drawing. |
| **F8** | **Audio Debug** | Toggles audio source debug markers. |
| **F9** | **Particle Debug** | Toggles particle emitter debug markers. |
| **F10** | **Editor Cursor** | Toggles editor cursor ownership. |
| **Shift+F10** | **Debug Camera** | Toggles the free/debug camera. |
| **F11** | **Pause Game** | Pauses or resumes the engine loop. |
| **F12** | **Time Scale** | Cycles time scale between slow motion and normal/fast values. |

## Panels

The `HelpPanel` mirrors the current shortcut list. The `ToolsPanel` exposes the runtime toggles through ImGui controls. Audio-system and UI-render-system enable/disable controls remain in the panel but intentionally have no function-key shortcut.

## Visual Debugging

- **Physics Debug**: Renders Bullet collision shapes through the Bullet debug drawer.
- **Gizmos**: Draws transform, light, grid, and other editor visual aids when their config flags are enabled.
- **Debug Camera**: Uses editor camera modules rather than a separate global debug system.
