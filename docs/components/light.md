# Light Components

This document describes how to define light sources in the `.scene` file.

> [!IMPORTANT]
> Shadow Casting is limited to **2 lights per type** (2 Dir, 2 Point, 2 Spot) due to texture unit constraints. See [Shadow Guide](../guides/shadows.md) for details.

## Directional Light (LIGHT_DIR)

Directional lights simulate distant light sources like the sun. The direction is determined by the entity's **Transform rotation**.

**Syntax:**
```
LIGHT_DIR <Active> <CastShadow> <R> <G> <B> <Intensity> [Ambient] [Diffuse]
```

- **Active** (0/1): value `1` enables the light.
- **CastShadow** (0/1): value `1` enables shadow casting.
- **R, G, B**: Color (0.0 - 1.0).
- **Intensity**: Brightness multiplier.
- **Ambient** (Optional, default 0.2): Ambient strength.
- **Diffuse** (Optional, default 0.8): Diffuse strength.

**Example:**
```yaml
  Entities:
    Sun:
      Component: Transform
        Position: 0.0 10.0 0.0
        Rotation: 45.0 0.0 0.0
      Component: LightDir
        Color: 1.0 0.95 0.8
        Intensity: 1.0
        AmbientStr: 0.2
        DiffuseStr: 0.8
        CastShadow: 1
```
*(Note: Use `TRANSFORM` rotation (Euler angles X Y Z) to set direction. Default direction is (0, -1, 0))*

---

## Point Light (LIGHT_POINT)

Point lights emit light in all directions from their position.

**Syntax:**
```
LIGHT_POINT <Active> <CastShadow> <R> <G> <B> <Intensity> <Radius> [Constant Linear Quadratic] [Ambient Diffuse]
```

- **Active**, **CastShadow**: (0/1).
- **R, G, B**: Color.
- **Intensity**: Brightness.
- **Radius**: Approximate range.
- **Attenuation** (Optional): `Constant` (default 1.0), `Linear` (default 0.09), `Quadratic` (default 0.032).
- **Ambient**, **Diffuse** (Optional): Strengths (default 0.1, 1.0).

**Example:**
```yaml
  Entities:
    Lamp:
      Component: Transform
        Position: 2.0 5.0 2.0
      Component: LightPoint
        Color: 1.0 0.5 0.0
        Intensity: 2.0
        Radius: 50.0
        Constant: 1.0
        Linear: 0.09
        Quadratic: 0.032
        AmbientStr: 0.1
        DiffuseStr: 1.0
        CastShadow: 1
```

---

## Spot Light (LIGHT_SPOT)

Spot lights emit a cone of light. Direction and position come from Transform.

**Syntax:**
```
LIGHT_SPOT <Active> <CastShadow> <R> <G> <B> <Intensity> <CutOff> <OuterCutOff> [Constant Linear Quadratic] [Ambient Diffuse]
```

- **CutOff**: Inner cone angle (in degrees).
- **OuterCutOff**: Outer cone angle (in degrees, for soft edges).
- Other parameters same as Point Light.

**Example:**
```yaml
  Entities:
    Flashlight:
      Component: Transform
        Position: 0.0 2.0 0.0
        Rotation: 90.0 0.0 0.0
      Component: LightSpot
        Color: 1.0 1.0 1.0
        Intensity: 3.0
        CutOff: 25.0
        OuterCutOff: 30.0
        CastShadow: 1
```
