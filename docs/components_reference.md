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

## Material
## Material
**Phong (Legacy/Cartoonish)**
```text
MATERIAL PHONG <shininess> <spec_r> <spec_g> <spec_b>
```
- **shininess**: Specular exponent.
- **spec_rgb**: Specular Color Color.

**PBR (Realistic)**
```text
MATERIAL PBR <roughness> <metallic> <ao>
```
- **roughness**: Surface irregularity (0.0 = Smooth, 1.0 = Rough).
- **metallic**: Metalness (0.0 = Dielectric/Plastic, 1.0 = Metal).
- **ao**: Ambient Occlusion factor (0.0 - 1.0).

## Lighting

### Directional Light
```text
LIGHT_DIR <dir_x> <dir_y> <dir_z> <r> <g> <b> <intensity> [ambient] [diffuse]
```
- **intensity**: Light intensity multiplier.
- **ambient**: Ambient strength multiplier (optional, default 0.2).
- **diffuse**: Diffuse strength multiplier (optional, default 0.8).

### Point Light
```text
LIGHT_POINT <r> <g> <b> <intensity> <radius> [constant] [linear] [quadratic] [ambient] [diffuse]
```
- **radius**: Approximate range of the light.
- **attenuation**: optional constant, linear, quadratic falloff.
- **ambient/diffuse**: optional multipliers (defaults: 0.1, 1.0).

### Spot Light
```text
LIGHT_SPOT <r> <g> <b> <intensity> <cut> <outer> [constant] [linear] [quadratic] [ambient] [diffuse]
```
- **cut/outer**: Cutoff angles in degrees.
- **attenuation**: optional constant, linear, quadratic falloff.
- **ambient/diffuse**: optional multipliers (defaults: 0.1, 1.0).

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
