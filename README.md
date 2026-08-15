<p align="center">
  <img src="include/engine/asset/project/logo.png" alt="AxisEngine logo" width="220">
</p>

# AxisEngine

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)]()
[![Graphics](https://img.shields.io/badge/Graphics-OpenGL%204.6-orange.svg)]()
[![Physics](https://img.shields.io/badge/Physics-Bullet-red.svg)]()

[English](#english) | [Tiếng Việt](#tiếng-việt)

---

## English

### Introduction
AxisEngine is a C++20 game engine using the Entity-Component-System (ECS) pattern. It provides an OpenGL 4.6 renderer, Bullet 3D physics, flexible audio backends, scene serialization, scripting, navigation, UI, networking, an ImGui editor, and 33 sample scenarios.

#### Features
- Core: EnTT ECS registry, state machine, job system, event dispatcher.
- Graphics: OpenGL 4.6 PBR renderer (Forward/Deferred), shadows, HDR, bloom, particles.
- Physics: Bullet 3D rigidbodies, colliders, character controller, raycasting.
- Audio: Null (default), FMOD, irrKlang backends; WASAPI mic capture on Windows.
- Scenes: `.axs` text format (5 schemas) and compiled `.axsb` binary format.
- Scripting: C++ `Scriptable` behavior hooks and stack-based `StateMachine`.
- Navigation & UI: Recast/Detour pathfinding, canvas UI layout system.
- Tools: ImGui editor (`Axis::Editor`), scene compiler (`axis_compile`).

### Links
- [English README](docs/README.eng.md)
- [Documentation Index](docs/eng/INDEX.md)
- [User Manual](docs/eng/MANUAL.md)
- [Build Guide](docs/eng/guides/build_guide.md)
- [Debug Controls](docs/eng/guides/debug_system.md)

### Quick Start
```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

### Minimal Code Example
```cpp
#include <axis_sdk.h>

class MainState final : public State {
public:
    void OnEnter() override { AXIS_LOG_INFO("MainState started"); }
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
    config.title = "AxisEngine Game";
    config.window.width = 1280;
    config.window.height = 720;

    if (!app->Initialize(config)) return 1;
    app->PushState<MainState>();
    app->Run();
    return 0;
}
```

---

## Tiếng Việt

### Giới thiệu
AxisEngine là game engine C++20 dùng mô hình Entity-Component-System (ECS). Engine cung cấp renderer OpenGL 4.6, vật lý Bullet 3D, backend âm thanh linh hoạt, serialization scene, scripting, điều hướng, UI, mạng, editor ImGui và 33 scenario mẫu.

#### Tính năng
- Cốt lõi: EnTT ECS, state machine, job system đa luồng, event dispatcher.
- Đồ họa: OpenGL 4.6 PBR renderer (Forward/Deferred), bóng đổ, HDR, bloom, hạt.
- Vật lý: Bullet 3D rigidbodies, colliders, character controller, raycasting.
- Âm thanh: Backend Null (mặc định), FMOD, irrKlang; thu âm micro WASAPI trên Windows.
- Scene: Định dạng `.axs` (5 schema) và file nhị phân biên dịch `.axsb`.
- Scripting: Lớp C++ `Scriptable` và quản lý trạng thái `StateMachine`.
- Điều hướng & UI: Recast/Detour pathfinding, hệ thống canvas UI.
- Công cụ: ImGui editor (`Axis::Editor`), scene compiler (`axis_compile`).

### Đường dẫn
- [README Tiếng Việt](docs/README.vn.md)
- [Mục lục Tài liệu](docs/vi/INDEX.md)
- [Sổ tay Sử dụng](docs/vi/MANUAL.md)
- [Hướng dẫn Build](docs/vi/guides/build_guide.md)
- [Công cụ Debug](docs/vi/guides/debug_system.md)

### Build và Chạy Nhanh
```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

### Mã nguồn Mẫu
```cpp
#include <axis_sdk.h>

class MainState final : public State {
public:
    void OnEnter() override { AXIS_LOG_INFO("MainState bat dau"); }
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
    config.title = "Tro Choi AxisEngine";
    config.window.width = 1280;
    config.window.height = 720;

    if (!app->Initialize(config)) return 1;
    app->PushState<MainState>();
    app->Run();
    return 0;
}
```
