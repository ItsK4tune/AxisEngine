# Debug & Editor Controls Guide

AXIS does not currently provide a standalone `DebugSystem` class. Runtime diagnostics and toggles are handled by editor modules, `DebugConfig`, `ToolsPanel`, and the active systems that read those config values.

## Controls

| Key | Function | Description |
| :--- | :--- | :--- |
| **F1** | **Entity Names** | Toggles entity name labels. |
| **F2** | **Gizmos** | Toggles general transform/editor gizmos. |
| **F3** | **Light Gizmos** | Toggles light gizmo rendering. |
| **F4** | **Audio System** | Toggles the audio system when available. |
| **F5** | **Post Process System** | Toggles post-processing when available. |
| **F6** | **Force Free Cursor** | Releases the cursor for editor panel interaction. |
| **F8** | **Physics Debug** | Toggles Bullet collider debug drawing. |
| **F9** | **UI Rendering** | Toggles UI render output. |
| **F10** | **Game Exit Hook** | Used by game states such as match/result flows. |
| **F11** | **Pause Game** | Pauses or resumes the engine loop. |
| **F12** | **Time Scale** | Cycles time scale between slow motion and normal/fast values. |
| **Shift+F6** | **Toggle Skybox** | Enables or disables skybox rendering. |
| **Shift+F7** | **Toggle Shadows** | Enables or disables shadow mapping. |
| **Shift+F8** | **Audio Debug** | Toggles audio debug display. |
| **Shift+F9** | **Particle Debug** | Toggles particle debug display. |
| **Shift+F11** | **Debug Camera** | Toggles the free/debug camera. |
| **Shift+F12** | **Cursor Mode** | Cycles cursor modes. |

## Panels

The `HelpPanel` mirrors the current shortcut list. The `ToolsPanel` exposes the same runtime toggles through ImGui controls, including physics debug, entity names, gizmos, light gizmos, skybox, shadows, pause, and time scale.

## Visual Debugging

- **Physics Debug**: Renders Bullet collision shapes through the Bullet debug drawer.
- **Gizmos**: Draws transform, light, grid, and other editor visual aids when their config flags are enabled.
- **Debug Camera**: Uses editor camera modules rather than a separate global debug system.
