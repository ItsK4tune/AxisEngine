# ScriptableSystem

**Include:** `<engine/ecs/system.h>`

Manages the lifecycle of C++ scripts attached to entities.

## Responsibilities
*   **Instantiation**: Creates script instances (calls factory) if they don't exist.
*   **Initialization**: Calls `Init()` and `OnCreate()` on new scripts.
*   **Updates**: Calls `OnUpdate(dt)` on all enabled scripts every frame.
*   **Cleanup**: `OnDestroy` is handled during component removal or system shutdown.

## Public API
*   `void Update(Scene &scene, float dt, Application *app)`
