# Physics Components

## RigidBodyComponent
**Struct:** `RigidBodyComponent`

Represents a physical object in the Bullet Physics world.

*   **Type**: `BOX`, `CAPSULE`, `COMPOUND`.
*   **BodyType**: `STATIC`, `DYNAMIC`, `KINEMATIC`.
*   **Properties**:
    *   `mass` (float): Object mass (0 for static).
    *   `restitution` (float): Bounciness (0.0 to 1.0).
    *   `friction` (float): Surface friction.
    *   `linearFactor`, `angularFactor` (vec3): Axis locking (e.g., `0 1 0` locks to Y-axis).
    *   `offset` (vec3): Center of mass offset from transform position.
    *   `isAttachedToParent` (bool): Sync with parent transform.
    *   `isParentMatter`, `isChildrenMatter` (bool): Compound physics interaction.

**AXS Example:**
```yaml
Component: RigidBody
  Type: BOX
  Mass: 10.0
  Size: 1.0 1.0 1.0
  BodyType: DYNAMIC
  AngularFactor: 0 1 0
```

**C++ Public Methods:**
- `SetLinearVelocity(glm::vec3)` / `GetLinearVelocity()`
- `SetAngularVelocity(glm::vec3)` / `GetAngularVelocity()`
- `SetFriction(float)`
- `SetRestitution(float)`
- `SetLinearFactor(glm::vec3)`
- `SetAngularFactor(glm::vec3)`
- `ApplyForce(glm::vec3, glm::vec3 offset)`
- `ApplyImpulse(glm::vec3, glm::vec3 offset)`
