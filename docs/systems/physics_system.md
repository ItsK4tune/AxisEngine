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

### Physics Accuracy
Physics accuracy and performance can be tuned via `PhysicsWorld::SetMode`. See [PhysicsWorld](../managers/physics_world.md) for details.

### Async Physics
The physics system supports asynchronous simulation to improve performance:
- **Async Mode (Default)**: Physics calculations run in a separate thread, allowing rendering to proceed in parallel. This is controlled via:
  - `settings.json`: `"physicsAsync": true`
  - Scene file: `CONFIG PHYSICS_ASYNC TRUE`
  - Runtime: `PhysicsSystem::SetAsyncPhysics(bool)`
- **Benefits**: Better frame rates on multi-core systems, as physics doesn't block the render thread.
- **Tradeoffs**: Results are from the previous frame (1-frame lag). For most gameplay, this is imperceptible.
- **Disable When**: Debugging physics issues, running on single-core systems, or needing deterministic single-threaded execution.

## Architecture

The physics system has been refactored into specialized components:
- **PhysicsTransformSync**: Handles bidirectional transform synchronization between ECS and Bullet Physics.
- **PhysicsCollisionDispatcher**: Manages collision event detection and callback dispatch to scripts.
- **PhysicsSystem**: Orchestrates the overall physics pipeline (sync → simulate → sync → events).
