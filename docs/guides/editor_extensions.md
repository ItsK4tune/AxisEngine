# Mở rộng AxisEngine Editor

Editor extension được đăng ký qua `IEditorExtensionRegistry`. Một extension không cần sửa
`EditorSystem`: nó cung cấp factory tạo module, panel hoặc cả hai.

Demo chạy được nằm tại:

- `sample/src/editor/sample_editor_extension.h`
- `sample/src/editor/sample_editor_extension.cpp`
- lifecycle đăng ký/hủy đăng ký nằm trong `SampleState::OnEnter` và `SampleState::OnExit`

Build `axis_samples` với `ENABLE_EDITOR=ON`, bật Editor, rồi mở **Tools > Extension Demo**.
Panel demo có thể tạo entity, chọn entity vừa tạo và di chuyển toàn bộ multi-selection. Cả hai
mutation đều tham gia undo/redo chung của editor.

## 1. Chọn module hay panel

- Dùng `IEditorModule` cho logic chạy theo frame, input hoặc world overlay.
- Dùng `IEditorPanel` cho cửa sổ ImGui.
- Chia sẻ state bằng `std::shared_ptr` được capture bởi hai factory. Không dùng biến global
  nếu state chỉ thuộc extension.
- Panel không nên chứa subsystem dài hạn; module không nên sở hữu UI window.

## 2. Đăng ký bằng owner ổn định

Mỗi plugin/game extension phải có owner id ổn định và duy nhất:

```cpp
constexpr char kOwner[] = "my_company.my_game.level_tools";

auto state = std::make_shared<MyExtensionState>();
registry.RegisterModule(
    kOwner, "level.validation",
    [state] { return std::make_unique<LevelValidationModule>(state); });
registry.RegisterPanel(
    kOwner, "level.panel",
    [state] { return std::make_unique<LevelToolsPanel>(state); });
```

`name` là identity nội bộ của extension, không phải title hiển thị. Panel title được trả về từ
`IEditorPanel::GetTitle()`. Không dựa vào index trong vector panel vì thứ tự có thể thay đổi khi
plugin được load/unload.

Registration trả về `false` khi owner/name rỗng, factory không hợp lệ, factory trả về null hoặc
cặp `(owner, name, kind)` đã tồn tại. Nếu đăng ký nhiều phần mà một phần thất bại, rollback toàn
owner:

```cpp
if (!moduleOk || !panelOk) {
    registry.UnregisterOwner(kOwner);
    return false;
}
```

## 3. Lifecycle và cleanup

Khi editor đã initialize, registry gọi `Initialize()` ngay sau khi factory tạo instance.
`UnregisterOwner()` gọi `Shutdown()` trước khi hủy instance. Vì vậy:

- đăng ký sau khi `EditorSystem` đã publish `IEditorExtensionRegistry`;
- hủy đăng ký trước khi DLL/plugin chứa code factory bị unload;
- `Initialize()` và `Shutdown()` phải đối xứng;
- không giữ raw pointer tới module/panel sau `UnregisterOwner()`;
- nên làm hàm register idempotent bằng cách kiểm tra `GetExtensions()`.

Demo gắn lifecycle vào `SampleState`. Plugin động nên gắn cùng thao tác vào entry/exit của plugin.

## 4. Mutation, selection và undo

Panel lấy selection dùng chung từ `EditorSelection`. Mutation trực tiếp lên scene phải bắt đầu
bằng transaction:

```cpp
auto* selection = ServiceLocator::Instance().Resolve<EditorSelection>();
if (selection && !selection->Empty()) {
    EditorSystem::BeginTransaction(scene, "Move selected entities");
    for (entt::entity entity : selection->GetAll()) {
        if (auto* position = scene.TryGetComponent<PositionComponent>(entity)) {
            position->value.x += 1.0f;
            scene.MarkTransformDirty(entity);
        }
    }
}
```

Các nguyên tắc:

- thao tác nhiều entity chỉ tạo một transaction;
- dùng `GetAll()` khi tool hỗ trợ multi-selection, `GetPrimary()` khi thực sự chỉ chỉnh primary;
- gọi dirty hook tương ứng sau khi sửa component trực tiếp;
- không mutate scene chỉ vì panel được vẽ;
- mutation liên tục như slider/drag nên bắt đầu transaction ở activation, không tạo transaction
  mỗi frame.

Extension source-level có thể dùng `ServiceLocator` như demo. API game thông thường vẫn nên dùng
`EngineAccessor`; plugin binary cần giới hạn phụ thuộc vào contract public để giảm chi phí nâng cấp.

## 5. CMake

Target editor phải link `Axis::Editor`; runtime target chỉ link `Axis::Engine`:

```cmake
target_link_libraries(MyGame PRIVATE Axis::Engine)
target_link_libraries(MyGameEditor PRIVATE Axis::Editor)
```

`Axis::Editor` truyền `ENABLE_EDITOR` cho consumer. Bọc file/header tích hợp bằng
`#ifdef ENABLE_EDITOR` nếu cùng source tree được dùng cho cả runtime và editor.

Demo của repository được lấy tự động từ `sample/src/editor/*.cpp` khi
`ENABLE_EDITOR AND BUILD_SAMPLES` bật.

## 6. Checklist extension

- Owner id duy nhất và ổn định.
- Register có rollback; unregister theo owner.
- Module/panel không giữ reference tới `Scene` qua frame.
- Scene mutation có transaction và dirty notification.
- Multi-selection không bị rút xuống primary ngoài ý muốn.
- Không consume input gameplay nếu tool/panel không active.
- Không serialize entity helper: đánh dấu `InfoComponent::isTransient = true`.
- Overlay và picking dùng viewport/G-buffer chính; extension không tạo Scene View riêng.
- Build được cả cấu hình có editor và runtime-only.

Xem thêm [Editor tools, panels và shortcuts](editor.md) và
[Extending Axis Engine](extending_engine.md).
