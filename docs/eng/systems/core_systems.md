# Core Systems & Custom System Creation Guide

> [Tiếng Việt](../../vi/systems/core_systems.md) | [Architecture Overview](../core/architecture.md) | [Public API Surface](../core/api_surface.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

Logic spanning many entities or requiring specific frame phase execution (PreUpdate, Update, PostUpdate, Render) is implemented as **ECS Systems**. Custom systems inherit from `ISystem` and register into the engine's `SystemRegistry`.

---

## 2. How to Use

1. **Subclass `ISystem`**: Create a class inheriting `ISystem` and override `void OnUpdate(Scene& scene, float deltaTime)`.
2. **Specify System Phase**: Override `SystemPhase GetPhase() const` to declare execution timing (`PreUpdate`, `Update`, `PostUpdate`, `Render`).
3. **Register System**: Call `app.GetSystemRegistry().RegisterSystem<MySystem>()`.

---

## 3. Examples

### Custom Weather System Example
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

## 4. API & Configuration Reference

### System Execution Phases & API Reference

| Phase Name | Priority Order | Primary Execution Responsibilities |
| :--- | :--- | :--- |
| `SystemPhase::PreUpdate` | Early Frame | Window messages, hardware input polling, network ingestion |
| `SystemPhase::Update` | Frame Core | Script callbacks, AI behavior, animation, job ticks |
| `SystemPhase::PostUpdate` | Late Frame | Bullet physics simulation step, transform hierarchy sync |
| `SystemPhase::Render` | Frame Presentation | Frustum/Octree culling, G-Buffer, shadow pass, UI pass, ImGui GUI pass |
