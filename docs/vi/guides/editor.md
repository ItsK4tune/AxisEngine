# Hướng dẫn AxisEngine Editor

> [English](../../eng/guides/editor.md)

Cập nhật: 2026-07-23. Phạm vi: build có `ENABLE_EDITOR=ON`.

## Kiến trúc

Editor được bootstrap qua `Axis::Editor`, dùng chung selection, command history,
viewport state và input router. Module giữ logic tool/overlay theo frame; panel
giữ cửa sổ ImGui. Extension đăng ký factory qua
`IEditorExtensionRegistry` bằng owner ID ổn định.

Game render thẳng ra platform backbuffer. Dockspace trung tâm trong suốt và UI
editor phủ lên trên; không có G-buffer editor thứ hai bắt buộc.

## Selection và transform

- Click hoặc box-drag trong viewport để chọn qua ID buffer.
- Giữ `Ctrl` để cộng/toggle selection.
- `Shift+click` trong Hierarchy để chọn range.
- Entity chọn gần nhất là primary và được Inspector chỉnh.
- Entity transient/editor-only vẫn hiện nhưng không được serialize bình thường.

Gizmo hỗ trợ move, rotate, scale, world/local, pivot/center và snapping. Một lần
drag tạo một undo transaction.

- `Alt` + arrow/PageUp/PageDown: move.
- `Ctrl+Alt` + arrow/PageUp/PageDown: rotate.
- `Shift+Alt` + arrow/PageUp/PageDown: scale.

## Play/Edit/Stop và history

- Play chụp snapshot đầy đủ của edit scene.
- Pause dừng runtime progression.
- Stop khôi phục snapshot và reset history.
- `Ctrl+S` lưu scene.
- `Ctrl+R` reload; nếu dirty sẽ hỏi trước khi discard.
- Undo: `Ctrl+Z`; redo: `Ctrl+Shift+Z`.

Luôn dùng version control. File/prefab operation ghi trực tiếp vào project.
File Hierarchy canonicalize path dưới project root, create không ghi đè,
duplicate sinh tên duy nhất và rename từ chối conflict. Các lệnh save/apply
được chọn rõ vẫn thay output của chính chúng.

## Panel

Editor gồm Hierarchy, Project/Assets, Inspector, Tools, Settings, Profiler,
Console, State, Network, Help, Animation Graph, VFX Graph, Input Actions,
Navigation, Frame Debugger, Lighting, Prefabs, File Hierarchy và Resource
Browser.

`Project / Assets` quản lý browse/import/dependency/reimport. `Prefabs` dùng
fragment `.axs`. `Lighting` queue probe capture và bake lightmap PPM đơn giản
theo UV cho static mesh. `Navigation` chỉnh navmesh và cost rule.

## Shortcut

- `Ctrl+1..0`: Hierarchy, Project/Assets, Inspector, Tools, Settings, Profiler,
  Console, State, Network, Help.
- `Ctrl+Shift+1`: Animation Graph; `+2`: VFX; `+3`: Input Actions;
  `+4`: Navigation; `+5`: Frame Debugger; `+6`: Lighting; `+7`: Prefabs.
- `F1..F9`: các overlay/debug toggle.
- `F10`: editor cursor; `Shift+F10`: Debug Camera.
- `F11`: pause/resume; `F12`: đổi time scale.

Shortcut yêu cầu đúng modifier. Khi editor/ImGui giữ keyboard, gameplay không
nhận event tương ứng.

## Debug Camera

- RMB + WASD; `Q/E` đi xuống/lên.
- MMB pan; `Alt+LMB` orbit; scroll dolly.
- `F` focus primary selection.
- `Shift`/`Ctrl` tăng/giảm speed.

Game camera được khôi phục khi kết thúc. Debug Camera là transient và không save.

## Quy tắc extension

- Logic theo frame/world đặt trong module; cửa sổ đặt trong panel.
- Dùng owner/name ổn định, không dùng vector index.
- Mở editor transaction trước ECS mutation.
- Đánh dấu helper entity bằng `InfoComponent::isTransient`.
- Overlay/picking dùng main viewport/G-buffer context.
- Asset loader đăng ký qua loader registry.

Xem [mở rộng editor](editor_extensions.md).
