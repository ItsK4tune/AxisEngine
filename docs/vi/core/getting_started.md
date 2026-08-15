# Hướng dẫn Bắt đầu Nhanh (Getting Started Guide)

> [English](../../eng/core/getting_started.md) | [Tổng quan Kiến trúc](architecture.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine là game & multimedia engine C++20 phát triển theo mô hình Entity-Component-System (ECS). Bài hướng dẫn này dẫn dắt bạn qua các bước cài đặt phụ thuộc, biên dịch binary của engine và tạo ứng dụng tối giản đầu tiên.

---

## 2. Cách dùng

Thực hiện theo các bước sau để thiết lập và chạy một dự án AxisEngine:

1. **Chuẩn bị Môi trường**: Cài đặt trình biên dịch C++20 (MSVC 2022 / GCC 11+), CMake 3.20+ và driver GPU hỗ trợ OpenGL 4.6.
2. **Clone & Cấu hình**: Clone repository và cấu hình dự án bằng CMake presets.
3. **Biên dịch Target**: Build thư viện tĩnh và file thực thi ứng dụng mẫu (`axis_samples`).
4. **Tạo Ứng dụng**: Kế thừa `Application`, ghi đè `RegisterUserScripts()`, định nghĩa một `State` và gọi `Run()`.

---

## 3. Ví dụ

### Ví dụ Ứng dụng C++ Tối giản
```cpp
#include <axis_sdk.h>

class MainGameState final : public State {
public:
    void OnEnter() override { AXIS_LOG_INFO("MainGameState Bat Dau"); }
    void OnUpdate(float dt) override {}
    void OnRender() override {}
    void OnExit() override {}
};

class GameApp final : public Application {
public:
    void RegisterUserScripts() override {}
};

int main() {
    auto app = std::make_shared<GameApp>();

    AppConfig config;
    config.title = "AxisEngine Bat Dau Nhanh";
    config.window.width = 1280;
    config.window.height = 720;

    if (!app->Initialize(config)) return 1;
    app->PushState<MainGameState>();
    app->Run();
    return 0;
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Param Cấu hình `AppConfig`

| Tên Tham số | Kiểu dữ liệu | Mặc định | Mô tả |
| :--- | :--- | :--- | :--- |
| `title` | `std::string` | `"AxisEngine"` | Chuỗi tiêu đề hiển thị trên cửa sổ |
| `window.width` | `uint32_t` | `1280` | Chiều rộng cửa sổ tính bằng pixel |
| `window.height` | `uint32_t` | `720` | Chiều cao cửa sổ tính bằng pixel |
| `window.mode` | `WindowMode` | `WINDOWED` | Chế độ hiển thị (`WINDOWED`, `FULLSCREEN`, `BORDERLESS`) |
| `window.vsync` | `bool` | `true` | Bật đồng bộ hóa dọc VSync |
| `graphicsApi` | `GraphicsAPI` | `OPENGL` | Provider dựng hình đồ họa |
| `physicsEngine` | `PhysicsEngine`| `BULLET` | Provider mô phỏng vật lý |
| `audioEngine` | `AudioEngine` | `NULL` | Provider phát âm thanh |
