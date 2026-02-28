# Resource Loading Standards

This guide outlines the standards and best practices for loading and managing resources in the AXIS Engine using the `.axs` scene format.

## 1. Scene File (.axs) Standards

### Syntax
- The format is **indentation-based** (similar to YAML).
- Use **2 or 4 spaces** consistently for indentation. **Do not use tabs**.
- Keys are case-sensitive (e.g., `Position` is valid, `position` may be ignored or cause warnings).
- Quotes are optional for single-word strings but **recommended for paths** containing spaces or special characters.

### Structure
1. `axis_scene:` (Root)
2. `  Resources:` (Optional, for pre-loading)
3. `  Config:` (Optional, for engine settings)
4. `  Entities:` (Main world content)

## 2. Resource Naming Conventions

To avoid conflicts and ensure maintainability, follow these naming patterns:

- **Shaders**: `<technique><LightingMode>Shader` (e.g., `phongLitShadowShader`, `unlitShader`).
- **Models**: `<objectName>Model` (e.g., `playerModel`, `building01Model`).
- **Textures**: `<objectName>_<mapType>` (e.g., `player_albedo`, `stone_normal`).
- **Sounds**: `<action>Sound` (e.g., `footstepSound`, `explosion_heavy`).

## 3. Recommended Asset Formats

| Type | Recommended Format | Notes |
| :--- | :--- | :--- |
| **3D Models** | `.fbx`, `.obj`, `.gltf` | FBX (binary) is generally most reliable for animations. |
| **Textures** | `.png`, `.jpg`, `.tga` | Use PNG for transparency, JPG for large albedo maps. |
| **Audio** | `.wav`, `.mp3`, `.ogg` | WAV for short effects, MP3/OGG for music/long ambient. |
| **Videos** | `.mp4` | Ensure H.264 codec for widest compatibility. |

## 4. Pre-loading vs. Lazy Loading

### Pre-loading (Resources Block)
Resources defined in the `Resources:` block are loaded **immediately** when the scene starts.
- **Best for**: Shaders, frequently used UI textures, common physics hulls.

### Lazy Loading (Component Level)
Resources defined directly inside a `Component` (e.g., `AudioSource`, `VideoPlayer`) are loaded when the component is created.
- **Best for**: Unique level assets, background music, cutscene videos.

## 5. Directory Organization

Keep your project clean by following the standard directory structure:
```text
/resources
  /models    - .fbx, .obj
  /textures  - .png, .jpg
  /shaders   - .vs, .fs, .gs
  /audios    - .wav, .mp3
  /fonts     - .ttf
  /videos    - .mp4
/scenes      - .axs files
```
