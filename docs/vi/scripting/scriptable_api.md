# Tham chiếu Scriptable API

> [English](../../eng/scripting/scriptable_api.md)

`Scriptable` là base class cho gameplay script C++. Script được gắn vào entity
bằng `ScriptComponent` và đăng ký tường minh theo application.

## Lifecycle

```text
Init(entity, scene, app)
  → OnCreate()
  → OnEnable()
  → OnUpdate(dt), input callback, physics callback
  → OnDisable()
  → OnDestroy()
```

- `Init` do engine gọi để gắn context.
- `OnCreate`: khởi tạo/cached handle.
- `OnUpdate`: logic mỗi frame, `dt` chịu time scale.
- `OnDestroy`: unsubscribe event, dừng resource thuộc script.
- `SetEnabled` kích hoạt `OnEnable`/`OnDisable`.

Không cache reference component qua thao tác có thể làm registry thay đổi; cache
entity ID hoặc truy xuất lại khi cần.

## Tạo và đăng ký script

```cpp
#include <script/scriptable.h>

class PlayerController final : public Scriptable {
public:
    void OnCreate() override {
        m_HasBody = HasComponent<RigidBodyComponent>();
    }

    void OnUpdate(float dt) override {
        if (GetActionDown("Jump"))
            Jump();
    }

    void OnDestroy() override {}

private:
    bool m_HasBody = false;
};
```

```cpp
class GameApplication final : public Application {
public:
    void RegisterUserScripts() override {
        RegisterScript<PlayerController>("PlayerController");
    }
};
```

```yaml
axis_scene:
  Entities:
    Player:
      Component: Script
        Class: PlayerController
```

Manual binding chỉ dùng khi thật sự cần:

```cpp
auto& component = registry.emplace<ScriptComponent>(entity);
component.Bind<PlayerController>();
component.instance = component.InstantiateScript();
component.instance->Init(entity, scenePtr, appPtr);
component.instance->OnCreate();
```

## Component

```cpp
template<class T> T& GetComponent();
template<class T> bool HasComponent();
template<class T> T* GetScript(entt::entity target);
```

`GetComponent<T>()` yêu cầu component tồn tại. Kiểm tra `HasComponent<T>()` nếu
schema không đảm bảo. `GetScript<T>()` trả `nullptr` khi entity/script không phù hợp.

## Input

Gameplay nên dùng action:

```cpp
if (GetAction("MoveForward")) { /* held */ }
if (GetActionDown("Jump"))    { /* pressed */ }
if (GetActionUp("Fire"))      { /* released */ }
float steering = GetAxis("Steer");
```

Raw input nâng cao:

- Keyboard: `GetKey`, `GetKeyDown`, `GetKeyUp`.
- Mouse: button held/down/up, position và delta.
- Callback script nhận key/mouse event khi entity active và script enabled.
- Editor-consumed key không đi vào gameplay action.

## Physics callback

```cpp
void OnCollisionEnter(entt::entity other) override;
void OnCollisionStay(entt::entity other) override;
void OnCollisionExit(entt::entity other) override;
void OnTriggerEnter(entt::entity other) override;
void OnTriggerStay(entt::entity other) override;
void OnTriggerExit(entt::entity other) override;
```

Callback chạy trong flow physics/script; không hủy hàng loạt entity khi đang
iterate nếu API có queue/deferred operation phù hợp.

## Truy cập engine

`Scriptable` kế thừa `EngineAccessor`:

```cpp
auto& resources = Get<ResourceManager>();
auto* audio = Resolve<AudioService>();
auto& physics = GetSystem<PhysicsSystem>();

LoadScene("assets/scenes/overlay.axs");
QueueLoadScene("assets/scenes/next.axs");
QueuePopScene();
```

Ưu tiên interface công khai thay concrete manager. Dùng `Resolve<T>()` khi
service là optional; `Get<T>()`/`GetSystem<T>()` khi contract yêu cầu tồn tại.

## EntityBuilder

`EntityBuilder` tạo entity và publish transform ban đầu ngay lập tức, nên code
chạy trước update tiếp theo của `TransformSystem` vẫn thấy world state đúng.

```cpp
Entity player = EntityBuilder(scene, resources, "Gameplay")
    .WithName("Player")
    .WithTag("player")
    .WithTransform({0.0f, 1.0f, 0.0f})
    .WithPBRRenderable("playerModel", "pbrShader", {0.0f, 1.0f, 0.0f})
    .WithRigidShape(ShapeType::Capsule, {1.0f}, 0.5f, 1.8f)
    .WithRigidBody(70.0f)
    .WithScript("PlayerController")
    .Build();
```

Builder có nhóm method cho resource, identity, transform, render/material,
terrain, physics, navigation, UI, hierarchy, audio, script, animation,
fragment, network, light, camera, particle, video, post-process, probe,
reflection và decal. Ưu tiên `With*` chuyên biệt để companion component và
initial state được tạo nhất quán.

## Pattern thường dùng

### Player controller

- Đọc action trong `OnUpdate`.
- Áp velocity/force qua physics component.
- Camera hoặc animation nhận state đã chuẩn hóa, không tự poll phím lần hai.

### Enemy AI

- Bind `PathFollowerComponent`.
- Đặt target, để `NavigationSystem` xử lý path và steering.
- Rate-limit perception/path request thay vì tính mỗi frame cho mọi enemy.

### Object tương tác

- Dùng trigger callback, tag/layer và action interact.
- Giữ one-shot state để callback stay không phát hành động lặp.

### UI button

- Runtime callback nằm trong `UIInteractiveComponent`, không serialize.
- Unbind callback khi script bị hủy nếu callback không thuộc component lifetime.

## Quy tắc thực hành

Nên:

- Đăng ký script tường minh.
- Dùng action binding, `dt`, queue scene operation và RAII subscription.
- Kiểm tra component/service optional.
- Giữ logic authoritative ở server khi dùng network.

Không nên:

- Gọi lifecycle thủ công ngoài manual-instantiation flow.
- Giữ raw pointer tới resource/provider đã có thể bị thay.
- Sửa scene registry trong callback đang iterate nếu không deferred.
- Tin network message hoặc asset/path từ nguồn không tin cậy.

## Xem thêm

- [State API](../state/state_api.md)
- [Component](../guides/components_reference.md)
- [Physics](../guides/physics.md)
- [Thiết bị](../guides/device_management.md)
