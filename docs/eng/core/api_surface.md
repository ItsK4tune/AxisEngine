# Public API Surface Guide

> [Tiếng Việt](../../vi/core/api_surface.md) | [Architecture Overview](architecture.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine organizes its public C++ APIs into four tiered header umbrellas (`axis_sdk.h`, `axis_plugin.h`, `axis_advanced.h`, `axis_all.h`). This tiering isolates external applications from internal backend implementation details while maintaining fast build speeds and ABI stability.

---

## 2. How to Use

Select the appropriate header umbrella based on your application role:

1. **Game Applications & Scripting**: `#include <axis_sdk.h>` (Stable high-level API).
2. **Custom Providers & Extensions**: `#include <axis_plugin.h>` (Plugin contracts for audio/physics/renderer).
3. **Low-Level Engine Integrations**: `#include <axis_advanced.h>` (Direct access to Bullet/OpenGL pointers).
4. **Internal Engine Compilation**: `#include <axis_all.h>` (Internal precompiled header only).

---

## 3. Examples

### Tier 1 Example (`<axis_sdk.h>`)
```cpp
#include <axis_sdk.h>

void CreatePlayer(Scene& scene) {
    auto player = scene.CreateEntity("Player");
    player.AddComponent<TransformComponent>(Vector3(0.0f, 1.0f, 0.0f));
    player.AddComponent<MeshRendererComponent>("models/character.obj");
}
```

### Tier 2 Example (`<axis_plugin.h>`)
```cpp
#include <axis_plugin.h>

class CustomAudioProvider final : public IAudioService {
public:
    bool Initialize() override { return true; }
    void Shutdown() override {}
    void PlaySound(const std::string& path, float volume) override {}
    void SetMasterVolume(float volume) override {}
};
AXIS_EXPORT_PLUGIN_PROVIDER(IAudioService, CustomAudioProvider)
```

---

## 4. API & Configuration Reference

### Header Selection & Stability Matrix

| Header File | Targeted Consumer | Stability Level | Include Guidance |
| :--- | :--- | :--- | :--- |
| `<axis_sdk.h>` | Game Developers, Scripters | **High (Stable)** | Primary umbrella for games & user scripts |
| `<axis_plugin.h>` | Plugin Authors, Providers | **Medium** | Use for custom audio/physics/network backends |
| `<axis_advanced.h>` | Low-Level Integrators | **Low (Internal)** | Use when direct OpenGL/Bullet pointers are required |
| `<axis_all.h>` | Internal Build Only | **Internal** | Do not include in external applications |
