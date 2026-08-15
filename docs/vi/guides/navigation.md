# Hướng dẫn Hệ thống Điều hướng & Tìm đường (Recast/Detour)

> [English](../../eng/guides/navigation.md) | [Tra cứu Component Reference](components_reference.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine tích hợp bộ công cụ **Recast & Detour Navigation Mesh** để cung cấp tính năng tự động tạo NavMesh 3D, truy vấn tìm đường, né tránh va chạm agent và xây dựng lại theo ô lợp (tiled rebuild) phục vụ AI tìm đường.

---

## 2. Cách dùng

1. **Tạo NavAgent**: Thêm `NavAgentComponent` vào entity AI và thiết lập `speed`, `acceleration`, `radius`, `height`.
2. **Thiết lập Điểm đến**: Đặt `agent.targetPosition = vectorDiemDen`.
3. **Truy vấn Đường đi Bất đồng bộ**: Sử dụng `ServiceLocator::Get<INavigationService>()->FindPathAsync(start, end, callback)`.

---

## 3. Ví dụ

### 1. Ví dụ Gán Điểm đến NavMesh
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

### 2. Ví dụ Truy vấn Tìm đường Bất đồng bộ
```cpp
#include <axis_sdk.h>

void QueryAsyncPath(const Vector3& start, const Vector3& goal) {
    auto nav = ServiceLocator::Get<INavigationService>();
    if (nav) {
        nav->FindPathAsync(start, goal, [](const std::vector<Vector3>& path) {
            AXIS_LOG_INFO("Duong di da tinh toan xong voi " + std::to_string(path.size()) + " diem waypoint.");
        });
    }
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Tham số `NavAgentComponent` & Tối ưu hóa

| Khóa Tham số | Kiểu dữ liệu | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `speed` | `float` | `3.5` | Tốc độ di chuyển tối đa tính bằng m/s |
| `acceleration` | `float` | `8.0` | Tốc độ gia tốc bẻ lái |
| `stoppingDistance` | `float` | `0.5` | Ngưỡng khoảng cách tới điểm đến để dừng |
| `radius` | `float` | `0.4` | Bán kính khoảng trống vật lý né tránh va chạm |
| `height` | `float` | `1.8` | Chiều cao khoảng trống của agent |
| `targetPosition` | `Vector3` | `0.0 0.0 0.0` | Vị trí mục tiêu điểm đến hiện tại |
| `OPT_NAVIGATION_ASYNC_PATHFINDING` | `bool` | `1` | Bật/tắt tính toán đường đi NavMesh bất đồng bộ |
