# Scene File Format Reference

The scene file (`.scene`) is a text-based format used to define resources, entities, and components in the game. It is parsed line-by-line by the `SceneManager`.

## 1. Resources
Resources must be loaded **before** they are used by entities.

| Command | Arguments | Description |
| :--- | :--- | :--- |
| `LOAD_SHADER` | `<name> <vertex_path> <fragment_path>` | Load a shader program. |
| `LOAD_MODEL` | `<name> <model_path>` | Load a 3D model (FBX, OBJ, GLB). |
| `LOAD_ANIMATION`| `<name> <model_name> <anim_path>` | Load an animation for a specific model. |
| `LOAD_FONT` | `<name> <font_path> <size>` | Load a TTF font. |
| `LOAD_SKYBOX` | `<name> <right> <left> <top> <bot> <front> <back>` | Load a cube map skybox. |

**Example:**
```text
LOAD_SHADER    modelShader    resources/shaders/model.vs    resources/shaders/model.fs
LOAD_MODEL     unityChan      resources/models/unitychan.fbx
LOAD_FONT      arial          resources/fonts/arial.ttf  24
```

---

## 2. Entities
Entities are the objects in your game world.

- Start a new entity with `NEW_ENTITY`.
- Components defined after `NEW_ENTITY` belong to that entity.
- The order of components does not matter.

```text
NEW_ENTITY <entity_name> <entity_tag>
```

---

## 3. General Components

### Transform
Position, rotation (quaternion or euler depends on implementation, usually euler in loader), and scale.
```text
TRANSFORM <pos_x> <pos_y> <pos_z> <rot_x> <rot_y> <rot_z> <scale_x> <scale_y> <scale_z>
```

### Renderer (Model)
Renders a 3D model using a specific shader.
```text
RENDERER <model_name> <shader_name>
# Alias:
MODEL    <model_name> <shader_name>
```

### Animator
Plays an animation.
```text
ANIMATOR <animation_name> <speed> <start_time> <frame_rate>
```

### Camera
Configures the entity as a camera.
```text
CAMERA <is_primary: 1|0> <fov> <yaw> <pitch>
```

### Script
Attaches a C++ script (must be registered in `GameState`).
```text
SCRIPT <script_class_name>
```

---

## 4. Physics Components

### RigidBody
Defines physical properties.

**Syntax:**
```text
RIGIDBODY <SHAPE_TYPE> [shape_params...] <mass> [BODY_TYPE]
```

**Body Types:**
- `DYNAMIC`: Affected by gravity and forces. Default if mass > 0.
- `STATIC`: Immovable. Default if mass = 0.
- `KINEMATIC`: Moved by code/animation, affects dynamic bodies but not affected by them.

**Shapes:**

| Type | Parameters | Example |
| :--- | :--- | :--- |
| `CAPSULE` | `<radius> <height>` | `RIGIDBODY CAPSULE 0.5 2.0 1.0 DYNAMIC` |
| `BOX` | `<size_x> <size_y> <size_z>` | `RIGIDBODY BOX 1.0 1.0 1.0 0.0 STATIC` |
| `COMPOUND` | N/A (Uses sub-shapes) | `RIGIDBODY COMPOUND 1.0` |

**Compound Shape Example:**
```text
RIGIDBODY COMPOUND 10.0
    SHAPE BOX 0.0 0.0 0.0  0.0 0.0 0.0  1.0 1.0 1.0
    SHAPE BOX 0.0 2.0 0.0  0.0 0.0 0.0  0.5 0.5 0.5
END_RIGIDBODY
```

---

## 5. Lighting Components

### Directional Light (Sun)
```text
LIGHT_DIR <dir_x> <dir_y> <dir_z> <r> <g> <b> <intensity>
```

### Point Light (Bulb)
```text
LIGHT_POINT <r> <g> <b> <intensity> <radius> [constant] [linear] [quadratic]
```

### Spot Light (Flashlight)
```text
LIGHT_SPOT <r> <g> <b> <intensity> <cut_off> <outer_cut> [constant] [linear] [quadratic]
```
*Note: Angles are in degrees.*

---

## 6. Audio Components

### Audio Source
Plays a 3D or 2D sound.
```text
AUDIO_SOURCE <path> <volume> <loop: 1|0> <is3D: 1|0> <min_dist> [playOnAwake: 1|0]
```
**Example:**
```text
AUDIO_SOURCE resources/audio/music.mp3 0.5 1 0 10.0 1
AUDIO_SOURCE resources/audio/footstep.wav 1.0 0 1 5.0 0
```

---

## 7. UI Components

### UI Transform
```text
UI_TRANSFORM <x> <y> <width> <height> <z_index>
```

### UI Renderer
```text
UI_RENDERER <r> <g> <b> <a> <shader_name>
```

### UI Text
```text
UI_TEXT "Content String" <font_name> <r> <g> <b> <scale>
```
