# Assets & Resource Management

The **ResourceManager** handles the lifecycle of all game assets, ensuring optimal memory usage through deduplication and caching.

---

## 1. Supported Formats

| Category | Supported Formats | Notes |
| :--- | :--- | :--- |
| **3D Models** | `.fbx`, `.obj`, `.gltf` | FBX is recommended for animated characters. |
| **Animations** | `.fbx`, `.dae` | Requires a target Model with matching rig. |
| **Textures** | `.png`, `.jpg`, `.tga` | PNG for transparency, JPG for high-res albedo. |
| **Audio** | `.wav`, `.mp3`, `.ogg` | WAV for SFX, MP3/OGG for music. |
| **Video** | `.mp4` | Uses FFmpeg for asynchronous decoding. |
| **Shaders** | `.vs`, `.fs` | Glsl-compatible text files. |

---

## 2. Defining Assets in `.axs`
The recommended way to load assets is via the `Resources:` block in your scene file.

```yaml
axis_scene:
  Resources:
    Model:
      Name: playerModel
      Path: models/hero.fbx
      Static: 0
    Texture:
      Name: hero_albedo
      Path: textures/hero_d.png
    Sound:
      Name: footsteps
      Path: audio/steps.wav
```

### Pre-loading vs. Lazy Loading
- **Pre-loading**: Resources in the `Resources:` block load immediately on scene start.
- **Lazy Loading**: Assets defined within components (e.g., a specific `AudioSource` path) load only when the entity is instantiated.

---

## 3. Organization & Conventions

### Directory Structure
```text
/resources
  /models    - .fbx, .obj
  /textures  - .png, .jpg
  /shaders   - .vs, .fs
  /audios    - .wav, .mp3
  /fonts     - .ttf
/scenes      - .axs files
```

### Naming Conventions
- **Models**: `<Name>Model` (e.g., `carModel`)
- **Shaders**: `<Name>Shader` (e.g., `outlineShader`)
- **Textures**: `<ObjectName>_<MapType>` (e.g., `stone_normal`)

---

## 4. Advanced Management

### Asynchronous Loading
Load heavy assets without halting the main thread:
```cpp
res.LoadModelAsync("City", "models/city.fbx");
```

### Hot Reloading
In Debug mode, the engine monitors the `resources/` directory:
- **Shaders**: Automatic recompilation and material updates on file save.
- **Textures**: Refreshed instantly without restarting (supported in dev builds).

### Deduplication
The `ResourceManager` caches assets by their **Name**. Loading the same file path under the same name multiple times will return a shared pointer to the existing instance, saving memory.

---

## See Also
- [Scene Format (.axs)](scene_format.md)
- [Architecture Overview](../core/architecture.md)
- [Managers](../core/managers.md)
