# AxisEngine User Manual

> [Tiếng Việt](../vi/MANUAL.md) | [Documentation Index](INDEX.md) | [Root README](../README.eng.md)

---

## 1. Introduction

AxisEngine is a C++20 static engine library with an optional static editor library (`Axis::Editor`). Built around an Entity-Component-System (ECS) architecture, AxisEngine provides developers with modular rendering (OpenGL 4.6 PBR), Bullet 3D physics, multi-backend audio, `.axs` scene compilation, and ImGui editor tools.

---

## 2. How to Use

1. **Configure & Build**: Use CMake presets (`windows-msvc-editor` / `linux-ninja-editor`) and run `cmake --build build --config Release`.
2. **Select SDK Headers**: Include `<axis_sdk.h>` for games/scripts, `<axis_plugin.h>` for provider extensions, or `<axis_advanced.h>` for low-level system access.
3. **Subclass Application**: Subclass `Application`, register user scripts in `RegisterUserScripts()`, push an initial `State`, and call `app->Run()`.
4. **Author Scenes**: Create `.axs` YAML scene files and optionally compile to `.axsb` binary files via `axis_compile`.

---

## 3. Examples

### Complete Application Setup Example
```cpp
#include <axis_sdk.h>

class GameplayState final : public State {
public:
    void OnEnter() override {
        AXIS_LOG_INFO("GameplayState Active");
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

## 4. API & Configuration Reference

### Provider Strategy Matrix Reference

| Capability | Backend Implementation | Key in `axis_config.axs` | CMake Option / Dependency |
| :--- | :--- | :--- | :--- |
| Graphics | OpenGL 4.6 PBR Renderer | `GRAPHICS_API: OPENGL` | Enabled by default; requires OpenGL 4.6 |
| Physics | Bullet Physics 3D | `PHYSICS_ENGINE: BULLET` | Enabled by default; Bullet 3D library |
| Audio | Null Backend (Default) | `AUDIO_ENGINE: NULL` | Default fallback; zero dependencies |
| Audio | FMOD Sound System | `AUDIO_ENGINE: FMOD` | `-DAXIS_AUDIO_BACKEND=FMOD -DFMOD_ROOT_DIR=...` |
| Audio | irrKlang Engine | `AUDIO_ENGINE: IRRKLANG` | `-DAXIS_AUDIO_BACKEND=IrrKlang -DIRRKLANG_ROOT_DIR=...` |
| Microphone | WASAPI Capture | `AUDIO_CAPTURE_ENABLED: 1` | Windows platform native microphone capture |

### CMake Build Target Options Reference

| Option Flag | Default | Values | Description |
| :--- | :--- | :--- | :--- |
| `ENABLE_EDITOR` | `OFF` | `ON` / `OFF` | Compiles ImGui editor library (`Axis::Editor`) |
| `BUILD_SAMPLES` | `ON` | `ON` / `OFF` | Compiles `axis_samples` executable |
| `ENABLE_TESTS` | `OFF` | `ON` / `OFF` | Compiles `axis_test` suite registered with CTest |
| `ENABLE_LTO` | `ON` | `ON` / `OFF` | Enables Link-Time Optimization (IPO) |
| `ENABLE_PCH` | `ON` | `ON` / `OFF` | Enables precompiled header support |
