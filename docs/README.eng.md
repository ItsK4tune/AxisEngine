<p align="center">
  <img src="../include/engine/asset/project/logo.png" alt="AxisEngine logo" width="220">
</p>

# AxisEngine - English Overview

> [Tiếng Việt](README.vn.md) | [Documentation Index](eng/INDEX.md) | [User Manual](eng/MANUAL.md)

---

## 1. Introduction

AxisEngine is a C++20 game engine built around the EnTT Entity-Component-System framework. It provides an OpenGL 4.6 renderer, Bullet 3D physics, flexible audio backends, scene compilation, and ImGui editor tools.

Applications provide their own `Application` subclass and initial `State`. The project builds the `Axis::Engine` static library, the `Axis::Editor` library, and the `axis_samples` scenario runner.

---

## 2. Support Matrix

| Area | Shipped Implementation | Config / Flags | Notes |
| :--- | :--- | :--- | :--- |
| Language | C++20 | `CMAKE_CXX_STANDARD=20` | Requires CMake 3.20+ |
| Graphics | OpenGL 4.6 | `AXIS_GRAPHICS_BACKEND=OpenGL` | Forward/Deferred PBR; non-macOS |
| Physics | Bullet Physics 3D | `AXIS_PHYSICS_BACKEND=Bullet` | Rigidbodies, colliders, constraints |
| Audio | Null, FMOD, irrKlang | `AXIS_AUDIO_BACKEND=Null/FMOD/IrrKlang` | Null is zero-dependency default |
| Microphone | WASAPI Capture | Automatic on Windows | Level meters & voice events |
| Platforms | Windows, Linux | Preset configurations | Windows MSVC / Linux Ninja |
| Editor | ImGui Extension | `ENABLE_EDITOR=ON` | Includes `Axis::Editor` target |
| Scenes | `.axs` (YAML subset), `.axsb` | Native Parsers / `axis_compile` | Supports 5 `.axs` schema types |

---

## 3. Setup & Build

### Configure and Build Engine with Editor
```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

### Run Tests
```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

---

## 4. Minimal Code Example

```cpp
#include <axis_sdk.h>

class GameplayState final : public State {
public:
    void OnEnter() override { AXIS_LOG_INFO("Gameplay State active"); }
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

## 5. Repository Structure

| Path | Purpose |
| :--- | :--- |
| `include/` | Public SDK headers (`axis_sdk.h`, `axis_plugin.h`, `axis_advanced.h`) |
| `src/` | Engine implementation and `Axis::Editor` sources |
| `sample/` | 33 demo scenarios, test scenes, textures, and assets |
| `compiler/` | `axis_compile` binary scene compiler utility |
| `tests/` | Unit, integration, and API test cases |
| `docs/` | Guides, API references, manuals, and index |
| `cmake/` | Package config, toolchain overlays, and dependencies |

---

## 6. Documentation Map

- Core Engine: [Getting Started](eng/core/getting_started.md) | [Architecture](eng/core/architecture.md) | [API Surface](eng/core/api_surface.md) | [Managers](eng/core/managers.md)
- Configuration & Format: [Build Guide](eng/guides/build_guide.md) | [Configuration](eng/guides/configuration.md) | [Scene Format](eng/guides/scene_format.md) | [Components Reference](eng/guides/components_reference.md)
- Subsystems: [Graphics](eng/guides/graphics.md) | [Physics](eng/guides/physics.md) | [Audio](eng/guides/audio.md) | [Audio Capture](eng/guides/audio_capture.md) | [UI System](eng/guides/ui.md) | [Navigation](eng/guides/navigation.md)
- Tools & Scripting: [Editor Guide](eng/guides/editor.md) | [Editor Extensions](eng/guides/editor_extensions.md) | [Scripting API](eng/scripting/scriptable_api.md) | [State API](eng/state/state_api.md) | [Debug System](eng/guides/debug_system.md)
