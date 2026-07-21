# Public API Surface

Axis separates its installed headers by intended stability and use.

| Header | Intended consumer | Contract |
|---|---|---|
| `axis_sdk.h` | Games and tools | Stable application, state, component, and public type surface |
| `axis_plugin.h` | Replaceable modules | Backend, lifecycle, resource-library, navigation, network, editor, serializer, and post-process interfaces |
| `axis_advanced.h` | Engine integrations | Concrete systems/managers; supported but more likely to change |
| `axis_all.h` | Source compatibility | Union of the three surfaces; avoid in reusable plugins |

Headers below any `strategy/` directory and built-in editor `modules/` or `panels/` are implementation details and are excluded from the installed SDK. `ServiceLocator` is a runtime mechanism, not an application API; use `EngineAccessor` or an interface from `axis_plugin.h`.

`StaticBatchManager` and `TextureAtlas` remain internal benchmark/asset-tool implementations and are also excluded from installed headers: neither has a renderer contract that preserves material identity and entity picking yet. Their focused unit tests remain to protect the portable batch format while they are internal.

## Ownership and replacement rules

- `Application` owns backend objects returned by `AppBuilder` factories.
- Resource-library overrides are `shared_ptr` values retained by the application's builder profile.
- Platform-provider acquisition returns `shared_ptr`, so an in-flight caller remains valid during a provider swap.
- Systems, scripts, component codecs, post-process effects, and editor extensions use explicit registration through `ISystemRegistry`, `IScriptRegistry`, `IComponentCodecRegistry`, `IPostProcessRegistry`, and `IEditorExtensionRegistry`. Owner-based registries must be unregistered before unloading module code.
- Localization is consumed through `ILocalizationService`; a replacement system can register that service without exposing `LocalizationSystem` to gameplay code.
- Only one `Application` may be initialized at a time. Its service context is visible to job workers; nested test/tool contexts remain thread-local.

## Current provider matrix

The shipped graphics backend is OpenGL and the shipped physics backend is Bullet. Playback supports Null plus optional FMOD or irrKlang builds. Windows microphone capture uses WASAPI; other platforms use the explicit unsupported capture service until a provider is installed. Vulkan, DirectX, PhysX, and OpenAL remain enum values for serialized/source compatibility but are not offered as selectable build providers.
