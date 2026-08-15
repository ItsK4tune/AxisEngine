# Sổ tay Hướng dẫn Sử dụng AxisEngine (User Manual)

> [English](../eng/MANUAL.md) | [Mục lục Tài liệu](INDEX.md) | [Trang chủ README](../README.vn.md)

---

## 1. Giới thiệu

AxisEngine là thư viện engine C++20 tĩnh kèm thư viện editor tùy chọn (`Axis::Editor`). Được xây dựng trên kiến trúc Entity-Component-System (ECS), AxisEngine cung cấp cho lập trình viên quy trình dựng hình mô-đun (OpenGL 4.6 PBR), vật lý Bullet 3D, âm thanh đa backend, biên dịch scene `.axs` và công cụ editor ImGui.

---

## 2. Cách dùng

1. **Cấu hình & Build**: Sử dụng CMake presets (`windows-msvc-editor` / `linux-ninja-editor`) và chạy `cmake --build build --config Release`.
2. **Chọn Header SDK**: Include `<axis_sdk.h>` cho game/script, `<axis_plugin.h>` cho mở rộng provider, hoặc `<axis_advanced.h>` cho truy cập cấp thấp.
3. **Kế thừa Application**: Kế thừa `Application`, đăng ký script người dùng trong `RegisterUserScripts()`, đẩy `State` ban đầu và gọi `app->Run()`.
4. **Soạn thảo Scene**: Tạo file scene văn bản `.axs` YAML và biên dịch sang `.axsb` nhị phân qua `axis_compile`.

---

## 3. Ví dụ

### Ví dụ Cấu hình Ứng dụng Hoàn chỉnh
```cpp
#include <axis_sdk.h>

class GameplayState final : public State {
public:
    void OnEnter() override {
        AXIS_LOG_INFO("GameplayState Dang Hoat Dong");
        SceneManager::Get().LoadScene("scenes/main.axs");
    }
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
    config.title = "AxisEngine User Manual Demo";
    config.window.width = 1280;
    config.window.height = 720;

    if (!app->Initialize(config)) return 1;
    app->PushState<GameplayState>();
    app->Run();
    return 0;
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Chiến lược Provider Strategy

| Khả năng | Triển khai Backend | Khóa trong `axis_config.axs` | Tùy chọn CMake / Phụ thuộc |
| :--- | :--- | :--- | :--- |
| Đồ họa | OpenGL 4.6 PBR Renderer | `GRAPHICS_API: OPENGL` | Bật mặc định; yêu cầu OpenGL 4.6 |
| Vật lý | Bullet Physics 3D | `PHYSICS_ENGINE: BULLET` | Bật mặc định; thư viện Bullet 3D |
| Âm thanh | Null Backend (Mặc định) | `AUDIO_ENGINE: NULL` | Dự phòng mặc định; không phụ thuộc thư viện |
| Âm thanh | Hệ thống Âm thanh FMOD | `AUDIO_ENGINE: FMOD` | `-DAXIS_AUDIO_BACKEND=FMOD -DFMOD_ROOT_DIR=...` |
| Âm thanh | Engine irrKlang | `AUDIO_ENGINE: IRRKLANG` | `-DAXIS_AUDIO_BACKEND=IrrKlang -DIRRKLANG_ROOT_DIR=...` |
| Microphone | Thu âm WASAPI | `AUDIO_CAPTURE_ENABLED: 1` | Thu âm micro native trên Windows |

### Bảng Tra cứu Tùy chọn Target CMake

| Flag Tùy chọn | Mặc định | Giá trị | Mô tả |
| :--- | :--- | :--- | :--- |
| `ENABLE_EDITOR` | `OFF` | `ON` / `OFF` | Biên dịch thư viện editor ImGui (`Axis::Editor`) |
| `BUILD_SAMPLES` | `ON` | `ON` / `OFF` | Biên dịch file thực thi `axis_samples` |
| `ENABLE_TESTS` | `OFF` | `ON` / `OFF` | Biên dịch bộ kiểm thử `axis_test` đăng ký với CTest |
| `ENABLE_LTO` | `ON` | `ON` / `OFF` | Bật Tối ưu hóa lúc liên kết (IPO) |
| `ENABLE_PCH` | `ON` | `ON` / `OFF` | Bật Precompiled Header |
