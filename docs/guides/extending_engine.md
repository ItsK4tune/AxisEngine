# Extending Axis Engine

Axis uses instance-owned providers and explicit registration hooks. Game code should start with `axis_sdk.h`; plugin contracts are in `axis_plugin.h`. `axis_advanced.h` exposes concrete systems for source-level integrations with a higher upgrade cost. Backend headers under `engine/*/strategy` are internal and are not installed.

Dynamic navigation providers can call `INavigationService::MarkNavMeshDirty` with a world-space region after changing walkable geometry. The built-in provider queues the intersecting tiles under the configured per-frame budget; custom providers may override the optional method or return the default unsupported result.

## Application providers

Configure an `AppBuilder`, then pass it to the `Application` base constructor. Providers are owned by that application profile; configuring one builder cannot affect another application or test.

```cpp
class GameApplication final : public Application {
public:
    explicit GameApplication(AppBuilder providers)
        : Application(std::move(providers)) {}
};

AppBuilder providers;
providers
    .WithGraphicsContextFactory([](const AppConfig&) {
        return std::make_unique<MyGraphicsContext>();
    })
    .WithPhysicsWorldFactory([](const AppConfig&) {
        return std::make_unique<MyPhysicsWorld>();
    })
    .WithAudioEngineFactory([](const AppConfig&) {
        return std::make_unique<MyAudioEngine>();
    })
    .WithAudioCaptureFactory([] {
        return std::make_unique<MyAudioCaptureService>();
    })
    .WithWindowFactory([] {
        return std::make_unique<MyWindow>();
    });

auto app = std::make_shared<GameApplication>(std::move(providers));
```

A factory must return a valid object. Null results and provider exceptions make `Initialize` fail, roll back initialized subsystems, and release the active application context.
When a custom graphics, physics, or audio factory is present, validation preserves the corresponding backend enum and passes it to that factory unchanged. Without a custom factory, unavailable backend values are reported and replaced by a backend compiled into the build.

Shader, texture, model, sound, font, and skybox facets can be replaced independently with the matching `With*Library(std::shared_ptr<...>)` method. The application retains the provider. The built-in `ResourceManager` still owns core asset bootstrap and resource bookkeeping; an override supplies the public library facet resolved by modules. Post-process and shadow rendering consume `IShaderLibrary` directly.

Platform filesystem and runtime providers use `SetPlatformFileSystemProvider` and `SetPlatformRuntimeProvider`. Acquire a lifetime-safe handle with `AcquirePlatformFileSystem` or `AcquirePlatformRuntime`. Install these during startup; replacing crash/runtime behavior after initialization is not a supported lifecycle operation.

## Systems and scripts

Register custom per-application systems in `Application::RegisterUserSystems`. A system registered there with the same name as a built-in or linked optional system wins for that application. Built-ins are created from an explicit, deterministic catalog.

```cpp
void GameApplication::RegisterUserSystems(ISystemRegistry& systems) {
    systems.RegisterSystem(std::make_unique<MyNavigationSystem>());
}

void GameApplication::RegisterUserScripts() {
    RegisterScript<PlayerController>("PlayerController");
}
```

The editor is an optional linked module. Linking only `Axis::Engine` produces a runtime application without editor code. Linking `Axis::Editor` also propagates `ENABLE_EDITOR` and retains a small bootstrap object that registers `EditorSystem` before application initialization; user code does not register it manually. The bootstrap uses a single anchor symbol rather than forcing the complete static archive into the executable. Scripts are instance-local and a later registration with the same name replaces the earlier factory.

```cmake
target_link_libraries(MyGame PRIVATE Axis::Engine)

# Development/editor target only:
target_link_libraries(MyGameEditor PRIVATE Axis::Editor)
```

Consumers that link the raw static archives instead of the exported CMake targets must preserve the bootstrap anchor themselves (`/INCLUDE:axis_editor_bootstrap_anchor` on 64-bit MSVC, or the platform-equivalent undefined-symbol option). Prefer `Axis::Editor`, which carries this requirement automatically.

## Module-owned registries

- `IPostProcessRegistry` registers a shader effect with an owner id, stable name, priority, rectangle, and input mask. Call `UnregisterOwner` during module unload.
- `IEditorExtensionRegistry` adds module/panel factories and removes every extension for an owner in one call.
- `IComponentCodecRegistry` registers owner-scoped loader/serializer pairs; `IComponentLoaderFactory` and `IComponentSerializerFactory` provide their two directions. Call `UnregisterOwner` before unloading module code (application shutdown also removes remaining owner registrations).
- `INetworkService` and `INavigationService` hide ENet and the built-in pathfinding implementation from gameplay modules.

## Unified file loaders

Implement `ILoaderStrategy`, return a stable type name from `GetName`, then call `ResourceManager::RegisterLoader`. A module strategy registered before `ResourceManager::Initialize` is preserved when the built-in `CONFIG` and `INPUT` defaults are installed.

## Runtime access boundary

`State` and `Scriptable` inherit `EngineAccessor`. Prefer its high-level scene, input, configuration, data-node, timing, rendering, and physics methods. For an advanced service use `Get<T>()`, `Resolve<T>()`, or `GetSystem<T>()`; application code should not depend on `ServiceLocator`.

See [API Surface](../core/api_surface.md), [Microphone Capture](audio_capture.md), and [Custom Post-process ABI](graphics.md#9-custom-post-process-abi).
