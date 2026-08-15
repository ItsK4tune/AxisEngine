# Project Structure Guide

> [Tiếng Việt](../../vi/guides/project_structure.md) | [Build Guide](build_guide.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine organizes code into distinct top-level directories (`include/`, `src/`, `sample/`, `compiler/`, `tests/`, `docs/`, `cmake/`) to isolate public headers from implementation code, test suites, assets, and tools.

---

## 2. How to Use

1. **Developing Games**: Include headers from `include/`, write application logic in your project folder, and load assets relative to the executable or using `asset://`.
2. **Accessing Engine Built-in Assets**: Use the `asset://` URI protocol to resolve engine default shaders, textures, and meshes.
3. **Compiling Scenes**: Use the compiler tool located in `compiler/` (`axis_compile`) to transform `.axs` files to `.axsb`.

---

## 3. Examples

### Path Resolution Example
```cpp
#include <axis_sdk.h>

void LoadAssetsDemo() {
    auto& resources = ResourceManager::Get();

    // 1. Built-in asset URI protocol
    auto checkerTex = resources.LoadTexture("asset://textures/default_checker.png");

    // 2. Project relative asset path
    auto modelMesh = resources.LoadModel("models/hero.obj");
}
```

---

## 4. API & Configuration Reference

### Directory Structure Reference Table

| Directory Path | Purpose & Contents | Output Artifact |
| :--- | :--- | :--- |
| `include/` | Public SDK umbrella headers (`axis_sdk.h`, `axis_plugin.h`, `axis_advanced.h`) | Installed header SDK tree |
| `src/` | Engine sources and `Axis::Editor` ImGui implementation | `axis_engine.lib`, `axis_editor.lib` |
| `sample/` | 33 demo scenarios, test scenes, textures, and shaders | `axis_samples.exe` |
| `compiler/` | `axis_compile` binary scene compiler utility | `axis_compile.exe` |
| `tests/` | Unit, integration, and CTest test cases | `axis_test.exe` |
| `docs/` | Comprehensive bilingual documentation index and guides | Markdown documentation |
| `cmake/` | Package config, find modules, triplets, and vcpkg toolchain | `AxisEngineConfig.cmake` |

### Path Protocols Reference

| Protocol Scheme | Resolution Path | Purpose |
| :--- | :--- | :--- |
| `asset://` | `share/AxisEngine/assets/` | Access built-in fallback engine textures, shaders, meshes |
| Relative Path | Execution Working Dir | Access project-specific models, textures, audio files |
| Absolute Path | Direct File System Path | Developer tools and editor import operations |
