# PhysicsWorld

**Include:** `<engine/physic/physic_world.h>`

The `PhysicsWorld` class wraps the `btDiscreteDynamicsWorld` from Bullet Physics and manages the physics simulation lifecycle.

## Overview

Manages the physics simulation environment, including:
*   Rigid Body creation and management.
*   Collision Dispatcher & Broadphase configuration.
*   Constraint solver.
*   Debug Drawing.
*   Simulation stepping (Fixed timestep).

## Configuration Modes

The world supports distinct configuration modes to balance performance vs accuracy. These can be set via:
- Scene files: `CONFIG PHYSICS_MODE <MODE>`
- Settings: `physicsMode` in `settings.json`
- Runtime: `PhysicsWorld::SetMode(int mode)`

Additionally, async physics can be toggled via:
- Scene files: `CONFIG PHYSICS_ASYNC TRUE|FALSE`
- Settings: `physicsAsync` in `settings.json`
- Runtime: `PhysicsSystem::SetAsyncPhysics(bool)`

| Mode | ID | Hz | Iterations | Description |
|------|----|----|------------|-------------|
| **FAST** | 0 | 30 | 2 | Extreme optimization. Uses 30Hz simulation, minimal iterations (2), and aggressive sleeping. Suitable for simple scenes or low-end devices. May cause tunneling or jitter. |
| **BALANCED** | 1 | 60 | 10 | Default. Uses 60Hz simulation, 10 iterations. Good balance for general purpose gameplay. |
| **ACCURATE** | 2 | 120 | 40 | High quality. Uses 120Hz simulation, 40 iterations. Uses Global CFM for stability. Expensive but precise. |

### Internal Settings

*   **Broadphase**: Automatically switches strategies based on mode (e.g., `btDbvtBroadphase` for reliability).
*   **Solver**: `btSequentialImpulseConstraintSolver`.
*   **Sleeping**: Thresholds are adjusted per mode to reduce CPU usage.

## Public API

### Lifecycle
*   `PhysicsWorld()`: Initializes Bullet world.
*   `Update(float dt)`: Steps the simulation using fixed timestep logic.
*   `Clear()`: Cleans up all bodies and constraints.

### Management
*   `CreateRigidBody(...)`: Creates and adds a `btRigidBody`.
*   `AddConstraint(...) / RemoveConstraint(...)`: Manage constraints (e.g., Hinge, Point2Point).
*   `SetMode(int mode)`: Switches the physics configuration (Re-initializes the world).

### Debugging
*   `GetDebugDrawer()`: Returns the debug drawer instance for rendering wireframes.
