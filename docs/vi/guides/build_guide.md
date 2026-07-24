# Hướng dẫn build AxisEngine

> [English](../../eng/guides/build_guide.md)

## Yêu cầu

- CMake 3.20 trở lên.
- Compiler C++20.
  - Windows: Visual Studio 2022/MSVC là đường build được duy trì chính.
  - Linux: GCC hoặc Clang với Ninja/Makefiles.
- Git và Git LFS.
- Dependency từ `vcpkg.json` hoặc system package tương thích.

Null audio là mặc định. FMOD và irrKlang cần SDK riêng, không được vendored.

## Windows

Engine-only:

```powershell
cmake --preset windows-msvc `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build --preset windows-release
```

Editor và sample:

```powershell
cmake --preset windows-msvc-editor `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

## Linux

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

Renderer hiện cần OpenGL 4.6 và driver tương thích.

## macOS

Repository có preset và platform source cho macOS, nhưng renderer dùng OpenGL/
GLSL 4.6 trong khi macOS chỉ hỗ trợ OpenGL 4.1. Renderer hiện không phải runtime
target được hỗ trợ trên macOS.

## Option chính

```text
ENABLE_EDITOR=OFF
BUILD_SAMPLES=ON
ENABLE_TESTS=OFF
ENABLE_LTO=ON
ENABLE_PCH=ON
ENABLE_UNITY_BUILD=ON
AXIS_GRAPHICS_BACKEND=OpenGL
AXIS_PHYSICS_BACKEND=Bullet
AXIS_AUDIO_BACKEND=Null
```

`BUILD_SAMPLES` chỉ có tác dụng khi `ENABLE_EDITOR=ON`.

## Test

```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Scene compiler

```powershell
cmake --build build --config Release --target axis_compile
.\build\bin\Release\axis_compile.exe input.axs output.axsb
```

Với Ninja/Makefiles:

```bash
cmake --build build --target axis_compile
./build/bin/axis_compile input.axs output.axsb
```

Menu Windows hiện có known issue về path multi-config; dùng lệnh trực tiếp cho
đến khi `axis_tools.bat` được sửa.

## Install và dùng từ project khác

```powershell
cmake --install build --config Release --prefix C:\AxisEngine
```

```cmake
find_package(AxisEngine CONFIG REQUIRED)
add_executable(my_game src/main.cpp)
target_link_libraries(my_game PRIVATE Axis::Engine)
target_compile_features(my_game PRIVATE cxx_std_20)
```

Configure consumer bằng `-DCMAKE_PREFIX_PATH=C:/AxisEngine` và cung cấp cùng
dependency package. Asset nội bộ nằm ở `share/AxisEngine/assets`; cần đóng gói
thư mục này cùng DLL/shared library tương ứng.
