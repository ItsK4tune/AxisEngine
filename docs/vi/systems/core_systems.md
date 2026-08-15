# Hướng dẫn Hệ thống Systems Cốt lõi & Tạo Custom System

> [English](../../eng/systems/core_systems.md) | [Tổng quan Kiến trúc](../core/architecture.md) | [Bề mặt API Công khai](../core/api_surface.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

Logic bao quát nhiều entity hoặc yêu cầu phase thực thi khung hình cụ thể (PreUpdate, Update, PostUpdate, Render) được triển khai dưới dạng **ECS Systems**. Các system tùy chỉnh kế thừa từ `ISystem` và đăng ký vào `SystemRegistry` của engine.

---

## 2. Cách dùng

1. **Kế thừa `ISystem`**: Tạo một lớp kế thừa `ISystem` và ghi đè `void OnUpdate(Scene& scene, float deltaTime)`.
2. **Chỉ định Phase System**: Ghi đè `SystemPhase GetPhase() const` để khai báo thời điểm thực thi (`PreUpdate`, `Update`, `PostUpdate`, `Render`).
3. **Đăng ký System**: Gọi `app.GetSystemRegistry().RegisterSystem<MySystem>()`.

---

## 3. Ví dụ

### Ví dụ Weather System Tùy chỉnh
```cpp
#include <axis_sdk.h>

class WeatherSystem final : public ISystem {
public:
    void OnUpdate(Scene& scene, float dt) override {
        auto& registry = scene.GetRegistry();
        auto view = registry.view<DirectionalLightComponent>();

        for (auto entity : view) {
            auto& light = view.get<DirectionalLightComponent>(entity);
            light.intensity = 2.5f;
        }
    }

    SystemPhase GetPhase() const override {
        return SystemPhase::Update;
    }
};

void RegisterSystemExample(Application& app) {
    app.GetSystemRegistry().RegisterSystem<WeatherSystem>();
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Các Phase Thực thi System & API Reference

| Tên Phase | Thứ tự Ưu tiên | Trách nhiệm Thực thi Chính |
| :--- | :--- | :--- |
| `SystemPhase::PreUpdate` | Đầu Khung hình | Thông điệp cửa sổ, nhận input phần cứng, nạp dữ liệu mạng |
| `SystemPhase::Update` | Giữa Khung hình | Callbacks script, AI behavior, animation, job ticks |
| `SystemPhase::PostUpdate` | Cuối Khung hình | Bước mô phỏng vật lý Bullet, đồng bộ transform phân cấp |
| `SystemPhase::Render` | Dựng hình Khung hình | Frustum/Octree culling, G-Buffer, shadow pass, UI pass, ImGui GUI pass |
