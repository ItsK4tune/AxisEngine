# Hướng dẫn physics

> [English](../../eng/guides/physics.md)

Build hiện tại dùng Bullet Physics.

## 1. Component

### RigidBody

- Shape: `BOX`, `SPHERE`, `CAPSULE`, `COMPOUND`, cùng shape mở rộng trong component reference.
- `STATIC`: cố định, mass 0.
- `DYNAMIC`: mô phỏng bằng force.
- `KINEMATIC`: code/animation điều khiển, có thể đẩy dynamic body.
- Field chính: `Mass`, `Friction`, `Restitution`, `LinearFactor`.

### CharacterController

Capsule controller cho player, hỗ trợ `StepHeight`, `MaxSlope`, `Move` và `Jump`.

## 2. Cấu hình world

| Mode | Rate | Iteration | Dùng cho |
|---|---:|---:|---|
| `FAST` | 30 Hz | 2 | Mobile/physics đơn giản |
| `BALANCED` | 60 Hz | 10 | Gameplay mặc định |
| `ACCURATE` | 120 Hz | 40 | Stack phức tạp/độ chính xác cao |

```yaml
Config:
  PHYSICS_MODE: BALANCED
```

## 3. Tính năng nâng cao

```cpp
IgnoreTagCollision("Player", "Bullet");
IgnoreNameCollision("EntityA", "EntityB");
```

- `Raycast(start, end)` trả `RayHit`.
- `RaycastFromScreen(position, distance)` dùng cho picking.
- Callback script: `OnCollisionEnter`, `OnCollisionStay`, `OnCollisionExit`;
  trigger có callback tương ứng.

## 4. Lifecycle

Trong fixed timestep:

1. Sync ECS → Bullet cho static/kinematic.
2. Step simulation.
3. Sync Bullet → ECS cho dynamic.
4. Thu manifold/trigger và dispatch callback.

## Xem thêm

- [Graphics](graphics.md)
- [Scene format](scene_format.md)
- [Scriptable API](../scripting/scriptable_api.md)
