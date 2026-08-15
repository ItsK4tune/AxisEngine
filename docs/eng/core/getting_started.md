# Getting Started Guide

> [Tiếng Việt](../../vi/core/getting_started.md) | [Architecture Overview](architecture.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine is a C++20 game and multimedia engine built around an Entity-Component-System (ECS) architecture. This guide walks you through setting up dependencies, building the engine binaries, and creating your first minimal application.

---

## 2. How to Use

Follow these steps to set up and run an AxisEngine project:

1. **Setup Prerequisites**: Install C++20 compiler (MSVC 2022 / GCC 11+), CMake 3.20+, and OpenGL 4.6 drivers.
2. **Clone & Configure**: Clone the repository and configure with CMake presets.
3. **Build Target**: Compile the static library and sample application executable (`axis_samples`).
4. **Create Application**: Subclass `Application`, override `RegisterUserScripts()`, define a `State`, and call `Run()`.

---

## 3. Examples

### Minimal C++ Application Example
```cpp
#include <axis_sdk.h>

class MainGameState final : public State {
public:
    void OnEnter() override { AXIS_LOG_INFO("MainGameState Started"); }
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
    config.title = "AxisEngine Getting Started";
    config.window.width = 1280;
    config.window.height = 720;

    if (!app->Initialize(config)) return 1;
    app->PushState<MainGameState>();
    app->Run();
    return 0;
}
```

---

## 4. API & Configuration Reference

### `AppConfig` Parameters Reference

| Parameter Name | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `title` | `std::string` | `"AxisEngine"` | Window title string |
| `window.width` | `uint32_t` | `1280` | Window width in pixels |
| `window.height` | `uint32_t` | `720` | Window height in pixels |
| `window.mode` | `WindowMode` | `WINDOWED` | Presentation mode (`WINDOWED`, `FULLSCREEN`, `BORDERLESS`) |
| `window.vsync` | `bool` | `true` | Enables vertical synchronization |
| `graphicsApi` | `GraphicsAPI` | `OPENGL` | Rendering strategy provider |
| `physicsEngine` | `PhysicsEngine`| `BULLET` | Physics strategy provider |
| `audioEngine` | `AudioEngine` | `NULL` | Audio playback strategy provider |
