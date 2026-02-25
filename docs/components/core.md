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
*   `entt::entity parent`: Parent entity ID (or `entt::null`).
*   `std::vector<entt::entity> children`: List of children entities.

**Helper Methods:**
*   `SetParent(entity child, entity parent, registry, keepWorldTransform)`
*   `GetWorldModelMatrix(registry)`: Returns the calculated global model matrix.
