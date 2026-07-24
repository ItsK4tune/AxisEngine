# Navigation Guide

> [Tiếng Việt](../../vi/guides/navigation.md)

AXIS Engine provides navigation for AI pathfinding and character movement through generated NavMeshes and grid-based A* providers.

---

## 1. Navigation Components

### NavMeshComponent
Stores the spatial data used for NavMesh pathfinding.
- **Data**: Vertices, Indices, and path-optimized polygons.
- **Usage**: Automatically processed by the `NavigationSystem` for A* calculations.

### PathFollowerComponent
Enables an entity to traverse a navigation provider.
- **Movement**: `MoveSpeed`, `ArrivalDistance`.
- **Rotation**: `RotationSpeed`, `MaxRotationSpeed`, `RotationAcceleration` (for smooth orientation).
- **Alignment**: `RotationOffset` to fix character model facing.
- **Provider Binding**: `navigationProviderEntity` can bind the follower to a specific `NavMeshComponent` or `NavigationGridComponent`; otherwise the system falls back to the first valid provider in the scene.

---

## 2. NavMesh Generation
NavMeshes are baked from existing scene geometry.

### Baking Steps
1.  **Tag Geometry**: Set the `Tag` of all floors and ramps to `Walkable`.
2.  **Add NavMesh Entity**: Create an empty entity with a `NavMeshComponent`.
3.  **Generate**: Add a `NavMeshComponent` and set `needsRebuild = true`; `NavigationSystem` rebuilds it during update through `NavMeshGenerator`.

---

## 3. Navigation System Features
The `NavigationSystem` automatically manages entities with a `PathFollowerComponent`:

- **A* Pathfinding**: Finds traversable paths between world points on either NavMesh or grid providers.
- **Path Smoothing**: Skips unnecessary nodes if a direct line-of-sight exists on the NavMesh.
- **Ground Alignment**: Automatically rotates the character to match the slope of the ground normal.
- **Steering**: Handles smooth rotation, local separation, and obstacle avoidance.

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
- `provider`: `Auto`, `NavMesh`, or `Grid`.
- `customCostFunc`: NavMesh-specific custom edge cost callback.
- `customGridCostFunc`: Grid-specific custom cell cost callback.

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
