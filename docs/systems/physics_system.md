# PhysicsSystem

**Include:** `<engine/ecs/system.h>`

Bridges the ECS world with the Bullet Physics world.

## Responsibilities
*   **Collision Callbacks**: Triggers `OnCollisionEnter` etc. on scripts.
*   **Debug Drawing**: Renders wireframes if explicitly called.
*   **Transform Sync**:
    *   Normally, Bullet controls the motion. The system reads `btRigidBody` transform and updates `TransformComponent`.
    *   However, if `isAttachedToParent` is true, or if body is Kinematic, logic might flow the other way.

## Optimization
*   **CachedQuery**: Uses `CachedQuery<RigidBodyComponent, TransformComponent>` to iterate linear memory for active physics objects, avoiding full view iteration overhead.
*   **World Matrix Cache**: Caches parent world matrices per-frame to optimize recursive transform calculations for deep hierarchies.

## Public API
*   `void Update(Scene &scene, PhysicsWorld &world, float dt)`
*   `void RenderDebug(Scene &scene, PhysicsWorld &world, Shader &shader, ...)`

## Configuration
Physics accuracy and performance can be tuned via `PhysicsWorld::SetMode`. See [PhysicsWorld](../managers/physics_world.md) for details.
