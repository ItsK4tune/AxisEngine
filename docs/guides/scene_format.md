# Scene File Format Reference (.axs)

The Axis Scene file (`.axs`) is a highly readable, indentation-based hierarchy format used to define resources, entities, and components in the AXIS Engine. It is parsed seamlessly by `SceneSerializer`.

---

## 1. Resources Block
Resources can be listed and pre-loaded. The format is a hierarchical `Key: Value` list.

### Example
```yaml
axis_scene:
  Resources:
    Shader:
      Name: modelShader
      VS: resources/shaders/model.vs
      FS: resources/shaders/model.fs
    Model:
      Name: unityChan
      Path: resources/models/unitychan.fbx
      Static: true
```

---

## 2. Configuration
Global engine settings are typically stored in `configuration/settings.json`. However, you can also inject Config loader blocks into scenes as needed (handled automatically by `ConfigLoader` parsing).

---

## 3. Entities
Entities form the core of your game world.

- An entity block starts with its name under `Entities:`.
- `Tag` is a standard string property.
- Subsequent `Component` definitions belong to that entity.

```yaml
  Entities:
    Player:
      Tag: Player
```

---

## 4. Components

Components define the physical representation, rendering, and logic of entities.
They are structured under the `Component: <Type>` node. Indentation defines the component's parameters.

### Basic Example
```yaml
  Entities:
    MyCube:
      Tag: default
      
      Component: Transform
        Position: 0.0 0.0 0.0
        Rotation: 0.0 0.0 0.0
        Scale: 1.0 1.0 1.0
        
      Component: Renderer
        Model: cubeModel
        Shader: defaultShader
        
      Component: Script
        Class: PlayerController
```

### Physics Example
Physics RigidBodies no longer use complex single-line strings.
```yaml
      Component: RigidBody
        Type: CAPSULE
        Radius: 1.0
        Height: 1.8
        Mass: 70.0
        BodyType: DYNAMIC
        Offset: 0.0 0.9 0.0
        AngularFactor: 0 1 0
        Restitution: 0.2
```

> For a full list of available components and their precise scene syntax properties, see the Component Loader implementation.

---

## 5. Component Reference & Defaults

The `.axs` format assigns default values if a property is missing. Keys are case-sensitive. Specifying an unknown key will trigger a console warning.

### Transform
- `Position`: Vector3 (Default: `0 0 0`)
- `Rotation`: Vector3 (Default: `0 0 0`, applied as Euler angles)
- `Scale`: Vector3 (Default: `1 1 1`)

### Camera
- `Primary`: Boolean (Default: `1`)
- `FOV`: Float (Default: `45.0`, Range: `(0, 180)`)
- `Yaw`: Float (Default: `-90.0`)
- `Pitch`: Float (Default: `0.0`, Range: `[-89, 89]`)
- `Near`: Float (Default: `0.1`, Must be > 0 and < Far)
- `Far`: Float (Default: `1000.0`)

### MeshRenderer
- `Model`: String (Required, Name of loaded Model resource)
- `Shader`: String (Required, Name of loaded Shader resource)

### LOD
Level of Detail component swapped dynamically based on distance to the camera (squared). Requires a base `MeshRenderer` component.
- `Models`: String List (Space-separated names of loaded Model resources for LOD1, LOD2, etc.)
- `Distances`: Float List (Space-separated distance thresholds for each LOD level.)

### Lights (LightDir, LightPoint, LightSpot)
All lights share these base properties:
- `Active`: Boolean (Default: `1`)
- `CastShadow`: Boolean (Default: `0`)
- `Color`: Vector3 (Default: `1 1 1`)
- `Intensity`: Float (Default: `1.0`, Must be >= 0)
- `AmbientStr`: Float (Default: `0.2` for Dir, `0.1` for Point/Spot)
- `DiffuseStr`: Float (Default: `0.8` for Dir, `1.0` for Point/Spot)
- `SpecularStr`: Float (Default: `0.5` for Dir, `1.0` for Point/Spot)

**Point Light Exclusive:**
- `Radius`: Float (Default: `10.0`, Must be > 0)
- `Constant`: Float (Default: `1.0`)
- `Linear`: Float (Default: `0.09`)
- `Quadratic`: Float (Default: `0.032`)

**Spot Light Exclusive:**
- `CutOff`: Float (Default: `12.5`, Range: `[0, 90]`)
- `OuterCutOff`: Float (Default: `17.5`, Range: `[0, 90]`, Must be >= CutOff)
- Attenuation parameters identical to Point Light.

### Material
- `Type`: String (`PHONG` or `PBR`, Default: `PHONG`)
- `Emission`: Vector3 (Default: `0 0 0`)

**PHONG Specific:**
- `Shininess`: Float (Default: `32.0`, Must be > 0)
- `Specular`: Vector3 (Default: `0.5 0.5 0.5`)
- `Ambient`: Vector3 (Default: `1 1 1`)

**PBR Specific:**
- `Roughness`: Float (Default: `0.5`, Range: `[0, 1]`)
- `Metallic`: Float (Default: `0.0`, Range: `[0, 1]`)
- `AO`: Float (Default: `1.0`, Range: `[0, 1]`)

### Animator
- `Animation`: String (Required, Name of loaded Animation resource)
- `Speed`: Float (Default: `1.0`, Must be >= 0)
- `StartTime`: Float (Default: `0.0`)
- `Rate`: Float (Default: `30.0`, Must be > 0)

### ParticleEmitter
- `Texture`: String (Required, Name of loaded Texture resource)
- `MaxParticles`: Integer (Default: `100`, Must be > 0)
- `Life`: Float (Default: `1.0`, Must be > 0)

### AudioSource
- `Path`: String (Required, Relative filepath to audio)
- `Volume`: Float (Default: `1.0`, Range: `[0, 1]`)
- `Loop`: Boolean (Default: `0`)
- `Is3D`: Boolean (Default: `0`)
- `MinDistance`: Float (Default: `1.0`, Must be > 0)
- `PlayOnAwake`: Boolean (Default: `1`)

### VideoPlayer
- `Path`: String (Required, Relative filepath to video)
- `Loop`: Boolean (Default: `0`)
- `Speed`: Float (Default: `1.0`, Must be >= 0)
- `PlayOnAwake`: Boolean (Default: `1`)

### Script
- `Class`: String (Required, Name of C++ class registered in `ScriptRegistry`)

### RigidBody
- `Type`: String (`BOX`, `SPHERE`, `CAPSULE`, `COMPOUND`, Default: `BOX`)
- `BodyType`: String (`STATIC`, `DYNAMIC`, `KINEMATIC`, Default: `STATIC`)
- `Mass`: Float (Default: `1.0`, Must be >= 0)
- `Restitution`: Float (Default: `0.0`)
- `AngularFactor`: Vector3 (Optional lock axes)
- `LinearFactor`: Vector3 (Optional lock axes)
- `Offset`: Vector3 (Optional center offset)

*(Shape-specific size parameters (`Size`, `Radius`, `Height`) map to their collision hulls)*

### Occlusion
Hardware-based culling component. Testing objects against the depth buffer using t-1 frame visibility results.
- No direct parameters required in `.axs`, the system generates query IDs automatically.

### UI Components
**UITransform**
- `Position`: Vector2 (Default: `0 0`)
- `Size`: Vector2 (Default: `100 100`, Must be >= 0)
- `ZOrder`: Integer (Default: `0`)

**UIRenderer**
- `Color`: Vector4 (RGBA, Default: `1 1 1 1`)
- `Shader`: String (Required, Name of loaded Shader resource)

**UIText**
- `Text`: String (Supported with quotes)
- `Font`: String (Required, Name of loaded Font resource)
- `Color`: Vector3 (Default: `1 1 1`)
- `Scale`: Float (Default: `1.0`)
