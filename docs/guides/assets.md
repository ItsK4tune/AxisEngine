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

### Strict Asset Loading
Debug builds can keep using internal error shaders, checkerboard textures, and model fallbacks to make missing assets visible. Set `STRICT_ASSET_LOADING: 1` in config for production-style behavior where failed shader, texture, and model loads report failure instead of silently substituting fallback assets.

### Hot Reloading
In Debug mode, the engine monitors registered shader and texture files:
- **Shaders**: Vertex, fragment, and geometry shader stages trigger recompilation on file save.
- **Textures**: Registered file-backed textures are uploaded again without restarting.
- **Scripts and metadata**: Runtime reload is not supported by the current compile-time script registry.

### Deduplication
The `ResourceManager` caches assets by their **Name**. Loading the same file path under the same name multiple times will return a shared pointer to the existing instance, saving memory.

---

## See Also
- [Scene Format (.axs)](scene_format.md)
- [Architecture Overview](../core/architecture.md)
- [Managers](../core/managers.md)
