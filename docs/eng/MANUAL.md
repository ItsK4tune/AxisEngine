# AxisEngine manual

> [Tiếng Việt](../vi/MANUAL.md)

## 1. Product scope

AxisEngine is a C++20 static engine library with an optional static editor
library. It is intended for native games, simulations, visualization tools, and
engine experimentation. It is not currently a sandbox for untrusted assets or
a turnkey network service.

The runtime is built around:

1. an `Application` that owns provider and lifecycle objects;
2. a stack-based `StateMachine`;
3. an EnTT `Scene` registry;
4. systems registered through `SystemFactory`/`ISystemRegistry`;
5. services activated through an application-local `ServiceLocator`;
6. data-driven `.axs` scenes and an optional `.axsb` compilation step.

Only one initialized `Application` may be process-active at a time. Several
global facilities remain process-wide, including the event manager, job system,
logger, profiler, and active-service bridge.

## 2. Provider matrix

| Capability | Available | Selection |
| --- | --- | --- |
| Graphics | OpenGL | `AXIS_GRAPHICS_BACKEND=OpenGL` |
| Physics | Bullet | `AXIS_PHYSICS_BACKEND=Bullet` |
| Playback audio | Null | `AXIS_AUDIO_BACKEND=Null` (default) |
| Playback audio | FMOD | `AXIS_AUDIO_BACKEND=FMOD`, plus FMOD SDK |
| Playback audio | irrKlang | `AXIS_AUDIO_BACKEND=IrrKlang`, plus irrKlang SDK |
| Capture audio | WASAPI | Built into Windows platform sources |
| Capture audio | Null/unsupported | Non-Windows default |
| Window/input | GLFW | Built with the OpenGL provider |

Vulkan, DirectX, PhysX, and OpenAL are not implemented by this release.
macOS source and CMake presets exist, but the shipped renderer requires OpenGL
4.6 while Apple's implementation ends at 4.1. Treat macOS as build-level
platform work, not a supported rendering target.

## 3. Build products

| Option | Default | Result |
| --- | --- | --- |
| `ENABLE_EDITOR` | `OFF` | Adds `Axis::Editor` / `axis_editor` |
| `BUILD_SAMPLES` | `ON` | Builds `axis_samples` only when the editor is enabled |
| `ENABLE_TESTS` | `OFF` | Builds `axis_test` and registers it with CTest |
| `ENABLE_LTO` | `ON` | Enables IPO for optimized configurations when supported |
| `ENABLE_PCH` | `ON` | Uses the private precompiled header |
| `ENABLE_UNITY_BUILD` | `ON` | Batches safe translation units |
| `ENABLE_PARALLEL_COMPILE` | `ON` | Enables MSVC `/MP` |

The scene compiler target, `axis_compile`, is `EXCLUDE_FROM_ALL`; build it
explicitly. Complete commands are in the [build guide](guides/build_guide.md).

## 4. Public header policy

- `axis_sdk.h`: preferred application-facing umbrella.
- `axis_plugin.h`: replaceable providers and extension contracts.
- `axis_advanced.h`: supported lower-level integration with more upgrade cost.
- `axis_all.h`: source-tree convenience umbrella; prefer the narrower headers
  in external consumers.
- `engine/**/strategy/**`: backend implementation, excluded from installation.

Installed CMake targets are `Axis::Engine` and, when built, `Axis::Editor`.
Built-in runtime assets install to `share/AxisEngine/assets`.

## 5. Application lifecycle

A consumer subclasses `Application`, optionally overrides registration hooks,
initializes a validated `AppConfig`, pushes a `State`, and calls `Run`.

```cpp
#include <axis_sdk.h>

class BootState final : public State
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
```

`Application::Initialize` creates providers, services, resources, systems, and
the runtime core. `Run` processes the active state and shuts the application
down when the loop ends. Calling `Shutdown` is safe and idempotent.

Headless mode skips the window and renderer, but core initialization may still
need configured compile-time dependencies. `axis_compile` uses this path.

## 6. State, scene, entity, and script workflow

Use a `State` for a global mode such as boot, menu, gameplay, pause, or results.
Use a `Scriptable` for behavior attached to one entity. Use an ECS system for
logic spanning many entities or requiring a defined update/render phase.

Typical flow:

1. Register script factories in `Application::RegisterUserScripts`.
2. Push an initial state after successful initialization.
3. Load a scene through the state/scene manager, or construct entities with
   `EntityBuilder`.
4. Attach `Script` components by registered factory name.
5. Keep transform data coherent by using scene/entity APIs or calling
   `MarkTransformDirty` after direct component mutation.

Scene hierarchy operations reject ordinary cycles and can preserve current and
previous world transforms during reparenting. Runtime systems still assume
components have not been corrupted through raw registry access.

## 7. Scene and asset formats

`.axs` is a deliberately small indentation-based format parsed by
`YAMLParser`; it is not a complete YAML implementation. Use spaces for
indentation, keep one `key: value` pair per line, and follow the structures in
the [scene guide](guides/scene_format.md). Tab indentation is rejected with a
line/column diagnostic. Avoid anchors, aliases, flow collections, multiline
scalars, and other general YAML syntax.

`axis_compile input.axs output.axsb` creates the current binary scene format.
The binary format is versioned and includes a serialized `.axs` payload in the
current version. Binary loading applies configurable file, payload, string, and
entity limits; legacy loads roll back scene changes on failure. Treat both
formats and all referenced assets as trusted project content.

Resources may be declared under `Resources:` or loaded through
`ResourceManager`. Texture decoding can run asynchronously; GPU upload and
completion events occur during manager updates. Model/texture names are aliases
and the managers deduplicate physical paths.

`asset://path` resolves built-in engine content. Game content normally uses
project-relative paths. Absolute paths are accepted, which is useful for tools
but means paths are not a security boundary.

## 8. Runtime systems

- Rendering supports forward/deferred paths, PBR materials, lights, shadows,
  post-processing, decals, terrain, particles, reflection probes, planar
  reflections, UI, video textures, culling, and batching.
- Physics uses Bullet rigid bodies, collision shapes, constraints, ray queries,
  character controllers, collision filtering, and transform synchronization.
- Navigation provides navmesh generation, tiled dirty-region rebuilds, grid
  pathfinding, path following, tags, and custom cost rules.
- Audio provides 2D/3D playback via the selected backend. Capture is a separate
  service.
- Networking uses ENet for client/server messaging and transform replication.
  A versioned protocol envelope validates packet kind, size, and sequence.
  Secure mode requires an application-supplied `INetworkSecurityProvider`.

Use the subsystem guides from the [documentation index](INDEX.md) for component
and configuration details.

## 9. Editor workflow

Build with `ENABLE_EDITOR=ON` and link `Axis::Editor`. The editor bootstrap
registers `EditorSystem` before application systems are created. `axis_samples`
is the reference editor host.

The editor offers hierarchy/inspector, project assets, file browsing, resource
preview, prefab operations, input actions, lighting/lightmap tools, navigation,
network inspection, profiling, frame debugging, animation/VFX graphs, console,
settings, and play/edit/stop state.

Editor operations modify real project files. Keep the project in version
control. File Hierarchy defaults to the canonical project root, creates files
exclusively, generates unique duplicate names, and refuses rename conflicts.
Explicit save/apply operations can still replace their selected output.

## 10. Networking and security

ENet provides reliable/unreliable UDP channels, not application security.
`NetworkConfig` defaults to `RequireSecure`; startup fails unless a registered
`INetworkSecurityProvider` can authenticate peers, seal/open packets, and
authorize decoded messages. AxisEngine does not ship a cryptographic provider.
`TrustedNetwork` explicitly bypasses these protections and logs a warning.

- Bind to an explicit interface and verify the logged bound address.
- The editor defaults its host/bind field to `127.0.0.1`; clearing it is an
  explicit request to bind every local interface.
- Register a reviewed security provider before exposing traffic to the Internet.
- Validate message types, sizes, rates, identity, and permissions in the game.
- Never trust replicated transforms as authoritative game state.
- Do not pass untrusted scene, shader, model, video, audio, prefab, or path data
  into the current runtime.

The engine applies per-update event/byte/time budgets, but those are performance
controls rather than a complete denial-of-service defense.

## 11. Testing and release checklist

Before release:

1. Configure the intended graphics/physics/audio combination.
2. Build both Debug and Release.
3. Run `ctest` with tests enabled.
4. Build the installed package consumer.
5. Run representative sample scenes on the target GPU and audio devices.
6. Test corrupt/missing assets with strict loading enabled.
7. Verify server bind address and firewall exposure.
8. Package `share/AxisEngine/assets` and required backend DLL/shared libraries.
9. Add a project license before redistribution.

The repository test suite is strong for headless core, scene, serialization,
physics, navigation, and scripting contracts. It does not currently provide
meaningful automated coverage of real OpenGL rendering, complete ENet sessions,
FMOD/irrKlang devices, or WASAPI hardware. Protocol validation and editor file
conflict/root behavior have headless automated tests.

## 12. Known limits

See the [source audit](audit/source_audit_2026-07-23.md) for evidence and
priorities. The most important operational limits are:

- trusted-input assumption for scenes/assets;
- no built-in cryptographic network provider;
- OpenGL-only renderer and Bullet-only physics;
- Windows-only microphone implementation;
- no repository license;
- `TrustedNetwork` is intentionally unauthenticated and unencrypted.
