# Scene File Format Reference

The scene file (`.scene`) is a text-based format used to define resources, entities, and components in the game. It is parsed line-by-line by the `SceneManager`.

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
```
21: LOAD_SHADER    modelShader    resources/shaders/model.vs    resources/shaders/model.fs
22: LOAD_MODEL     unityChan      resources/models/unitychan.fbx
23: LOAD_FONT      arial          resources/fonts/arial.ttf  24
25: ```

---

## 2. Configuration
Global engine settings can be configured using the `CONFIG` command.

| Argument 1 | Argument 2 | Argument 3 | Description |
| :--- | :--- | :--- | :--- |
| `SHADOWS` | `1` or `0` | - | Enable (`1`) or disable (`0`) shadow rendering. |
| `CULL_FACE`| `1` or `0` | `BACK`, `FRONT`, `FRONT_AND_BACK` | Enable face culling with specified mode. |
| `DEPTH_TEST`| `1` or `0` | `LESS`, `ALWAYS`, `LEQUAL`, etc. | Enable depth test with specified function. |

### Example
```text
CONFIG SHADOWS 1
CONFIG CULL_FACE 1 BACK
CONFIG DEPTH_TEST 1 LESS
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

```

---

## 3. Components

Components define the behavior and properties of entities.

> For a full list of available components and their syntax, see [Component Reference](components_reference.md).

### Basic Example
```text
TRANSFORM 0 0 0  0 0 0  1 1 1
RENDERER  cube   modelShader
SCRIPT    PlayerController
```
