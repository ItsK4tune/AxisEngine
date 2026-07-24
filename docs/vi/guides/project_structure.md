# Cấu trúc repository và project sử dụng AxisEngine

> [English](../../eng/guides/project_structure.md)

## Repository

```text
AxisEngine/
  include/                 Public umbrella và engine header
    axis_sdk.h             API game/tool ưu tiên
    axis_plugin.h          Provider và extension contract
    axis_advanced.h        Tích hợp cấp thấp
    engine/asset/          Runtime asset tích hợp
  src/
    audio/                 Playback/capture
    core/                  Application, loop, state, jobs, event, config
    ecs/                   Entity, component, system, builder
    editor/                ImGui editor tùy chọn
    navigation/            Navmesh và pathfinding
    network/               ENet, protocol và replication
    physics/               Physics logic và Bullet
    platform/              Filesystem/runtime/input/window
    render/                Renderer và OpenGL
    resource/              Asset manager/cache
    scene/                 Scene, validation, text/binary serializer
    script/                Script registry và Scriptable
  sample/                  App tham chiếu và 33 scenario
  compiler/                axis_compile
  tests/                   Unit, integration, API, package consumer
  template/                Shader template
  docs/
    eng/                   Toàn bộ tài liệu tiếng Anh
    vi/                    Toàn bộ tài liệu tiếng Việt
    testcases/             Artifact test dùng chung
  cmake/                   Package config, triplet, overlay
  CMakeLists.txt
  CMakePresets.json
  vcpkg.json
```

Repository không có `game/`, `resources/` hoặc `scenes/` ở root; ví dụ nằm trong
`sample/`. Dependency third-party đến từ package manager.

Output multi-config:

```text
build/
  bin/<Config>/
  lib/<Config>/
  tests/
  vcpkg_installed/
```

Generator single-config thường dùng `build/bin` và `build/lib`.

## Project consumer khuyến nghị

```text
MyGame/
  CMakeLists.txt
  config.axs
  assets/
    scenes/ prefabs/ models/ textures/ shaders/
    audio/ video/ fonts/ input/ localization/
  src/
    main.cpp
    states/
    scripts/
    systems/
    editor/
  tests/
```

Link `Axis::Engine` cho runtime, chỉ link `Axis::Editor` với editor host. Dùng
path tương đối project cho game asset; `asset://` dành cho asset tích hợp của
AxisEngine.

Xem [hướng dẫn build](build_guide.md).
