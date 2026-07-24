# Mở rộng AxisEngine Editor

> [English](../../eng/guides/editor_extensions.md)

Extension đăng ký module/panel qua `IEditorExtensionRegistry`, không cần sửa
`EditorSystem`. Ví dụ chạy được nằm tại
`sample/src/editor/sample_editor_extension.*`.

## Đăng ký

```cpp
constexpr char kOwner[] = "my_company.my_game.level_tools";
auto state = std::make_shared<MyExtensionState>();

const bool moduleOk = registry.RegisterModule(
    kOwner, "level.validation",
    [state] { return std::make_unique<LevelValidationModule>(state); });

const bool panelOk = registry.RegisterPanel(
    kOwner, "level.panel",
    [state] { return std::make_unique<LevelToolsPanel>(state); });

if (!moduleOk || !panelOk)
    registry.UnregisterOwner(kOwner);
```

Owner và internal name phải ổn định, duy nhất. Title lấy từ
`IEditorPanel::GetTitle`; không dùng index trong vector làm identity.

Khi editor đã initialize, registration tạo và initialize instance ngay.
`UnregisterOwner` gọi `Shutdown` trước khi hủy. Phải unregister trước khi unload
DLL/code chứa factory hoặc virtual method.

## Mutation an toàn

Mở một transaction cho toàn bộ thao tác nhiều entity, gọi dirty hook sau khi sửa
component trực tiếp. Với slider/drag liên tục, bắt đầu transaction lúc activate,
không tạo mỗi frame. Không giữ `Scene&` hoặc raw module/panel pointer qua
lifecycle boundary.

```cpp
auto* selection = ServiceLocator::Instance().Resolve<EditorSelection>();
if (selection && !selection->Empty())
{
    EditorSystem::BeginTransaction(scene, "Move selected entities");
    for (entt::entity entity : selection->GetAll())
    {
        if (auto* position = scene.TryGetComponent<PositionComponent>(entity))
        {
            position->value.x += 1.0f;
            scene.MarkTransformDirty(entity);
        }
    }
}
```

## Build

```cmake
target_link_libraries(MyGame PRIVATE Axis::Engine)
target_link_libraries(MyGameEditor PRIVATE Axis::Editor)
```

`Axis::Editor` truyền `ENABLE_EDITOR`. Source dùng chung nên guard bằng
`#ifdef ENABLE_EDITOR`.

## Checklist

- Owner ID ổn định; rollback nếu đăng ký dở.
- Initialize/shutdown đối xứng; unregister trước code unload.
- Có transaction và dirty notification khi mutate scene.
- Tôn trọng multi-selection và input ownership.
- Helper entity phải transient.
- Test cả editor-enabled và runtime-only.
