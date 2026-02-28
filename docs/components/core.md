# Core Components

## InfoComponent
**Struct:** `InfoComponent`

Basic metadata for an entity.
*   `std::string name`: The name of the entity.
*   `std::string tag`: A tag for categorization (e.g., "Player", "Enemy").
*   `uint32_t layer`: The bitmask layer for filtering (1-32).
*   `std::string sceneName`: Internal ref to the source scene file.

## TransformComponent
**Struct:** `TransformComponent`

Defines the position, rotation, and scale of an entity in the world. Also handles hierarchy.

*   `glm::vec3 position`: World/Local position.
*   `glm::quat rotation`: Quaternion rotation.
*   `glm::vec3 scale`: Scale factor.
*   `entt::entity parent`: Parent entity ID.
*   `std::vector<entt::entity> children`: List of children entities.

**AXS Example**:
```yaml
Component: Transform
  Position: 10.0 0.0 5.0
  Rotation: 0.0 90.0 0.0
  Scale: 1.0 1.0 1.0
```

**Helper Methods:**
*   `SetParent(entity child, entity parent, registry, keepWorldTransform)`
*   `GetWorldModelMatrix(registry)`: Returns the calculated global model matrix.

## CameraComponent
**Struct:** `CameraComponent`

Defines the view/projection logic for the scene.
*   `bool isPrimary`: If true, this is the active camera. (AXS: `Primary`)
*   `float fov`: Field of view in degrees. (AXS: `FOV`)
*   `float yaw`, `pitch`: Rotation angles. (AXS: `Yaw`, `Pitch`)
*   `float nearPlane`, `farPlane`: Clipping planes. (AXS: `Near`, `Far`)
*   `glm::mat4 viewMatrix`, `projMatrix`: Internal matrices.

**AXS Example**:
```yaml
Component: Camera
  Primary: 1
  FOV: 60.0
  Near: 0.1
  Far: 1000.0
```
