# Architecture Overview Guide

> [Tiếng Việt](../../vi/core/architecture.md) | [Public API Surface](api_surface.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine is built on a **Hybrid Modular ECS Architecture**. Combining EnTT's cache-coherent sparse-set entity registry with decoupled strategy providers (`IGraphicsContext`, `IPhysicsWorld`, `IAudioService`), AxisEngine delivers high CPU cache performance while supporting interchangeable backends.

---

## 2. How to Use

The engine loop runs through four execution phases during every frame tick:

1. **Phase 1 (Input & Poll)**: Poll GLFW window messages and update mouse/keyboard/gamepad states.
2. **Phase 2 (Logic & Script)**: Execute `State::OnUpdate()`, `Scriptable::OnUpdate()`, job ticks, and event queue flushing.
3. **Phase 3 (Physics)**: Advance Bullet 3D physics simulation (60Hz tick) and sync entity transforms.
4. **Phase 4 (Render & Present)**: Spatial culling, shadow pass, G-Buffer, lighting, UI, ImGui Editor GUI, and buffer swap.

---

## 3. Examples

### Querying Entities & Components in ECS
```cpp
#include <axis_sdk.h>

void SystemProcessTransforms(Scene& scene, float deltaTime) {
    auto& registry = scene.GetRegistry();
    auto view = registry.view<TransformComponent, RigidBodyComponent>();

    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& rigidbody = view.get<RigidBodyComponent>(entity);

        if (rigidbody.body) {
            transform.SetPosition(rigidbody.body->GetPosition());
        }
    }
}
```

---

## 4. API & Configuration Reference

### Core Architecture Components Reference

| Subsystem / Class | Pattern / Base | Primary Responsibilities |
| :--- | :--- | :--- |
| `Application` | Singleton Host | Global process lifecycle, windowing, and provider owner |
| `ServiceLocator` | Service Registry | Decoupled global access point for `IAudioService`, `IPhysicsWorld` |
| `StateMachine` | Stack Manager | Manages active state stack (`PushState`, `PopState`, `ChangeState`) |
| `Scene` | EnTT Registry Wrapper | Entity creation, component storage, hierarchy, and serialization |
| `JobSystem` | Thread Pool | Lock-free worker thread dispatcher for parallel tasks |
| `EventManager` | Pub-Sub Dispatcher | Deferred queue event publishing and handler subscription |
