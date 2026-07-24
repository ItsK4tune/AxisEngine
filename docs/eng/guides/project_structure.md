# AxisEngine repository and consumer-project structure

> [Tiếng Việt](../../vi/guides/project_structure.md)

## Repository

```text
AxisEngine/
  include/                 Public umbrellas and engine headers
    axis_sdk.h             Preferred game/tool API
    axis_plugin.h          Provider and extension contracts
    axis_advanced.h        Lower-level supported integration
    engine/asset/          Built-in runtime assets in the source tree
  src/
    audio/                 Playback/capture services and providers
    core/                  Application, loop, state, jobs, events, config
    ecs/                   Entities, components, systems, builders
    editor/                Optional ImGui editor
    navigation/            Navmesh and pathfinding
    network/               ENet service and transform replication
    physics/               Physics logic and Bullet provider
    platform/              Filesystem/runtime/input/window implementations
    render/                Renderer and OpenGL provider
    resource/              Asset managers and caches
    scene/                 Scene, validation, text/binary serialization
    script/                Script registry and Scriptable behavior
  sample/                  Reference app, assets, scene, scripts, 33 scenarios
  compiler/                axis_compile source
  tests/                   Unit, integration, API, package-consumer tests
  template/                Custom shader templates
  docs/
    eng/                   Complete English documentation tree
    vi/                    Complete Vietnamese documentation tree
    testcases/             Language-neutral test artifacts
  cmake/                   Find modules, package config, triplets, overlays
  CMakeLists.txt           Targets, options, install/export rules
  CMakePresets.json        Windows/Linux/macOS and CI presets
  vcpkg.json               Dependency manifest
```

There is no repository-level `game/`, `resources/`, or `scenes/` directory.
Game-facing examples live under `sample/`. Third-party source is not embedded
under `include/`; dependencies are supplied through packages.

Build output goes to `build/`:

```text
build/
  bin/<Config>/            Multi-config executables and runtime DLLs
  lib/<Config>/            Static/import libraries
  tests/                   Generated test project files
  vcpkg_installed/         Build-local vcpkg packages when used
```

Single-config generators normally use flat `build/bin` and `build/lib`.

## Recommended consumer project

Keep game code outside the engine repository:

```text
MyGame/
  CMakeLists.txt
  config.axs
  assets/
    scenes/
    prefabs/
    models/
    textures/
    shaders/
    audio/
    video/
    fonts/
    input/
    localization/
  src/
    main.cpp
    states/
    scripts/
    systems/
    editor/                 Optional game-specific editor extensions
  tests/
```

Link `Axis::Engine` for the runtime and `Axis::Editor` only for an editor host.
Use project-relative paths for game content and `asset://` only for built-in
AxisEngine assets.

See the [English build guide](build_guide.md) or
[Vietnamese build guide](../../vi/guides/build_guide.md).
