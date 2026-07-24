<p align="center">
  <img src="include/engine/asset/project/logo.png" alt="AxisEngine logo" width="220">
</p>

# AxisEngine

AxisEngine is a C++20, ECS-based game and multimedia engine. The current release
ships an OpenGL renderer and Bullet physics, with Null audio by default and
optional FMOD or irrKlang playback backends. It also includes scene
serialization, scripting, navigation, networking, video playback, an optional
ImGui editor, a scene compiler, and a sample application containing 33
scenarios.

This repository is an engine/framework, not a ready-made game. The executable
produced by the sample build is `axis_samples`; applications using the engine
provide their own `Application` subclass and initial `State`.

[Bilingual landing page](README.md) | [Vietnamese README](README.vn.md) |
[English documentation](docs/eng/INDEX.md) | [English manual](docs/eng/MANUAL.md) |
[Vietnamese documentation](docs/vi/INDEX.md)

## Current support

| Area | Shipped implementation | Notes |
| --- | --- | --- |
| Language | C++20 | CMake 3.20 or newer |
| Graphics | OpenGL | The renderer requires OpenGL 4.6; it is not usable on macOS |
| Physics | Bullet | Selected at configure time |
| Audio playback | Null, FMOD, irrKlang | Null is the dependency-free default; FMOD/irrKlang require their SDKs |
| Microphone capture | WASAPI on Windows | Other platforms expose an explicit unsupported service |
| Platforms | Windows, Linux; limited macOS build support | macOS cannot run the shipped OpenGL 4.6 renderer |
| Editor | ImGui, optional | Enable with `ENABLE_EDITOR=ON` |
| Scene formats | `.axs`, `.axsb` | `.axs` is AxisEngine's YAML-like subset; `.axsb` is compiled binary |

Vulkan, DirectX, PhysX, and OpenAL values remain in some enums for source and
serialization compatibility, but this release does not provide those backends.

## Build

The default configure builds the static engine library only:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-release
```

Build the editor and sample application:

```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

Build and run the automated tests:

```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

On Linux, use `linux-ninja` or `linux-ninja-editor` and omit `-C Release`
when using a single-configuration build directory. See the
[build guide](docs/eng/guides/build_guide.md) for dependencies, audio backend
selection, installation, and consumer-project setup.

The interactive helpers are `axis_tools.bat` and `axis_tools.sh`. They are
convenience wrappers; the CMake commands above are the canonical workflow.

## Minimal application

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

Use `axis_sdk.h` for games and tools, `axis_plugin.h` for provider/module
contracts, and `axis_advanced.h` only when lower-level system integration is
required. Backend headers under `strategy/` are implementation details and are
not installed as part of the SDK.

## Repository map

| Path | Purpose |
| --- | --- |
| `include/` | Public umbrellas plus engine headers |
| `src/` | Engine and editor implementation |
| `sample/` | Sample app, resources, scenes, scripts, and 33 scenarios |
| `compiler/` | `axis_compile`, the `.axs` to `.axsb` compiler |
| `tests/` | Unit, integration, API, and package-consumer tests |
| `template/` | Custom shader templates |
| `docs/` | Guides, references, manuals, and audit reports |
| `cmake/` | package config, find modules, sources, triplets, and overlays |

## Documentation

Start with the [manual](docs/eng/MANUAL.md). Detailed references are organized
in the [documentation index](docs/eng/INDEX.md), including:

- [Getting started](docs/eng/core/getting_started.md)
- [Architecture and API boundaries](docs/eng/core/architecture.md)
- [Scene format](docs/eng/guides/scene_format.md)
- [Components](docs/eng/guides/components_reference.md)
- [Configuration](docs/eng/guides/configuration.md)
- [Scripting](docs/eng/scripting/scriptable_api.md)
- [Editor](docs/eng/guides/editor.md)
- [Latest source audit](docs/eng/audit/source_audit_2026-07-23.md)
- [Audit remediation](docs/eng/audit/remediation_2026-07-23.md)

## Security and trust boundary

AxisEngine treats scenes and assets as trusted project input. Network sessions
default to `RequireSecure` and refuse startup without a registered
`INetworkSecurityProvider`. The explicit `TrustedNetwork` mode uses plain ENet
without authentication or encryption and must not be exposed to the Internet.
AxisEngine does not ship a cryptographic provider; production applications must
supply a reviewed implementation. See the
[remediation report](docs/eng/audit/remediation_2026-07-23.md).

## License

No license file is currently included. The repository must not be described or
redistributed as MIT-licensed until an explicit license is added. Optional FMOD
and irrKlang SDKs also have their own licensing terms.
