# Application & Engine Configuration Reference

> [Tiếng Việt](../../vi/guides/configuration.md) | [Scene Format Guide](scene_format.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine features a centralized data-driven configuration system. Application defaults, graphics rendering flags, physics precision, audio backends, and optimization parameters are defined in `axis_config.axs` or mutated dynamically at runtime.

---

## 2. How to Use

1. **Static Configuration (`.axs`)**: Define an `axis_config` block inside your project configuration `.axs` file.
2. **Runtime Configuration API**: Access `Application::Get().GetConfig()` to modify parameters dynamically.
3. **Notify Systems**: Publish `ConfigChangedEvent` via `EventManager` to apply graphics, physics, or optimization changes live.

---

## 3. Examples

### 1. `.axs` Configuration File Example
```yaml
axis_config:
    LOG_LEVEL: VERBOSE
    JOB_THREADS: -1
    TIME_SCALE: 1.0
    STRICT_ASSET_LOADING: 0

    WINDOW_WIDTH: 1920
    WINDOW_HEIGHT: 1080
    WINDOW_MODE: BORDERLESS_FULLSCREEN
    VSYNC: 1
    FPS: 120

    GRAPHICS_API: OPENGL
    ANTIALIASING: TAA
    HDR_ENABLED: 1
    TONEMAPPING: ACES
    BLOOM_ENABLED: 1

    SHADOWS: 1
    SHADOW_RESOLUTION: 2048
    SHADOW_SOFTNESS: 1

    PHYSICS_ENGINE: BULLET
    GRAVITY: 0.0 -9.81 0.0
    PHYSICS_MODE: BALANCED

    AUDIO_ENGINE: NULL
    VOLUME: 100
```

### 2. Runtime Dynamic Tuning Example
```cpp
#include <axis_sdk.h>

void ApplyRuntimeGraphicsQuality(bool highQuality) {
    auto& config = Application::Get().GetConfig();
    config.graphics.shadowResolution = highQuality ? 4096 : 1024;
    config.graphics.antiAliasing = highQuality ? AntialiasingMode::TAA : AntialiasingMode::FXAA;

    EventManager::Get().Publish(ConfigChangedEvent::Graphics{});
}
```

---

## 4. API & Configuration Reference

### Engine & Global Settings Reference

| Setting Key | Allowed Values / Range | Default | Description |
| :--- | :--- | :--- | :--- |
| `LOG_LEVEL` | `NONE`, `MINIMAL`, `VERBOSE`, `DEBUG` | `VERBOSE` | Console log threshold |
| `JOB_THREADS` | `-1` (Auto), `1` to `64` | `-1` | Worker thread count |
| `TIME_SCALE` | `0.0` to `10.0` | `1.0` | Global simulation speed multiplier |
| `STRICT_ASSET_LOADING` | `0` or `1` | `0` | Disables fallback textures on missing assets |

### Graphics & Display Settings Reference

| Setting Key | Allowed Values / Range | Default | Description |
| :--- | :--- | :--- | :--- |
| `WINDOW_MODE` | `WINDOWED`, `FULLSCREEN`, `BORDERLESS_FULLSCREEN` | `BORDERLESS_FULLSCREEN` | Presentation mode |
| `VSYNC` | `0` or `1` | `1` | Vertical sync toggle |
| `FPS` | `0` (Unlimited), `30`, `60`, `120`, `144` | `120` | Target frame rate limit |
| `ANTIALIASING` | `NONE`, `FXAA`, `TAA` | `TAA` | Anti-aliasing algorithm |
| `TONEMAPPING` | `NONE`, `REINHARD`, `ACES` | `ACES` | Tonemapping curve |
| `BLOOM_ENABLED` | `0` or `1` | `1` | Bloom post-processing toggle |
| `SPATIAL_CULLING` | `AUTO`, `LINEAR`, `OCTREE` | `AUTO` | Spatial culling backend |

### Shadow & Physics Settings Reference

| Setting Key | Allowed Values / Range | Default | Description |
| :--- | :--- | :--- | :--- |
| `SHADOWS` | `0` or `1` | `1` | Master shadow toggle |
| `SHADOW_RESOLUTION` | `512`, `1024`, `2048`, `4096` | `2048` | Depth map resolution |
| `GRAVITY` | 3D Vector `X Y Z` | `0 -9.81 0` | World gravity vector |
| `PHYSICS_MODE` | `FAST` (30Hz), `BALANCED` (60Hz), `ACCURATE` (120Hz) | `BALANCED` | Simulation tick preset |
