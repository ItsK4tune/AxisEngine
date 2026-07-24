# Extending the AxisEngine editor

> [Tiếng Việt](../../vi/guides/editor_extensions.md)

Editor extensions register modules and panels through
`IEditorExtensionRegistry`; they do not need to modify `EditorSystem`.
The working example is in `sample/src/editor/sample_editor_extension.*`.

## Register

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

Owner and internal names must be stable and unique. Panel display text comes
from `IEditorPanel::GetTitle`; never use panel-vector position as identity.

When the editor is initialized, registration immediately creates and
initializes the instance. `UnregisterOwner` calls `Shutdown` before destruction.
Unregister before unloading code/DLLs that contain factories or virtual methods.

## Mutate safely

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

Create one transaction for a multi-entity operation. Start continuous
slider/drag transactions on activation, not every frame. Call the appropriate
dirty hook after direct component mutation. Do not retain a `Scene&` or raw
module/panel pointer across lifetime boundaries.

## Build

Runtime-only targets link `Axis::Engine`. Editor hosts link `Axis::Editor`,
which propagates `ENABLE_EDITOR`:

```cmake
target_link_libraries(MyGame PRIVATE Axis::Engine)
target_link_libraries(MyGameEditor PRIVATE Axis::Editor)
```

Guard shared integration source with `#ifdef ENABLE_EDITOR`.

## Checklist

- Unique stable owner ID; rollback partial registration.
- Symmetric initialize/shutdown; unregister before code unload.
- Transaction and dirty notification for scene mutation.
- Respect multi-selection and input ownership.
- Mark editor helper entities transient.
- Do not serialize extension-only runtime state accidentally.
- Test editor-enabled and runtime-only configurations.
