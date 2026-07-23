# AxisEngine Editor: tools, panels và shortcuts

Cập nhật: 2026-07-23
Phạm vi: runtime có `ENABLE_EDITOR`

## 1. Kiến trúc hiện tại

Editor được tổ chức quanh bốn service dùng chung:

- `EditorSelection`: một primary entity và tập selection có thứ tự.
- `EditorCommandHistory`: transaction snapshot và command-based undo/redo.
- `EditorViewportState`: vùng tương tác của game viewport bên dưới lớp editor overlay.
- `IKeyboardInputRouter`: quyết định phím nào thuộc editor trước khi event đi vào gameplay.

Module dùng cho tool/overlay không sở hữu cửa sổ. Panel chỉ phụ trách UI. Extension đăng ký
module/panel qua `IEditorExtensionRegistry` với owner ổn định để cleanup theo plugin.

## 2. Selection và hierarchy

- Click trong Viewport chọn entity bằng ID buffer.
- Giữ `Ctrl` khi click/box-select để cộng vào selection.
- Kéo chuột trái trên Viewport để box-select.
- Click trong Hierarchy chọn một entity.
- `Ctrl+click` trong Hierarchy toggle từng entity.
- `Shift+click` chọn range giữa anchor và entity hiện tại.
- Primary entity là entity được chọn gần nhất; Inspector chỉnh primary.
- Tất cả entity trong selection được outline. Membership được truyền qua SSBO, không bị giới hạn
  bởi số uniform cố định.
- Entity transient/editor-only vẫn hiện trong Hierarchy bằng màu cam và hậu tố `[Transient]`,
  nhưng không được ghi trong scene save thông thường.

## 3. Game viewport và editor overlay

Editor không đặt viewport vào trong ImGui:

- Game render được present trực tiếp ra platform backbuffer như runtime bình thường.
- Dockspace trung tâm trong suốt; tool và panel ImGui chỉ phủ lên game viewport.
- Picking, box selection, outline và transform gizmo đọc trực tiếp framebuffer/G-buffer chính.
- Không có render pass, texture copy, post-process pipeline hay G-buffer thứ hai dành riêng cho editor.
- Resize/kéo dock hoặc panel không resize render target, tránh GPU stall và cảm giác giật.
- Editor chuẩn bị các dock slot quanh vùng game trung tâm nhưng không tự mở Hierarchy, Project,
  Inspector hay extension panel. Mở panel cần dùng từ menu **View** hoặc shortcut.
- Dùng **View > Reset Layout** nếu file layout ImGui cũ làm panel bị co nhỏ hoặc nằm sai dock.

Điều khiển camera trên game viewport:

- `F10`: bật/tắt quyền điều khiển cursor của editor.
- `Shift+F10`: chuyển giữa camera game và Debug Camera transient.
- Khi Debug Camera active, đưa chuột ra ngoài panel rồi giữ RMB + WASD; `Q/E` đi xuống/lên.
- MMB kéo để pan.
- `Alt+LMB` để orbit.
- Scroll để dolly; scroll khi giữ RMB đổi fly speed.
- `F` focus primary selection.
- `Shift/Ctrl` tăng/giảm tốc độ.
- Khi tắt editor, active game camera được khôi phục và Debug Camera không bị serialize.

## 4. Transform gizmo và selection policy

Transform gizmo trên game viewport hỗ trợ:

- Move, Rotate, Scale.
- World/Local orientation.
- Pivot/Center placement.
- Axis/plane/uniform handles và snapping.
- Một lần drag tạo một undo transaction.

Keyboard nudge:

- `Alt + Arrow/PgUp/PgDn`: move.
- `Ctrl+Alt + Arrow/PgUp/PgDn`: rotate.
- `Shift+Alt + Arrow/PgUp/PgDn`: scale.

## 5. Play/Edit/Stop và scene safety

- Play chụp snapshot đầy đủ của edit scene.
- Pause chỉ dừng runtime.
- Stop khôi phục snapshot, xóa runtime mutation và reset command history.
- `Ctrl+S` lưu scene và cập nhật dirty baseline.
- `Ctrl+R` reload ngay nếu sạch; nếu dirty sẽ mở modal xác nhận trước khi discard.
- Snapshot editor có transient entities; file scene bình thường loại transient.

## 6. Undo/redo

- Undo: `Ctrl+Z`.
- Redo: `Ctrl+Shift+Z`.
- Không dùng `Ctrl+Y`.
- Gizmo, hierarchy operations, nudge, prefab operations và các mutation bắt đầu từ
  Inspector/Animation/VFX UI đều đi qua transaction bridge.
- Transaction không thay đổi ECS bị loại bỏ, nên click thuần UI không làm bẩn history.

## 7. Project, assets và prefab

Panel `Project / Assets` có ba tab:

- Project: browse/preview, tạo folder, tạo scene/prefab/material/text asset, rename, duplicate và
  delete có xác nhận. Non-empty directory không bị xóa đệ quy.
- Loaded Resources: inspect model, texture, shader, audio, animation, video, skybox, font,
  fragment và source assets.
- Import / Dependencies: import qua loader registry, xem metadata dependency và reimport thủ công.
  Shader/texture vẫn hỗ trợ watcher hot reload.

Panel `Prefabs` dùng fragment asset làm định dạng prefab:

- Create khóa source entity ngay lúc mở hộp thoại, nhập đường dẫn `.axs`, cảnh báo overwrite và
  tùy chọn thay selection bằng linked instance.
- `Create instance from existing` duyệt các prefab dưới `assets/prefabs`.
- Apply: ghi child hierarchy hiện tại về asset, bỏ namespace instance và reload.
- Revert: xóa overrides và yêu cầu `FragmentSystem` instantiate lại asset.
- Inspector hiển thị trạng thái asset/instance, visual component overrides, reimport, reload và clear.
- Advanced override YAML có validate trước khi áp dụng.

Panel `Input Actions` dùng tên action ổn định, chọn device/control bằng tên thân thiện, hiển thị
trạng thái live và binding dạng chip; click chip để gỡ. Numeric code vẫn có trong Advanced mode.
Toolbar asset hỗ trợ New, Open, Save và Save As; Open chỉ liệt kê file `.axs` có schema
`axis_input`, còn Save As yêu cầu xác nhận trước khi ghi đè.

## 8. Lighting và navigation

Lighting panel:

- Chọn lighting mode.
- Theo dõi directional/point/spot lights và probe counts.
- Queue incremental reflection-probe recapture.
- User nhập output image path; token `{entity}` tạo một PPM riêng cho từng static mesh.
- Bake static mesh lightmap raster theo UV hiện có, gồm ambient và direct
  directional/point lighting. Lightmap được gán vào material, serialize và sample bằng shader.
- Mesh cần static model và UV không overlap để có kết quả ổn định.

Navigation panel bake NavMesh theo scene geometry và có custom rule graph. Mỗi rule chứa nhóm
điều kiện `ALL` hoặc `ANY`, nhiều condition, `NOT`, tùy chọn `Stop on match`, và kết quả Reward,
Penalty hoặc Block. Condition hỗ trợ tag, height, uphill/downhill và slope.
Toàn bộ graph nằm trực tiếp trong `NavMeshComponent` của provider và được serialize cùng entity.
Geometry có thể khai báo nhiều navigation tag bằng dấu `+`, ví dụ `walkable+road+flat`.
Panel có bảng chọn `PathFollower` để gán NavMesh provider cùng Custom criterion; không tạo thêm
navigation-profile asset.

## 9. Frame Debugger

Frame Debugger hiển thị:

- CPU/GPU pass timing và frame share.
- Draw calls, triangles, state changes và uniform updates.
- Post-process graph theo priority/owner.
- Draw-call stream: pass, entity, shader, VAO/elements và resource.
- `Execute first N draws` dừng render sau draw thứ N; đặt `0` để chạy toàn bộ frame.
- Capture/Freeze giữ nguyên stats, effects và draw list để inspect.
- Số liệu live là moving average 3 giây để giảm rung.
- Export tạo CSV mới có timestamp trong `log/<folder>`, gồm summary, pass timings và draw-call stream.

## 10. Panel shortcuts

Bank 1:

- `Ctrl+1`: Scene Hierarchy
- `Ctrl+2`: Project / Assets
- `Ctrl+3`: Inspector
- `Ctrl+4`: Tools
- `Ctrl+5`: Settings
- `Ctrl+6`: Profiler
- `Ctrl+7`: Console
- `Ctrl+8`: State
- `Ctrl+9`: Network
- `Ctrl+0`: Help

Bank 2:

- `Ctrl+Shift+1`: Animation Graph
- `Ctrl+Shift+2`: VFX Graph
- `Ctrl+Shift+3`: Input Actions
- `Ctrl+Shift+4`: Navigation
- `Ctrl+Shift+5`: Frame Debugger
- `Ctrl+Shift+6`: Lighting
- `Ctrl+Shift+7`: Prefabs
- `Ctrl+Shift+8`: chưa gán
- `Ctrl+Shift+9`: chưa gán
- `Ctrl+Shift+0`: chưa gán

Các shortcut yêu cầu exact modifier. Khi editor sở hữu cursor hoặc ImGui nhập text, gameplay
không nhận editor keyboard events; editor camera vẫn dùng raw state nội bộ.

## 11. Function keys

- `F1`: entity names
- `F2`: gizmos
- `F3`: light gizmos
- `F4`: skybox
- `F5`: shadows
- `F6`: post-process
- `F7`: physics debug
- `F8`: audio debug
- `F9`: particle debug
- `F10`: editor cursor
- `Shift+F10`: persistent debug-camera control
- `F11`: pause/resume
- `F12`: cycle time scale

## 12. Quy tắc mở rộng

- Tool logic mới nên là module; cửa sổ mới nên là panel.
- Không polling shortcut theo index vector panel. Dùng owner/name identity và shortcut slot ổn định.
- ECS mutation từ editor phải mở transaction trước mutation.
- Editor-only entity phải đặt `InfoComponent::isTransient`.
- View-specific rendering phải dùng `PushRenderViewContext` và G-buffer/view target riêng, không đổi
  active game camera lâu dài.
- Asset loader mới đăng ký qua loader registry để tự xuất hiện trong Import.
