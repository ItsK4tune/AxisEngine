# Scene File Format Reference

The scene file (`.scene`) is a text-based format used to define resources, entities, and components in the AXIS Engine. It is parsed line-by-line by the `SceneManager`.

---

## 1. Resources
Resources must be loaded **before** they are used by entities.

### Commands

| Command | Arguments | Description |
| :--- | :--- | :--- |
| `LOAD_SHADER` | `<name> <vertex_path> <fragment_path>` | Load a shader program. |
| `LOAD_MODEL` | `<name> <model_path>` | Load a 3D model (FBX, OBJ, GLB). |
| `LOAD_ANIMATION`| `<name> <model_name> <anim_path>` | Load an animation for a specific model. |
| `LOAD_FONT` | `<name> <font_path> <size>` | Load a TTF font. |
| `LOAD_SKYBOX` | `<name> <right> <left> <top> <bot> <front> <back>` | Load a cube map skybox. |

### Example
```text
LOAD_SHADER    modelShader    resources/shaders/model.vs    resources/shaders/model.fs
LOAD_MODEL     unityChan      resources/models/unitychan.fbx
LOAD_FONT      arial          resources/fonts/arial.ttf  24
```

---

## 2. Configuration
Global engine settings can be configured using the `CONFIG` command.
> **Note**: For initial startup configuration (Title, resolution preference, icon), use `configuration/settings.json`. The scene config below overrides specific runtime settings.

| Argument 1 | Argument 2 | Argument 3 | Description |
| :--- | :--- | :--- | :--- |
| `SHADOWS` | `0`, `1`, or `2` | - | Shadow mode: `0`=None, `1`=Once (single), `2`=All (up to 4). |
| `SHADOW_SIZE` | `<float>` | - | Orthographic projection size for directional shadows (default: 100.0). |
| `INSTANCING` | `1` or `0` | - | Enable/disable instance batching for static meshes. |
| `CULL_FACE`| `1` or `0` | `BACK`, `FRONT`, `FRONT_AND_BACK` | Enable face culling with mode. |
| `DEPTH_TEST`| `1` or `0` | `LESS`, `ALWAYS`, `LEQUAL`, etc. | Enable depth test with function. |
| `WINDOW` | `<w> <h>` | `[mode] [monitor] [hz]` | Resolution, mode (`WINDOWED`/`FULLSCREEN`/`BORDERLESS`), monitor, refresh rate. |
| `VSYNC` | `1` or `0` | - | Enable/disable Vertical Sync. |
| `FPS` | `<int>` | - | Frame rate limit (0 = unlimited). |
| `FRUSTUM` | `1` or `0` | - | Enable/disable camera frustum culling. |
| `DISTANCE` | `<float>` | - | Max render distance from camera (0 = unlimited). |
| `SHADOW_FRUSTUM` | `1` or `0` | - | Enable/disable light frustum culling for shadows. |
| `SHADOW_DISTANCE` | `<float>` | - | Max distance for shadow casting. |

### Example
```text
CONFIG SHADOWS 1
CONFIG SHADOW_SIZE 100.0
CONFIG INSTANCING 1
CONFIG CULL_FACE 1 BACK
CONFIG DEPTH_TEST 1 LESS
CONFIG WINDOW 1920 1080 BORDERLESS 0
CONFIG FRUSTUM 1
CONFIG DISTANCE 500.0
```

---

## 3. Entities
Entities are the objects in your game world.

- Start a new entity with `NEW_ENTITY`.
- Components defined after `NEW_ENTITY` belong to that entity.
- The order of components does not matter.

```text
NEW_ENTITY <entity_name> <entity_tag>
```

---

## 4. Components

Components define the behavior and properties of entities.

> For a full list of available components and their scene syntax, see [Component Reference](components_reference.md).

### Basic Example
```text
TRANSFORM 0 0 0  0 0 0  1 1 1
RENDERER  cube   modelShader
SCRIPT    PlayerController
```
