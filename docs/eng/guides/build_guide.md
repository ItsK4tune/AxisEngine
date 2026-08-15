# Build Guide & CMake Configuration

> [Tiếng Việt](../../vi/guides/build_guide.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine relies on **CMake 3.20+** for cross-platform project generation. The build system compiles the `Axis::Engine` static library, the optional `Axis::Editor` visual editor library, automated tests, and scene compiler utilities.

---

## 2. How to Use

1. **Select CMake Preset**: Choose a preset from `CMakePresets.json` (`windows-msvc`, `windows-msvc-editor`, `linux-ninja`, `linux-ninja-editor`).
2. **Configure Engine**: Run `cmake --preset <preset_name>` from repository root.
3. **Build Binary Targets**: Execute `cmake --build build --config Release --parallel`.
4. **Run Sample / Tests**: Launch `axis_samples.exe` or execute `ctest --test-dir build`.

---

## 3. Examples

### 1. Building Editor & Samples via CLI Example
```powershell
# Configure with MSVC Editor Preset
cmake --preset windows-msvc-editor

# Build Release binaries in parallel
cmake --build build --config Release --parallel

# Run sample application
.\build\bin\Release\axis_samples.exe
```

### 2. Consuming AxisEngine Package Example (`CMakeLists.txt`)
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
find_package(AxisEngine REQUIRED)

add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE Axis::Engine)
```

---

## 4. API & Configuration Reference

### CMake Presets Reference Matrix

| Preset Name | Platform | Editor Status | Generator / Compiler |
| :--- | :--- | :--- | :--- |
| `windows-msvc` | Windows | `OFF` | MSVC 2022 Static Engine Library |
| `windows-msvc-editor` | Windows | `ON` | MSVC 2022 + ImGui Editor + `axis_samples` |
| `linux-ninja` | Linux | `OFF` | GCC / Clang + Ninja Generator |
| `linux-ninja-editor` | Linux | `ON` | GCC / Clang + Ninja + Editor + Samples |

### CMake Configuration Options Reference

| Option Key | Default | Values | Description |
| :--- | :--- | :--- | :--- |
| `ENABLE_EDITOR` | `OFF` | `ON` / `OFF` | Compiles ImGui editor library (`Axis::Editor`) |
| `BUILD_SAMPLES` | `ON` | `ON` / `OFF` | Compiles `axis_samples` executable |
| `ENABLE_TESTS` | `OFF` | `ON` / `OFF` | Compiles unit/integration test suite (`axis_test`) |
| `ENABLE_LTO` | `ON` | `ON` / `OFF` | Enables Link-Time Optimization (IPO) |
| `ENABLE_PCH` | `ON` | `ON` / `OFF` | Enables precompiled header support |
| `AXIS_GRAPHICS_BACKEND` | `OpenGL`| `OpenGL` | Core rendering strategy provider |
| `AXIS_PHYSICS_BACKEND` | `Bullet` | `Bullet` | Physics simulation strategy provider |
| `AXIS_AUDIO_BACKEND` | `Null` | `Null`, `FMOD`, `IrrKlang` | Audio playback strategy provider |
