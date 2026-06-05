# Physics Guide

AXIS Engine integrates **Bullet Physics** as the physics provider in the current build.

---

## 1. Physics Components

### RigidBody
The core component for physical interaction.
- **Types**: `BOX`, `SPHERE`, `CAPSULE`, `COMPOUND`.
- **Body Types**:
  - `STATIC`: Fixed in place (Mass = 0).
  - `DYNAMIC`: Fully simulated by forces.
  - `KINEMATIC`: Moved manually via code/animations; pushes dynamic objects.
- **Settings**: `Mass`, `Friction`, `Restitution`, `LinearFactor` (Axis locking).

### CharacterController
A specialized capsule-based body for players.
- Handles stepping over obstacles (`StepHeight`).
- Slopes management (`MaxSlope`).
- Managed via `Move(direction)` and `Jump()` methods.

---

## 2. Physics World Configuration
Global simulation quality is configured in the `.axs` file via `PHYSICS_MODE`.

| Mode | Rate | Iterations | Use Case |
| :--- | :---: | :---: | :--- |
| **FAST** | 30Hz | 2 | Mobile/Low-end, simple physics. |
| **BALANCED** | 60Hz | 10 | Standard gameplay (Default). |
| **ACCURATE** | 120Hz | 40 | High-precision / Complex stacks. |

```yaml
Config:
  PHYSICS_MODE: BALANCED
```

---

## 3. Advanced Features

### Collision Matrix
Tag-based filtering to ignore specific interactions.
```cpp
// Scripting Example
IgnoreTagCollision("Player", "Bullet");
IgnoreNameCollision("EntityA", "EntityB");
```

### Raycasting
Allows querying the world for physical hits.
- `Raycast(start, end)`: Returns `RayHit` data.
- `RaycastFromScreen(pos, dist)`: Used for mouse picking.

### Collision Callbacks
Hook into physics events in any `Scriptable` component:
- `OnCollisionEnter(other)`
- `OnCollisionStay(other)`
- `OnCollisionExit(other)`

---

## 4. Systems & Lifecycle
The `PhysicsSystem` orchestrates the simulation during the **Fixed Timestep** loop:

1.  **Sync In**: Mirror ECS transforms to Bullet bodies (Kinematic/Static).
2.  **Simulate**: Bullet steps the world.
3.  **Sync Out**: Mirror Bullet transforms back to ECS `TransformComponent`.
4.  **Events**: Detect manifolds and dispatch callbacks to scripts.

---

## See Also
- [Graphics Guide](graphics.md)
- [Scene Format (.axs)](scene_format.md)
- [Scriptable API](../scripting/scriptable_api.md)
