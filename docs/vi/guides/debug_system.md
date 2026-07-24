# Debug và phím điều khiển editor

> [English](../../eng/guides/debug_system.md)

AxisEngine không có class `DebugSystem` độc lập. Diagnostic/toggle được quản lý
bởi editor module, `DebugConfig`, `ToolsPanel` và system đang đọc config.

| Phím | Chức năng |
|---|---|
| `F1` | Bật/tắt tên entity |
| `F2` | Gizmo tổng quát |
| `F3` | Light gizmo |
| `F4` | Skybox |
| `F5` | Shadow |
| `F6` | Post-process |
| `F7` | Bullet collider debug |
| `F8` | Audio source marker |
| `F9` | Particle emitter marker |
| `F10` | Quyền sở hữu cursor của editor |
| `Shift+F10` | Debug/free camera |
| `F11` | Pause/resume engine loop |
| `F12` | Chuyển time scale |

`HelpPanel` hiển thị shortcut hiện hành; `ToolsPanel` cung cấp cùng toggle qua
ImGui. Physics debug vẽ collision shape; gizmo vẽ transform/light/grid; debug
camera thuộc editor camera module.
