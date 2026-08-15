# Navigation & Pathfinding Guide (Recast/Detour)

> [Tiếng Việt](../../vi/guides/navigation.md) | [Components Reference](components_reference.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine integrates **Recast & Detour Navigation Mesh** to provide automatic 3D NavMesh generation, pathfinding queries, agent collision avoidance, and tiled dynamic rebuilding for AI pathfinding.

---

## 2. How to Use

1. **Creating NavAgents**: Add `NavAgentComponent` to AI entity and set `speed`, `acceleration`, `radius`, `height`.
2. **Setting Target Destination**: Set `agent.targetPosition = destinationVector`.
3. **Querying Async Paths**: Use `ServiceLocator::Get<INavigationService>()->FindPathAsync(start, end, callback)`.

---

## 3. Examples

### 1. Setting NavMesh Destination Example
```cpp
#include <axis_sdk.h>

void MoveAgentToTarget(Entity agentEntity, const Vector3& targetPos) {
    if (agentEntity.HasComponent<NavAgentComponent>()) {
        auto& agent = agentEntity.GetComponent<NavAgentComponent>();
        agent.targetPosition = targetPos;
        agent.speed = 4.5f;
    }
}
```

### 2. Async Pathfinding Query Example
```cpp
#include <axis_sdk.h>

void QueryAsyncPath(const Vector3& start, const Vector3& goal) {
    auto nav = ServiceLocator::Get<INavigationService>();
    if (nav) {
        nav->FindPathAsync(start, goal, [](const std::vector<Vector3>& path) {
            AXIS_LOG_INFO("Path calculated with " + std::to_string(path.size()) + " waypoints.");
        });
    }
}
```

---

## 4. API & Configuration Reference

### `NavAgentComponent` & Optimization Parameters Reference

| Parameter Key | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `speed` | `float` | `3.5` | Max movement speed along path in m/s |
| `acceleration` | `float` | `8.0` | Steering acceleration speed |
| `stoppingDistance` | `float` | `0.5` | Arrival threshold distance |
| `radius` | `float` | `0.4` | Agent physical clearance radius |
| `height` | `float` | `1.8` | Agent clearance height |
| `targetPosition` | `Vector3` | `0.0 0.0 0.0` | Current target destination |
| `OPT_NAVIGATION_ASYNC_PATHFINDING` | `bool` | `1` | Asynchronous NavMesh path calculation toggle |
