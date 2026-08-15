# Scene Serialization Guide (`.axs` & `.axsb`)

> [Tiếng Việt](../../vi/guides/scene_format.md) | [Components Reference](components_reference.md) | [Configuration Reference](configuration.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine supports two primary serialization formats: human-readable text `.axs` (a YAML subset) and precompiled binary `.axsb`. `.axs` files allow developers to author scenes, input bindings, data tables, localization strings, and engine configs in plain text, while `.axsb` files optimize level loading speeds for release builds.

---

## 2. How to Use

1. **Authoring `.axs` Files**: Create text files using space-based indentation (tabs are strictly prohibited) and specify one of the five root schema keys (`axis_scene`, `axis_input`, `axis_data`, `axis_localization`, or `axis_config`).
2. **Compiling to `.axsb`**: Execute `axis_compile input.axs output.axsb` via command line.
3. **Loading in C++**: Call `SceneManager::Get().LoadScene("scene.axs")` or `LoadBinaryScene("scene.axsb")`.

---

## 3. Examples

### 1. `axis_scene` Schema Example
```yaml
axis_scene:
    Version: 1.0

Resources:
    Textures:
        - Name: "crate_diffuse"
          Path: "textures/crate_d.png"

Entities:
    - Name: "Sun Light"
      Transform:
          Position: 0.0 10.0 0.0
          Rotation: 45.0 -30.0 0.0
      DirectionalLight:
          Color: 1.0 0.95 0.8
          Intensity: 2.5

    - Name: "Box Entity"
      Transform:
          Position: 0.0 1.0 0.0
      MeshRenderer:
          Model: "models/cube.obj"
          AlbedoTexture: "crate_diffuse"
```

### 2. `axis_input` Schema Example
```yaml
axis_input:
    Bindings:
        MoveForward:
            Key: W
            GamepadAxis: LeftY
        Jump:
            Key: Space
            GamepadButton: South
```

### 3. `axis_data` Schema Example
```yaml
axis_data:
    PlayerStats:
        BaseHealth: 100
        BaseSpeed: 5.5
        MaxInventorySlots: 20
```

### 4. `axis_localization` Schema Example
```yaml
axis_localization:
    Language: "vi_VN"
    Strings:
        UI_PLAY: "CHƠI NGAY"
        UI_QUIT: "THOÁT GAME"
```

---

## 4. API & Configuration Reference

### Comprehensive Reference of the 5 `.axs` Schema Types

| Schema Root Key | Primary Purpose | Key Child Sections | Loader / Parser Subsystem |
| :--- | :--- | :--- | :--- |
| `axis_scene` | Scene hierarchy, entities, resources, and environment lighting | `Resources`, `Environment`, `Entities` | `SceneManager`, `YAMLParser` |
| `axis_input` | Action bindings, key mappings, mouse, and gamepad axes | `Bindings`, `Actions`, `Axes` | `InputSerializer`, `InputManager` |
| `axis_data` | Key-value data tables, weapon stats, balance parameters | Custom data nodes | `DataLoader`, `YAMLParser` |
| `axis_localization` | Multi-language translation dictionaries and UI strings | `Language`, `Strings` | `LocalizationService` |
| `axis_config` | Engine global settings, windowing, graphics, and physics | Graphics, Physics, Audio parameters | `ConfigManager`, `Application` |

### Scene Format Syntax Rules & Compiler Reference

| Property / Rule | Constraint | Description |
| :--- | :--- | :--- |
| Indentation | Spaces Only | Tabs are strictly forbidden and raise line/column syntax errors |
| Key-Value Format | `key: value` | Single line key-value pair |
| Unsupported Syntaxes | Anchors (`&`), Aliases (`*`), Flow (`[...]`), Multiline (`\|`) | General YAML features are omitted for maximum parse speed |
| `axis_compile` CLI | `axis_compile <in.axs> <out.axsb>` | Binary compiler executable utility |
| Binary Rollback | Automatic | Legacy or corrupt binary loads roll back scene modifications safely |
