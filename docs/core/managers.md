# Engine Managers & Services

Managers are centralized services instantiated by the `Application`. They handle lifecycle, resource state, and OS-level interactions.

---

## 1. ResourceManager
**Include:** `<engine/core/resource_manager.h>`  
**Access:** `m_App->GetResourceManager()`

Handles loading, caching, and retrieval of all engine assets.

### Features
- **Deduplication**: Automatically caches textures, shaders, and models by name or path.
- **Asynchronous Loading**: Supports non-blocking asset loading via the Job System.
- **Hot Reloading**: Automatically recompiles shaders and refreshes textures when source files change.

### Common Methods
- `GetModel(name)`, `GetTexture(name)`, `GetShader(name)`
- `LoadModel(name, path)`, `LoadTexture(name, path)`

---

## 2. SceneManager
**Include:** `<engine/core/scene_manager.h>`  
**Access:** `m_App->GetSceneManager()`

Manages the active entity registries and scene transitions.

### Common Methods
- `LoadScene(path)`: Adds entities from an `.axs` file to the current world.
- `ChangeScene(path)`: Clears all active entities and loads a fresh scene.
- `ClearAllScenes()`: Resets the entire world state.

---

## 3. InputManager & Handlers
**Include:** `<engine/core/app_handler.h>`  
**Access:** `m_App->GetAppHandler()`

Distributes OS-level keyboard and mouse events.

### Keyboard & Mouse
- **Keyboard**: `GetKeyboard().GetKey(GLFW_KEY_W)` for state polling.
- **Mouse**: `GetMouse().GetPosition()` for screen-space coordinates.
- **Cursor Modes**: Supports `Normal`, `Hidden`, `Locked`, and `LockedHidden`.

### Input Actions
Recommended for gameplay: Use `GetAction("Jump")` to poll mapped keys defined in configuration.

---

## 4. SoundManager
**Include:** `<engine/core/sound_manager.h>`  
**Access:** `m_App->GetSoundManager()`

Wraps the irrKlang engine for spatial audio.

### Methods
- `Play2D(source, loop)`: Global background music or UI sounds.
- `Play3D(source, pos, loop)`: Positional audio for entities.
- `UpdateListener(pos, dir, up)`: Syncs the audio "ears" to the active camera.

---

## See Also
- [Architecture Overview](architecture.md)
- [Scene Format (.axs)](../guides/scene_format.md)
- [Project Structure](../guides/project_structure.md)
