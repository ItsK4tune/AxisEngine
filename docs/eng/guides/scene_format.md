# Scene File Format Reference (.axs)

> [Tiếng Việt](../../vi/guides/scene_format.md)

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
      Vertex: assets/shaders/model.vs
      Fragment: assets/shaders/model.fs
    Model:
      Name: unityChan
      Path: assets/models/unitychan.fbx
      Static: true
```

---

## 2. Configuration
Global engine settings are defined directly in the `.axs` file under the `Config:` block. The `ConfigLoader` seamlessly injects these into `Application` before loading the `Entities` block.

```yaml
axis_scene:
  Config:
    WINDOW_WIDTH: 1920
    WINDOW_HEIGHT: 1080
    WINDOW_MODE: BORDERLESS_FULLSCREEN
    VSYNC: 1
    ANTIALIASING: TAA
    MSAA: 4
    HDR_ENABLED: 1
    BLOOM_ENABLED: 1
    TONEMAPPING: ACES
    SHADOW_RESOLUTION: 2048
    JOB_THREADS: -1
    LOG_LEVEL: VERBOSE
    PHYSICS_MODE: BALANCED
```

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
      Layer: 1
```

---

## 4. Components

Components define the physical representation, rendering, and logic of entities.
They are structured under the `Component: <Type>` node. Indentation defines the component's parameters.

### Basic Example
```yaml
axis_scene:
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

> For a detailed list of all components and their exact YAML properties, refer to the:
> ### 📘 **[Components Reference Guide](components_reference.md)**

---

## 5. Scene Best Practices

1.  **Unique Names**: Entity names should be unique within their scene context to avoid lookup ambiguity.
2.  **Modular Configs**: Keep runtime configuration in `axis_config`; scene files should describe resources and entities.
3.  **Layer Filtering**: Use `Layer` bitmasks to isolate entities for specific camera views or physics queries.

---

## 6. Component Reference Summary

The `.axs` format assigns default values if a property is missing. Keys are case-sensitive. Specifying an unknown key will trigger a console warning.

### Basic Components
For properties like `Position`, `Rotation`, and `Scale`, see the **[Architecture Overview](../core/architecture.md)**.

### Rendering & Visuals
Refer to the **[Graphics Guide](graphics.md)** for:
- `MeshRenderer`: Model, Shader, Render Order.
- `LOD`: Level-of-detail model swapping.
- `Lights`: Directional, Point, and Spot configuration.
- `Material`: PBR vs Phong settings, texture overrides, and transparency.
- `SkyboxRenderer`: Environment cubemaps.

### Physics & Movement
Refer to the **[Physics Guide](physics.md)** for:
- `RigidBody`: Dynamic, Static, and Kinematic bodies.
- `CharacterController`: Player-specific movement and gravity.
- `PhysicsMode`: Fast, Balanced, or Accurate simulation settings.

### AI & Navigation
Refer to the **[Navigation Guide](navigation.md)** for:
- `NavMesh`: Baking walkable surfaces.
- `PathFollower`: Pathfinding and steering parameters.

### User Interface
Refer to the **[UI Guide](ui.md)** for:
- `UITransform`: 2D positioning, anchoring, and scaling.
- `UIRenderer` & `UIText`: Visual UI elements and dynamic labels.

### Scripts & Logic
Entities can run custom C++ logic via the `Script` component:
- `Class`: The name of the C++ class (e.g., `PlayerController`).
- See **[Scriptable API](../scripting/scriptable_api.md)** for more.

---

## 6. Resources & Tags
- **Tags**: Used for filtering and identification (e.g., `Walkable`, `Player`).
- **Layers**: Bitmask (1-32) for selective rendering visibility.
- **Resources**: Pre-loading assets to avoid runtime hitches. See **[Assets Guide](assets.md)**.
