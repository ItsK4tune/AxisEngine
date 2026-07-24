# Hướng dẫn navigation

> [English](../../eng/guides/navigation.md)

AxisEngine hỗ trợ AI pathfinding qua NavMesh và grid A*.

## 1. Component

### NavMeshComponent

Lưu vertex, index, polygon/node và metadata dùng cho pathfinding.

### PathFollowerComponent

- Di chuyển: `MoveSpeed`, `ArrivalDistance`.
- Xoay: `RotationSpeed`, `MaxRotationSpeed`, `RotationAcceleration`.
- `RotationOffset` sửa hướng mặc định của model.
- Có thể bind `navigationProviderEntity` tới `NavMeshComponent` hoặc
  `NavigationGridComponent`; nếu không, system chọn provider hợp lệ đầu tiên.

## 2. Bake NavMesh

1. Gắn tag `Walkable` cho floor/ramp.
2. Tạo entity chứa `NavMeshComponent`.
3. Đặt `needsRebuild = true`.
4. `NavigationSystem` gọi `NavMeshGenerator` trong update.

Geometry động gọi `INavigationService::MarkNavMeshDirty(region)` để queue tile
giao nhau theo budget mỗi frame.

## 3. Tính năng system

- A* trên NavMesh hoặc grid.
- Smoothing bằng line-of-sight.
- Căn theo normal mặt đất.
- Smooth rotation, local separation và obstacle avoidance.
- Neighbor index hỏng bị bỏ qua tại API tiêu thụ.

## 4. Tiêu chí pathfinding

- `Shortest`: khoảng cách Euclid.
- `Smoothest`: phạt thay đổi độ cao.
- `StayOnRoad`: ưu tiên tag.
- `StraightLine`: bỏ qua provider.
- `HighGround`: ưu tiên node cao.
- `Custom`: callback cost C++.

`PathfindingOptions` có `preferredTags`, `tagWeightBonus`,
`altitudePenaltyWeight`, `provider`, `customCostFunc`,
`customGridCostFunc`.

## 5. Scripting

```cpp
auto& follower = GetComponent<PathFollowerComponent>();
follower.SetTarget(targetPosition);
```

System tự tính path, đi qua waypoint và tính lại khi target đổi.

## Xem thêm

- [Physics](physics.md)
- [Graphics](graphics.md)
- [Scriptable API](../scripting/scriptable_api.md)
