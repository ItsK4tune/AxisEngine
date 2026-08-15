# Hướng dẫn Hệ thống Mô phỏng Vật lý (Bullet 3D)

> [English](../../eng/guides/physics.md) | [Tra cứu Component Reference](components_reference.md) | [Tra cứu Cấu hình Configuration](configuration.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine tích hợp **Bullet Physics 3D** làm provider mô phỏng vật lý chính. Subsystem vật lý quản lý động lực học vật thể rắn 3D, phát hiện va chạm, truy vấn raycasting, khớp nối constraint, lọc lớp va chạm và character controller.

---

## 2. Cách dùng

1. **Tạo RigidBody**: Thêm `RigidBodyComponent` vào entity, thiết lập khối lượng (`0.0` cho vật thể tĩnh static, `>0` cho vật thể động dynamic), hình dạng (`BOX`, `SPHERE`, `CAPSULE`, `MESH`) và kích thước.
2. **Thực thi Raycast**: Truy vấn `ServiceLocator::Get<IPhysicsWorld>()->Raycast(origin, direction, distance, hitResult)`.
3. **Tác dụng Lực & Xung lực**: Gọi `rigidbody.body->ApplyCentralImpulse(forceVector)` trên các vật thể động.

---

## 3. Ví dụ

### 1. Ví dụ Tạo RigidBody Động
```cpp
#include <axis_sdk.h>

void SpawnPhysicsBall(Scene& scene, const Vector3& pos) {
    auto ball = scene.CreateEntity("Physics Ball");

    auto& transform = ball.AddComponent<TransformComponent>();
    transform.SetPosition(pos);

    auto& rb = ball.AddComponent<RigidBodyComponent>();
    rb.mass = 2.0f;
    rb.shape = CollisionShapeType::SPHERE;
    rb.radius = 0.5f;
    rb.restitution = 0.8f; // Nẩy
}
```

### 2. Ví dụ Bắn tia Raycast
```cpp
#include <axis_sdk.h>

void FireRaycast(const Vector3& from, const Vector3& dir) {
    auto physics = ServiceLocator::Get<IPhysicsWorld>();
    RaycastHit hit;

    if (physics->Raycast(from, dir, 100.0f, hit)) {
        AXIS_LOG_INFO("Khoang cach trung: " + std::to_string(hit.distance));
    }
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Cấu hình & Tham số Vật lý

| Tham số / Thuộc tính | Kiểu dữ liệu | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `PHYSICS_ENGINE` | `Enum` | `BULLET` | Provider mô phỏng vật lý |
| `GRAVITY` | `Vector3` | `0.0 -9.81 0.0` | Vector trọng lực thế giới |
| `PHYSICS_MODE` | `Enum` | `BALANCED` | Tần số mô phỏng (`FAST` 30Hz, `BALANCED` 60Hz, `ACCURATE` 120Hz) |
| `CCD_ENABLED` | `bool` | `false` | Bật phát hiện va chạm liên tục cho vật thể di chuyển nhanh |
| `RigidBodyComponent::mass` | `float` | `1.0` | Khối lượng tính bằng kg (`0.0` tạo vật thể tĩnh static) |
| `RigidBodyComponent::shape` | `Enum` | `BOX` | Hình dạng va chạm cơ bản (`BOX`, `SPHERE`, `CAPSULE`, `MESH`) |
| `RigidBodyComponent::friction` | `float` | `0.5` | Hệ số ma sát bề mặt |
| `RigidBodyComponent::restitution` | `float` | `0.0` | Hệ số đàn hồi nẩy |
