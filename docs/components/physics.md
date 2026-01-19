# Physics Components

## RigidBodyComponent
**Struct:** `RigidBodyComponent`

Represents a physical object in the Bullet Physics world.

*   `btRigidBody* body`: Pointer to the internal Bullet body.
*   `bool isAttachedToParent`: If true, position is synced to parent transform.

**Methods:**
*   `SetLinearVelocity(vec3)`
*   `SetAngularVelocity(vec3)`
*   `SetFriction(float)`
*   `SetRestitution(float)` (Bounciness)
*   `SetLinearFactor(vec3)`: Lock movement axes (e.g., 0,0,0 to lock).
*   `SetAngularFactor(vec3)`: Lock rotation axes.

**Note:** Changes to `TransformComponent` do NOT automatically sync to `RigidBodyComponent` every frame unless manually handled (Teleporting). Usually, the Physics system moves the Transform.
