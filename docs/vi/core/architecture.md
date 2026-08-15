# Hướng dẫn Tổng quan Kiến trúc (Architecture Overview)

> [English](../../eng/core/architecture.md) | [Bề mặt API Công khai](api_surface.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine được thiết kế dựa trên **Kiến trúc Mô-đun Lai ECS (Hybrid Modular ECS Architecture)**. Kết hợp bộ lưu trữ entity sparse-set của EnTT với các provider trừu tượng (`IGraphicsContext`, `IPhysicsWorld`, `IAudioService`), AxisEngine mang lại hiệu năng bộ nhớ đệm CPU cao đồng thời hỗ trợ linh hoạt thay đổi các backend.

---

## 2. Cách dùng

Vòng lặp engine lặp qua 4 phase thực thi trong mỗi frame tick:

1. **Phase 1 (Input & Poll)**: Xử lý thông điệp cửa sổ GLFW và cập nhật trạng thái phím/chuột/gamepad.
2. **Phase 2 (Logic & Script)**: Thực thi `State::OnUpdate()`, `Scriptable::OnUpdate()`, job ticks và xả hàng đợi sự kiện.
3. **Phase 3 (Physics)**: Tiến hành mô phỏng vật lý Bullet 3D (60Hz tick) và đồng bộ transform entity.
4. **Phase 4 (Render & Present)**: Culling không gian, shadow pass, G-Buffer, lighting, UI, ImGui Editor GUI và tráo đổi buffer.

---

## 3. Ví dụ

### Truy vấn Entity & Component trong ECS
```cpp
#include <axis_sdk.h>

void SystemProcessTransforms(Scene& scene, float deltaTime) {
    auto& registry = scene.GetRegistry();
    auto view = registry.view<TransformComponent, RigidBodyComponent>();

    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& rigidbody = view.get<RigidBodyComponent>(entity);

        if (rigidbody.body) {
            transform.SetPosition(rigidbody.body->GetPosition());
        }
    }
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Thành phần Kiến trúc Cốt lõi

| Subsystem / Lớp | Mẫu / Lớp Cơ sở | Trách nhiệm & Vai trò Chính |
| :--- | :--- | :--- |
| `Application` | Singleton Host | Quản lý tiến trình toàn cục, cửa sổ và các provider |
| `ServiceLocator` | Service Registry | Điểm truy cập phân tách toàn cục cho `IAudioService`, `IPhysicsWorld` |
| `StateMachine` | Stack Manager | Quản lý bộ nhớ state dạng stack (`PushState`, `PopState`, `ChangeState`) |
| `Scene` | EnTT Registry Wrapper | Tạo entity, lưu trữ component, hierarchy và serialization |
| `JobSystem` | Thread Pool | Bộ phân phối công việc đa luồng không khóa cho các tác vụ song song |
| `EventManager` | Pub-Sub Dispatcher | Phát sự kiện hàng đợi hoãn lại và đăng ký hàm lắng nghe |
