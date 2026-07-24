# Getting started with AxisEngine

> [Tiếng Việt](../../vi/core/getting_started.md)

## 1. Build the reference sample

On Windows with a configured vcpkg toolchain:

```powershell
cmake --preset windows-msvc-editor `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

The sample is the fastest way to verify the renderer, editor bootstrap,
resources, input, and runtime on your machine. It contains 33 scenarios under
`sample/src/scenarios`.

If dependencies are already available through another CMake setup, the
toolchain argument may be omitted. See the
[build guide](../guides/build_guide.md).

## 2. Create a consumer project

Recommended layout:

```text
MyGame/
  CMakeLists.txt
  config.axs                 # optional runtime configuration
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

`CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyGame LANGUAGES CXX)

find_package(AxisEngine CONFIG REQUIRED)

add_executable(my_game
    src/main.cpp
)
target_link_libraries(my_game PRIVATE Axis::Engine)
target_compile_features(my_game PRIVATE cxx_std_20)
```

## 3. Add the application and state

```cpp
#include <axis_sdk.h>

class GameState final : public State
{
public:
    std::string GetName() const override { return "Game"; }
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
    config.logLevel = LogLevel::Verbose;

    if (!app->Initialize(config))
        return 1;

    app->PushState<GameState>();
    app->Run();
    return 0;
}
```

`State` has four required methods: `OnEnter`, `OnUpdate`, `OnRender`, and
`OnExit`. Fixed update, debug render, pause, and resume hooks are optional.

## 4. Add an `.axs` scene

AxisEngine uses an indentation-based YAML-like subset. It is not general YAML;
use spaces rather than tabs.

```yaml
axis_scene:
  Resources:
    Model:
      Name: cube
      Path: assets/models/cube.fbx
      Static: true
    Shader:
      Name: lit
      Vertex: assets/shaders/lit.vs
      Fragment: assets/shaders/lit.fs
  Entities:
    Cube:
      Tag: World
      Component: Transform
        Position: 0 0 0
        Rotation: 0 0 0
        Scale: 1 1 1
      Component: Renderer
        Model: cube
        Shader: lit
        CastShadow: true
        ReceiveShadow: true
      Component: Material
        Metallic: 0.0
        Roughness: 0.5
        AO: 1.0
```

Use the exact resource/component keys documented by the
[scene format](../guides/scene_format.md) and
[component reference](../guides/components_reference.md). The repository's
`sample/scene/sample.axs` is a serializer-produced example.

## 5. Add a script

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

Register it before scene loading:

```cpp
void RegisterUserScripts() override
{
    RegisterScript<RotateScript>("RotateScript");
}
```

Attach it in the scene using the `Script` component syntax from the
[scriptable API](../scripting/scriptable_api.md).

## 6. Test

Engine repository tests:

```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

For your own game, add tests around state transitions, serialization, script
logic, and any network protocol built above ENet.

## 7. Common failures

- **Initialization fails:** inspect `logs/`, verify OpenGL 4.6, dependency
  DLLs/shared libraries, and selected backend SDKs.
- **Default assets fail:** package `share/AxisEngine/assets` or call
  `FileSystem::setEngineAssetRoot` through advanced integration.
- **Script not found:** register the exact string used by the scene before it
  loads.
- **Model/texture not found:** paths resolve from the detected executable/
  project root; inspect the resolved path in logs.
- **Scene parses strangely:** use spaces, the AxisEngine subset, and validate
  the scene; do not use arbitrary YAML features.
- **macOS renderer fails:** the shipped renderer requires OpenGL 4.6 and is not
  supported on macOS.
