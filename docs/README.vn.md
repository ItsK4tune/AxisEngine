<p align="center">
  <img src="../include/engine/asset/project/logo.png" alt="AxisEngine logo" width="220">
</p>

# AxisEngine - Tổng quan Tiếng Việt

> [English](README.eng.md) | [Mục lục Tài liệu](vi/INDEX.md) | [Sổ tay Sử dụng](vi/MANUAL.md)

---

## 1. Giới thiệu

AxisEngine là game engine C++20 xây dựng trên nền tảng EnTT Entity-Component-System. Engine cung cấp renderer OpenGL 4.6, vật lý Bullet 3D, backend âm thanh linh hoạt, bộ biên dịch scene nhị phân và bộ công cụ editor ImGui.

Ứng dụng người dùng tự định nghĩa lớp `Application` và `State` ban đầu. Dự án biên dịch ra thư viện tĩnh `Axis::Engine`, thư viện `Axis::Editor` và ứng dụng chạy scenario mẫu `axis_samples`.

---

## 2. Bảng Hỗ trợ Hệ thống

| Tính năng | Triển khai Hỗ trợ | Flag / Thiết lập | Ghi chú |
| :--- | :--- | :--- | :--- |
| Ngôn ngữ | Chuẩn C++20 | `CMAKE_CXX_STANDARD=20` | Yêu cầu CMake 3.20+ |
| Đồ họa | OpenGL 4.6 | `AXIS_GRAPHICS_BACKEND=OpenGL` | Forward/Deferred PBR; ngoại trừ macOS |
| Vật lý | Bullet Physics 3D | `AXIS_PHYSICS_BACKEND=Bullet` | Rigidbodies, colliders, constraints |
| Âm thanh | Null, FMOD, irrKlang | `AXIS_AUDIO_BACKEND=Null/FMOD/IrrKlang` | Null là mặc định không phụ thuộc SDK ngoài |
| Microphone | Thu âm WASAPI | Tự động trên Windows | Đo mức âm lượng & sự kiện giọng nói |
| Nền tảng | Windows, Linux | Preset cấu hình sẵn | Windows MSVC / Linux Ninja |
| Editor | Mở rộng ImGui | `ENABLE_EDITOR=ON` | Bao gồm mục tiêu `Axis::Editor` |
| Scene | `.axs` (YAML subset), `.axsb` | Native Parsers / `axis_compile` | Hỗ trợ 5 loại schema `.axs` |

---

## 3. Hướng dẫn Build

### Biên dịch Engine và Editor
```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

### Chạy Bộ Kiểm thử
```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

---

## 4. Mã nguồn Mẫu Tối giản

```cpp
#include <axis_sdk.h>

class GameplayState final : public State {
public:
    void OnEnter() override { AXIS_LOG_INFO("Gameplay State dang hoat dong"); }
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
    config.title = "AxisEngine Quickstart";
    config.window.width = 1280;
    config.window.height = 720;

    if (!app->Initialize(config)) return 1;
    app->PushState<GameplayState>();
    app->Run();
    return 0;
}
```

---

## 5. Cấu trúc Repository

| Đường dẫn | Mô tả |
| :--- | :--- |
| `include/` | Header SDK công khai (`axis_sdk.h`, `axis_plugin.h`, `axis_advanced.h`) |
| `src/` | Mã nguồn engine và công cụ `Axis::Editor` |
| `sample/` | 33 scenario demo, scene thử nghiệm, texture và asset |
| `compiler/` | Công cụ biên dịch scene nhị phân `axis_compile` |
| `tests/` | Các bộ kiểm thử unit test, integration test |
| `docs/` | Hướng dẫn, tài liệu API, sổ tay sử dụng và mục lục |
| `cmake/` | Cấu hình package, toolchain overlays và phụ thuộc |

---

## 6. Sơ đồ Tài liệu

- Core Engine: [Bắt đầu nhanh](vi/core/getting_started.md) | [Kiến trúc Engine](vi/core/architecture.md) | [Bề mặt API](vi/core/api_surface.md) | [Quản lý Managers](vi/core/managers.md)
- Cấu hình & Định dạng: [Hướng dẫn Build](vi/guides/build_guide.md) | [Cấu hình Configuration](vi/guides/configuration.md) | [Định dạng Scene](vi/guides/scene_format.md) | [Tra cứu Component](vi/guides/components_reference.md)
- Subsystem Runtime: [Đồ họa](vi/guides/graphics.md) | [Vật lý](vi/guides/physics.md) | [Âm thanh](vi/guides/audio.md) | [Thu âm Microphone](vi/guides/audio_capture.md) | [Hệ thống UI](vi/guides/ui.md) | [Điều hướng NavMesh](vi/guides/navigation.md)
- Editor & Scripting: [Hướng dẫn Editor](vi/guides/editor.md) | [Mở rộng Editor](vi/guides/editor_extensions.md) | [Lập trình Scripting API](vi/scripting/scriptable_api.md) | [Quản lý State API](vi/state/state_api.md) | [Hệ thống Debug](vi/guides/debug_system.md)
