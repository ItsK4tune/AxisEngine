# PhysicsSystem

**Include:** `<engine/ecs/system.h>`

Bridges the ECS world with the Bullet Physics world.

## Responsibilities
*   **Collision Callbacks**: Triggers `OnCollisionEnter` etc. on scripts.
*   **Debug Drawing**: Renders wireframes if explicitly called.
*   **Transform Sync**:
    *   Normally, Bullet controls the motion. The system reads `btRigidBody` transform and updates `TransformComponent`.
    *   However, if `isAttachedToParent` is true, or if body is Kinematic, logic might flow the other way.

## public API
*   `void Update(Scene &scene, PhysicsWorld &world, float dt)`
*   `void RenderDebug(Scene &scene, PhysicsWorld &world, Shader &shader, ...)`
