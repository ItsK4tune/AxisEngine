# Bắt đầu với AxisEngine

> [English](../../eng/core/getting_started.md)

## 1. Build sample tham chiếu

Trên Windows:

```powershell
cmake --preset windows-msvc-editor `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

Sample gồm 33 scenario và là cách nhanh nhất để kiểm tra renderer, editor,
resource, input và runtime. Xem
[hướng dẫn build](../guides/build_guide.md) nếu dependency chưa được cấu
hình.

## 2. Cấu trúc project

```text
MyGame/
  CMakeLists.txt
  config.axs
  assets/
    scenes/
    models/
    textures/
    audio/
    shaders/
  src/
    main.cpp
    game_state.h
    scripts/
```

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyGame LANGUAGES CXX)

find_package(AxisEngine CONFIG REQUIRED)
add_executable(my_game src/main.cpp)
target_link_libraries(my_game PRIVATE Axis::Engine)
target_compile_features(my_game PRIVATE cxx_std_20)
```

## 3. Application và state đầu tiên

```cpp
#include <axis_sdk.h>

class GameState final : public State
{
public:
    void OnEnter() override {}
    void OnUpdate(float) override {}
    void OnRender() override {}
    void OnExit() override {}
};

class GameApplication final : public Application
{
public:
    void RegisterUserScripts() override
    {
        // RegisterScript<PlayerController>("PlayerController");
    }
};

int main()
{
    auto app = std::make_shared<GameApplication>();
    AppConfig config;
    config.title = "My AxisEngine Game";
    config.window.width = 1280;
    config.window.height = 720;

    if (!app->Initialize(config))
        return 1;
    app->PushState<GameState>();
    app->Run();
    return 0;
}
```

`OnEnter`, `OnUpdate`, `OnRender`, `OnExit` là bắt buộc. Fixed update, debug
render, pause và resume là hook tùy chọn.

## 4. Scene `.axs`

`.axs` là YAML-like subset riêng, không phải YAML đầy đủ. Dùng space, không dùng
tab.

```yaml
axis_scene:
  Resources:
  Entities:
    Cube:
      Tag: World
      Component: Transform
        Position: 0 0 0
        Rotation: 0 0 0
        Scale: 1 1 1
```

Tra key chính xác trong
[scene format](../guides/scene_format.md) và
[component reference](../guides/components_reference.md).

## 5. Script

```cpp
#pragma once
#include <axis_sdk.h>

class RotateScript final : public Scriptable
{
public:
    void OnUpdate(float dt) override
    {
        auto& rotation = GetComponent<RotationComponent>();
        rotation.value =
            glm::normalize(glm::angleAxis(glm::radians(45.0f * dt),
                                         glm::vec3(0.0f, 1.0f, 0.0f)) *
                           rotation.value);
        MarkTransformDirty();
    }
};
```

Đăng ký bằng đúng tên dùng trong scene:

```cpp
void RegisterUserScripts() override
{
    RegisterScript<RotateScript>("RotateScript");
}
```

## 6. Test

```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

## 7. Lỗi thường gặp

- Initialize fail: xem `logs/`, kiểm tra OpenGL 4.6, DLL/shared library và SDK.
- Default asset fail: đóng gói `share/AxisEngine/assets`.
- Script not found: đăng ký đúng tên trước khi load scene.
- Asset not found: xem resolved path trong log.
- Scene parse sai: dùng space và chỉ dùng AxisEngine subset.
- macOS renderer fail: renderer hiện yêu cầu OpenGL 4.6 và không hỗ trợ macOS.
