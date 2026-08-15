# Physics Simulation System Guide (Bullet 3D)

> [Tiếng Việt](../../vi/guides/physics.md) | [Components Reference](components_reference.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine integrates **Bullet Physics 3D** as its primary physical simulation provider. The physics subsystem manages 3D rigid body dynamics, collision detection, raycasting queries, constraints, collision layer filtering, and character controllers.

---

## 2. How to Use

1. **Creating RigidBodies**: Add `RigidBodyComponent` to an entity, set mass (`0.0` for static, `>0` for dynamic), shape (`BOX`, `SPHERE`, `CAPSULE`, `MESH`), and extents.
2. **Performing Raycasts**: Query `ServiceLocator::Get<IPhysicsWorld>()->Raycast(origin, direction, distance, hitResult)`.
3. **Applying Impulses & Forces**: Call `rigidbody.body->ApplyCentralImpulse(forceVector)` on active dynamic bodies.

---

## 3. Examples

### 1. Creating Dynamic RigidBody Example
```cpp
#include <axis_sdk.h>

void SpawnPhysicsBall(Scene& scene, const Vector3& pos) {
    auto ball = scene.CreateEntity("Physics Ball");

    auto& transform = ball.AddComponent<TransformComponent>();
    transform.SetPosition(pos);

    auto& rb = ball.AddComponent<RigidBodyComponent>();
    rb.mass = 2.0f;
    rb.shape = CollisionShapeType::SPHERE;
    rb.radius = 0.5f;
    rb.restitution = 0.8f; // Bouncy
}
```

### 2. Physics Raycast Example
```cpp
#include <axis_sdk.h>

void FireRaycast(const Vector3& from, const Vector3& dir) {
    auto physics = ServiceLocator::Get<IPhysicsWorld>();
    RaycastHit hit;

    if (physics->Raycast(from, dir, 100.0f, hit)) {
        AXIS_LOG_INFO("Hit distance: " + std::to_string(hit.distance));
    }
}
```

---

## 4. API & Configuration Reference

### Physics Settings & Parameters Reference

| Setting / Property | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `PHYSICS_ENGINE` | `Enum` | `BULLET` | Physics strategy provider |
| `GRAVITY` | `Vector3` | `0.0 -9.81 0.0` | World gravity vector |
| `PHYSICS_MODE` | `Enum` | `BALANCED` | Simulation frequency (`FAST` 30Hz, `BALANCED` 60Hz, `ACCURATE` 120Hz) |
| `CCD_ENABLED` | `bool` | `false` | Enables Continuous Collision Detection for fast objects |
| `RigidBodyComponent::mass` | `float` | `1.0` | Mass in kg (`0.0` creates static body) |
| `RigidBodyComponent::shape` | `Enum` | `BOX` | Shape primitive (`BOX`, `SPHERE`, `CAPSULE`, `MESH`) |
| `RigidBodyComponent::friction` | `float` | `0.5` | Surface friction coefficient |
| `RigidBodyComponent::restitution` | `float` | `0.0` | Bounciness elasticity coefficient |
