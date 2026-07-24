# Tham chiếu State API

> [English](../../eng/state/state_api.md)

`State` biểu diễn mode/screen như Menu, Gameplay, Pause hoặc Settings. State
quản lý scene và policy system; logic từng entity nên nằm trong `Scriptable`.

## Lifecycle

| Method | Thời điểm | Mục đích |
|---|---|---|
| `OnEnter()` | Khi state active | Load scene/resource, bật system, đặt cursor |
| `OnFixedUpdate(float)` | Fixed timestep | Logic cần nhịp cố định |
| `OnUpdate(float)` | Mỗi frame | Điều phối state/input |
| `OnRender()` | Sau update | Render tùy chỉnh hiếm dùng |
| `OnExit()` | Khi rời state | Queue cleanup, lưu state |

Nếu `OnEnter` fail, state machine khôi phục state trước đó.

## Tạo state

```cpp
class GameplayState final : public State {
public:
    void OnEnter() override {
        LoadScene("assets/scenes/gameplay.axs");
        EnablePhysics(true);
        EnableRender(true);
        EnableAudio(true);
        EnableLogic(true);
        SetCursorMode(CursorMode::LockedHidden);
    }

    void OnUpdate(float dt) override {
        if (GetActionDown("Pause")) {
            // Application/state graph thực hiện PushState<PauseState>().
        }
    }

    void OnExit() override {
        QueuePopScene();
    }
};
```

Application khởi động state:

```cpp
app.PushState<MenuState>();
app.Run();
```

Đăng ký metadata để State Machine panel hiển thị graph:

```cpp
void RegisterUserStates() override {
    RegisterState<MenuState>("Menu");
    RegisterState<GameplayState>("Gameplay");
    RegisterState<PauseState>("Pause");

    RegisterStateTransition<MenuState, GameplayState>(
        "Start Game", StateTransitionKind::Change);
    RegisterStateTransition<GameplayState, PauseState>(
        "Pause", StateTransitionKind::Push);
}
```

Runtime `PushState`, `PopState`, `ChangeState` cũng được quan sát tự động.

## Scene API

```cpp
LoadScene(path);          // thêm .axs/.axsb
UnloadScene(path);
ChangeScene(path);        // dọn và nạp scene mới
QueueLoadScene(path);     // cuối frame
QueuePopScene();
IsSceneLoaded(path);
```

Ưu tiên queue operation khi đang update để tránh invalid registry iterator.
Scene operation queue chạy FIFO.

## Điều khiển system

```cpp
EnablePhysics(bool);
EnableRender(bool);
EnableAudio(bool);
EnableScript(bool);
EnableAnimation(bool);
EnableVideo(bool);
EnableUIInteract(bool);
EnableUIRender(bool);
EnableParticle(bool);
EnableSkybox(bool);
EnableLogic(bool);
```

`EnableLogic` điều khiển nhóm gameplay system. Pause overlay thường tắt physics
và logic nhưng giữ render/UI; menu có thể tắt physics và giữ audio/UI.

## Input và service

```cpp
bool GetAction(name);
bool GetActionDown(name);
bool GetActionUp(name);
float GetAxis(name);

auto& resources = Get<ResourceManager>();
auto* audio = Resolve<AudioService>();
auto& system = GetSystem<PhysicsSystem>();
```

Action binding độc lập thiết bị và nên được dùng thay key code.

## Pattern

### Menu

Load UI scene, bật UI/audio, tắt physics; cursor `Normal`.

### Gameplay

Load world, bật system cần thiết, cursor `LockedHidden`; pause bằng push overlay.

### Pause overlay

Giữ scene gameplay, tắt physics/logic, vẫn render world và UI; pop để resume và
khôi phục chính xác policy/cursor trước đó.

### Loading

Hiển thị UI, queue async resource/scene operation, chỉ change state khi future
và publication hoàn tất.

### Cutscene

Giữ render/audio/animation/video, tắt physics hoặc gameplay script chọn lọc.

## Quy tắc thực hành

- State điều phối mode; Scriptable điều khiển entity.
- Ghép enable/disable đối xứng giữa `OnEnter` và `OnExit`.
- Dùng queue cho scene transition trong update.
- Không giữ component reference qua change/unload scene.
- Kiểm tra kết quả load và cung cấp trạng thái lỗi cho user.

## Xem thêm

- [Core systems](../systems/core_systems.md)
- [Scriptable API](../scripting/scriptable_api.md)
- [Scene format](../guides/scene_format.md)
- [Thiết bị](../guides/device_management.md)
