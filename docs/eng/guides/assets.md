# Assets & Resource Management Guide

> [Tiếng Việt](../../vi/guides/assets.md) | [Scene Format](scene_format.md) | [Managers & Services](../core/managers.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine handles models, textures, audio clips, and GLSL shaders using a centralized `ResourceManager`. The asset subsystem handles reference counting, path deduplication, background async loading, and memory purging.

---

## 2. How to Use

1. **Loading Assets**: Call `ResourceManager::Get().LoadTexture("path.png")` or `LoadModel("model.obj")`.
2. **Built-in Assets**: Address built-in engine assets using `asset://` URI scheme (`asset://textures/white.png`).
3. **Purging Memory**: Call `resources.UnloadUnusedResources()` to release unreferenced assets from RAM/GPU.

---

## 3. Examples

### 1. Asset Loading Example
```cpp
#include <axis_sdk.h>

void LoadAssets() {
    auto& resources = ResourceManager::Get();

    auto albedo = resources.LoadTexture("textures/brick_d.png");
    auto model = resources.LoadModel("models/house.obj");

    if (albedo && model) {
        AXIS_LOG_INFO("Assets cached successfully!");
    }
}
```

### 2. Purging Memory Example
```cpp
#include <axis_sdk.h>

void CleanupCache() {
    ResourceManager::Get().UnloadUnusedResources();
}
```

---

## 4. API & Configuration Reference

### Asset Formats & Loaders Reference Matrix

| Asset Category | Formats | Loader Class | Cache Key Pattern |
| :--- | :--- | :--- | :--- |
| **3D Models** | `.obj`, `.gltf`, `.glb` | `ModelLoader` | Model file relative path |
| **Textures** | `.png`, `.tga`, `.jpg`, `.dds` | `TextureLoader` | Texture file relative path |
| **Audio Clips** | `.wav`, `.ogg`, `.mp3` | `AudioLoader` | Sound file relative path |
| **Shaders** | `.glsl`, `.vert`, `.frag` | `ShaderCompiler` | Shader source file path |
| **Scenes** | `.axs` (YAML), `.axsb` (Binary) | `SceneManager` | Scene file path |
