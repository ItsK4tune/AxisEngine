# Graphics & Rendering System Guide

> [Tiếng Việt](../../vi/guides/graphics.md) | [Components Reference](components_reference.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine implements an **OpenGL 4.6 Physically-Based Rendering (PBR)** graphics engine. Supporting Forward and Deferred pipelines, the renderer handles metallic-roughness PBR materials, dynamic shadow mapping, post-processing (HDR, Bloom, TAA), environment reflection probes, particle systems, and video textures.

---

## 2. How to Use

1. **Setting Up Mesh & PBR Material**: Add a `MeshRendererComponent` to an entity and specify albedo/normal textures, roughness, and metallic properties.
2. **Adding Scene Lighting**: Add `DirectionalLightComponent`, `PointLightComponent`, or `SpotLightComponent` entities.
3. **Configuring Post-Processing**: Enable `HDR_ENABLED`, `BLOOM_ENABLED`, and `ANTIALIASING` in `AppConfig` or `axis_config.axs`.

---

## 3. Examples

### 1. PBR Material Setup Example
```cpp
#include <axis_sdk.h>

void CreatePBRSphere(Scene& scene) {
    auto entity = scene.CreateEntity("PBR Sphere");

    auto& transform = entity.AddComponent<TransformComponent>();
    transform.SetPosition(Vector3(0.0f, 1.0f, 0.0f));

    auto& renderer = entity.AddComponent<MeshRendererComponent>();
    renderer.modelPath = "models/sphere.obj";
    renderer.albedoTexture = "textures/metal_d.png";
    renderer.normalTexture = "textures/metal_n.png";
    renderer.roughness = 0.2f;
    renderer.metallic = 0.9f;
}
```

### 2. Directional Sun Light Example
```cpp
#include <axis_sdk.h>

void SetupSunlight(Scene& scene) {
    auto sun = scene.CreateEntity("Sun Light");

    auto& transform = sun.AddComponent<TransformComponent>();
    transform.SetRotation(Vector3(45.0f, -30.0f, 0.0f));

    auto& light = sun.AddComponent<DirectionalLightComponent>();
    light.color = Vector3(1.0f, 0.95f, 0.85f);
    light.intensity = 3.0f;
    light.castShadows = true;
}
```

---

## 4. API & Configuration Reference

### Graphics Pipeline Features & Settings Reference

| Feature / Setting Key | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `GRAPHICS_API` | `Enum` | `OPENGL` | Core rendering API strategy provider |
| `ANTIALIASING` | `Enum` | `TAA` | Anti-aliasing algorithm (`NONE`, `FXAA`, `TAA`) |
| `HDR_ENABLED` | `bool` | `true` | Enables high dynamic range rendering buffers |
| `TONEMAPPING` | `Enum` | `ACES` | Tonemapping curve selection (`NONE`, `REINHARD`, `ACES`) |
| `BLOOM_ENABLED` | `bool` | `true` | Enables post-processing glow pass |
| `SHADOWS` | `bool` | `true` | Master shadow rendering toggle |
| `SHADOW_RESOLUTION` | `int` | `2048` | Depth map resolution in pixels (`512` to `4096`) |
| `SPATIAL_CULLING` | `Enum` | `AUTO` | Spatial culling backend (`AUTO`, `LINEAR`, `OCTREE`) |
