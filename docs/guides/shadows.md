# Shadow System Guide

The engine supports dynamic shadow casting for **Directional**, **Point**, and **Spot** lights.

## Quick Start

To enable shadows:
1. Ensure `CONFIG SHADOWS 2` (or 1) is set in your `.scene` file.
2. Add `castShadow` flag = `1` to your light entities in `.scene`.

## Limitations & Constraints

Due to hardware Texture Unit limits (max 16 usually), there is a strict limit on the number of **Simultaneous Shadow Casting Lights**:

| Light Type | Max Shadow Casters | Texture Units |
| :--- | :---: | :---: |
| **Directional** | **2** | 10, 11 |
| **Point** | **2** | 12, 13 |
| **Spot** | **2** | 14, 15 |
| **TOTAL** | **6** | 10 - 15 |

> [!WARNING]
> If you enable `castShadow` for more than 2 lights of the same type, **only the first 2** will cast shadows. The others will be ignored (no crash, just missing shadows).

## Configuration

### Scene File (`game.scene`)

```
CONFIG SHADOWS 2           // 0=Off, 1=First Dir Only, 2=All Supported
CONFIG SHADOW_SIZE 100.0   // Ortho size for Directional Shadows
CONFIG SHADOW_DISTANCE 100.0 // Max distance to render shadows
```

### Component Setup

#### Directional Light
Direction (0, -1, 0) is rotated by Transform.
```
NEW_ENTITY Sun
TRANSFORM 0 10 0 -45 -30 0 1 1 1
LIGHT_DIR 1 1 1.0 1.0 1.0 1.0
//        ^ ^-- Cast Shadow (0=No, 1=Yes)
//        ^---- Active
```

#### Spot Light
Cone direction follows Transform rotation.
```
NEW_ENTITY SpotLamp
TRANSFORM 0 5 0 -90 0 0 1 1 1
LIGHT_SPOT 1 1 1.0 1.0 1.0 2.0 25.0 30.0
//         ^ ^-- Cast Shadow
```

#### Point Light
Omnidirectional shadow (most expensive).
```
NEW_ENTITY Bulb
TRANSFORM 0 2 0 0 0 0 1 1 1
LIGHT_POINT 1 1 1.0 1.0 0.0 2.0 10.0
//          ^ ^-- Cast Shadow
```

## Troubleshooting

### Black Screen / Missing Shadows
1. **Check Limits**: Are you using >2 shadow lights of one type?
2. **Check Intensity**: Is the light too dim?
3. **Check FarPlane**:
   - Spot/Point shadows use a fixed FarPlane (default 100.0).
   - If objects are further than 100 units from the light, they won't cast shadows.
4. **Scene Rotation**:
   - Default Spot Light rotation (0,0,0) points **Right (+X)** or **Forward (-Z)** depending on implementation.
   - Recommended: Rotate Pitch -90 to point **Down**.

