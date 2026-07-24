# Engine Managers & Services

> [Tiếng Việt](../../vi/core/managers.md)

Managers are centralized services instantiated by the `Application`. They handle lifecycle, resource state, and OS-level interactions.

---

## 1. ResourceManager
**Include:** `<resource/logic/resource_manager.h>`
**Access from `State`/`Scriptable`:** `Get<ResourceManager>()`

Handles loading, caching, and retrieval of all engine assets.

### Features
- **Deduplication**: Automatically caches textures, shaders, and models by name or path.
- **Asynchronous Loading**: Supports non-blocking asset loading via the Job System.
- **Hot Reloading**: Automatically recompiles shaders and refreshes textures when source files change.
- **Strict Asset Loading**: `STRICT_ASSET_LOADING` disables debug fallback substitutions so failed shader, texture, and model loads surface as load failures.

### Common Methods
- `GetModel(name)`, `GetTexture(name)`, `GetShader(name)`
- `LoadModel(name, path)`, `LoadTexture(name, path)`

---

## 2. SceneManager
Use the `EngineAccessor` scene methods (`LoadScene`, `QueueLoadScene`, `ChangeScene`, `UnloadScene`, `PopScene`) rather than depending on the concrete manager.

Manages the active entity registries and scene transitions.

### Common Methods
- `LoadScene(path)`: Adds entities from an `.axs` file to the current world.
- `ChangeScene(path)`: Clears all active entities and loads a fresh scene.
- `ClearAllScenes()`: Resets the entire world state.

---

## 3. InputManager & Handlers

Use `GetAction`, `GetActionDown`, and `GetActionUp` for gameplay. Advanced raw-device code can explicitly resolve `IOHandler`.

Distributes OS-level keyboard and mouse events.

### Keyboard & Mouse
- **Keyboard**: `Resolve<IOHandler>()->GetKeyboard()` for raw state polling.
- **Mouse**: `Resolve<IOHandler>()->GetMouse()` for raw pointer state.
- **Cursor Modes**: Supports `Normal`, `Hidden`, `Locked`, and `LockedHidden`.

### Input Actions
Recommended for gameplay: Use `GetAction("Jump")` to poll mapped keys defined in configuration.

---

## 4. AudioService
**Include:** `<audio/logic/audio_service.h>`
**Access from `State`/`Scriptable`:** `Resolve<AudioService>()`

Wraps whichever `IAudioEngine` backend was selected or injected. Playback is independent from `IAudioCaptureService` microphone input.

### Methods
- `Play2D(source, loop)`: Global background music or UI sounds.
- `Play3D(source, pos, loop)`: Positional audio for entities.
- `UpdateListener(pos, dir, up)`: Syncs the audio "ears" to the active camera.

---

## See Also
- [Architecture Overview](architecture.md)
- [Scene Format (.axs)](../guides/scene_format.md)
- [Project Structure](../guides/project_structure.md)
