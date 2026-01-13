# Component Reference

This page lists all components supported in `.scene` files.

## General

### Transform
```text
TRANSFORM <x> <y> <z> <rot_x> <rot_y> <rot_z> <scale_x> <scale_y> <scale_z>
```

### Renderer
```text
RENDERER <model_name> <shader_name>
```

### Script
```text
SCRIPT <Class_Name>
```

### Camera
```text
CAMERA <isPrimary> <fov> <yaw> <pitch> [near] [far]
```
- **near**: Near plane distance (optional, default 0.1).
- **far**: Far plane distance (optional, default 1000.0).

## Physics

### RigidBody
```text
RIGIDBODY <SHAPE> [params] <mass> [TYPE]
```
- **Shapes**: `BOX`, `SPHERE`, `CAPSULE`, `COMPOUND`.
- **Types**: `DYNAMIC` (default), `STATIC`, `KINEMATIC`.

## Lighting

### Directional Light
```text
LIGHT_DIR <dir_x> <dir_y> <dir_z> <r> <g> <b> <intensity>
```

### Point Light
```text
LIGHT_POINT <r> <g> <b> <intensity> <radius> [attenuation...]
```

### Spot Light
```text
LIGHT_SPOT <r> <g> <b> <intensity> <cut> <outer>
```

## Audio

### Audio Source
```text
AUDIO_SOURCE <path> <vol> <loop> <3d> <dist> <awake>
```

## User Interface (UI)

### UI Transform
```text
UI_TRANSFORM <x> <y> <w> <h> <z>
```

### UI Renderer
```text
UI_RENDERER <r> <g> <b> <a> <shader>
```

### UI Text
```text
UI_TEXT "String" <font> <r> <g> <b> <scale>
```

## Particle System

### `LOAD_PARTICLE`
Loads a particle texture into the resource manager.
- **Syntax**: `LOAD_PARTICLE <name> <path>`
- **Example**: `LOAD_PARTICLE fireTex resources/textures/fire.png`

### `PARTICLE_EMITTER`
Creates a particle emitter attached to the entities.
- **Syntax**: `PARTICLE_EMITTER <texture_name> <max_particles> <spawn_rate> <lifetime> <start_size> <end_size> <shape>`
- **Shapes**: `DIRECTIONAL`, `CONE`, `FIGURE8`
- **Example**: `PARTICLE_EMITTER fireTex 1000 200.0 1.5 2.0 0.0 CONE`
