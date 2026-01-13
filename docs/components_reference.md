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
