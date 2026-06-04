# Navigation Guide

AXIS Engine features a robust Navigation System for AI pathfinding and character movement, based on generated Navigation Meshes (NavMeshes).

---

## 1. Navigation Components

### NavMeshComponent
Stores the spatial data used for pathfinding.
- **Data**: Vertices, Indices, and path-optimized polygons.
- **Usage**: Automatically processed by the `NavigationSystem` for A* calculations.

### PathFollowerComponent
Enables an entity to traverse the NavMesh.
- **Movement**: `MoveSpeed`, `ArrivalDistance`.
- **Rotation**: `RotationSpeed`, `MaxRotationSpeed`, `RotationAcceleration` (for smooth orientation).
- **Alignment**: `RotationOffset` to fix character model facing.

---

## 2. NavMesh Generation
NavMeshes are baked from existing scene geometry.

### Baking Steps
1.  **Tag Geometry**: Set the `Tag` of all floors and ramps to `Walkable`.
2.  **Add NavMesh Entity**: Create an empty entity with a `NavMeshComponent`.
3.  **Generate**: Call `NavigationSystem::GenerateNavMesh()` (usually via editor or script) to bake the data from all `Walkable` meshes.

---

## 3. Navigation System Features
The `NavigationSystem` automatically manages entities with a `PathFollowerComponent`:

- **A* Pathfinding**: Finds the shortest traversable path between world points.
- **Path Smoothing**: Skips unnecessary nodes if a direct line-of-sight exists on the NavMesh.
- **Ground Alignment**: Automatically rotates the character to match the slope of the ground normal.
- **Steering**: Handles smooth acceleration and deceleration during rotation.

---

## 4. Pathfinding Logic
The system supports multiple criteria to suit different gameplay needs:

- **Shortest**: Standard Euclidean distance (Fastest).
- **Smoothest**: Minimizes altitude (slope) changes for more natural movement.
- **StayOnRoad**: Prefers nodes with specific tags (e.g., `road`, `walkable`).
- **StraightLine**: Bypasses the NavMesh and moves directly from start to target.
- **HighGround**: Biases path cost toward higher nodes.
- **Custom**: Evaluates path cost via a user-defined C++ callback.

### PathfindingOptions
Configure behavior via the `PathfindingOptions` struct:
- `preferredTags`: List of tags that reduce movement cost.
- `tagWeightBonus`: Multiplier to prioritize preferred areas.
- `altitudePenaltyWeight`: Scaling for slope-based cost (Smoothest mode).

---

## 5. Scripting API
```cpp
auto& follower = GetComponent<PathFollowerComponent>();

// To move a character:
follower.SetTarget(targetPosition);

// The NavigationSystem handles the rest:
// - Calculating path
// - Following nodes
// - Re-calculating if target moves
```

---

## See Also
- [Physics Guide](physics.md)
- [Graphics Guide](graphics.md)
- [Scriptable API](../scripting/scriptable_api.md)
