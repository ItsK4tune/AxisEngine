# AxisEngine build guide

> [Tiếng Việt](../../vi/guides/build_guide.md)

## Requirements

- CMake 3.20 or newer.
- A C++20 compiler:
  - Windows: Visual Studio 2022/MSVC is the maintained path.
  - Linux: GCC or Clang with Ninja/Makefiles.
- Git and Git LFS for a complete clone.
- Third-party dependencies from `vcpkg.json` or compatible system packages.

Required libraries are Assimp, Bullet, ENet, EnTT, FFmpeg, FreeType, Glad,
GLFW, GLM, ImGui, and stb. ImGui remains in the manifest even when the editor
is disabled, although CMake only finds/links it for editor builds.

FMOD and irrKlang are optional proprietary SDK integrations and are not
vendored. Null audio is the default.

## Windows with vcpkg

Set `VCPKG_ROOT` to a vcpkg checkout. Configure the engine-only build:

```powershell
cmake --preset windows-msvc `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_OVERLAY_PORTS="$PWD/cmake/vcpkg-overlay-ports" `
  -DVCPKG_OVERLAY_TRIPLETS="$PWD/cmake/vcpkg-triplets"

cmake --build --preset windows-release
```

The local workspace may already inject a vcpkg toolchain through environment or
CMake user configuration; the explicit form above is portable.

Editor and samples:

```powershell
cmake --preset windows-msvc-editor `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"

cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

## Linux

Install compatible development packages or configure with a vcpkg toolchain:

```bash
cmake --preset linux-ninja
cmake --build --preset linux-release
```

Editor/sample:

```bash
cmake --preset linux-ninja-editor
cmake --build build --parallel
./build/bin/axis_samples
```

The shipped OpenGL renderer requires OpenGL 4.6 and a compatible driver.

## macOS limitation

CMake presets and platform filesystem/runtime sources exist, but the current
renderer uses OpenGL 4.6/GLSL 4.6. macOS only exposes OpenGL 4.1. The shipped
renderer therefore fails explicitly and is not a supported macOS runtime.

## Build options

```text
ENABLE_EDITOR=OFF
BUILD_SAMPLES=ON
ENABLE_TESTS=OFF
ENABLE_LTO=ON
ENABLE_PCH=ON
ENABLE_UNITY_BUILD=ON
ENABLE_PARALLEL_COMPILE=ON
AXIS_UNITY_BATCH_SIZE=8
AXIS_GRAPHICS_BACKEND=OpenGL
AXIS_PHYSICS_BACKEND=Bullet
AXIS_AUDIO_BACKEND=Null
```

`BUILD_SAMPLES` only has an effect when `ENABLE_EDITOR=ON`.

Select optional audio:

```powershell
cmake -S . -B build `
  -DAXIS_AUDIO_BACKEND=FMOD `
  -DFMOD_ROOT_DIR="C:/SDK/FMOD"
```

or:

```powershell
cmake -S . -B build `
  -DAXIS_AUDIO_BACKEND=IrrKlang `
  -DIRRKLANG_ROOT_DIR="C:/SDK/irrKlang"
```

## Tests

```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

On a single-config Ninja/Make build, omit `-C Release` unless the build
directory was configured differently.

## Scene compiler

`axis_compile` is excluded from the default build:

```powershell
cmake --build build --config Release --target axis_compile
.\build\bin\Release\axis_compile.exe sample\scene\sample.axs sample\scene\sample.axsb
```

For a single-config build:

```bash
cmake --build build --target axis_compile
./build/bin/axis_compile sample/scene/sample.axs sample/scene/sample.axsb
```

The current Windows helper menu has a known multi-config path issue; use the
direct command above until `axis_tools.bat` is fixed.

## Install and consume

Install:

```powershell
cmake --install build --config Release --prefix C:\AxisEngine
```

Consumer `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyAxisGame LANGUAGES CXX)

find_package(AxisEngine CONFIG REQUIRED)
add_executable(my_game src/main.cpp)
target_link_libraries(my_game PRIVATE Axis::Engine)
target_compile_features(my_game PRIVATE cxx_std_20)
```

Configure the consumer with `-DCMAKE_PREFIX_PATH=C:/AxisEngine` and access to
the same dependency packages. If AxisEngine was built with the editor, the
package also exports `Axis::Editor`.

Installed built-in assets live under `share/AxisEngine/assets`. Ship that
directory beside the install layout, plus required DLL/shared libraries.

## Build hygiene

For release qualification, also run a configuration with PCH and unity disabled
to catch missing direct includes:

```powershell
cmake -S . -B build-portable `
  -DENABLE_PCH=OFF `
  -DENABLE_UNITY_BUILD=OFF `
  -DENABLE_TESTS=ON
```
