# Components Reference

This guide provides an exhaustive technical reference for all entity components supported by the AXIS Engine's YAML serialization system (.axs).

Runtime-only state is intentionally not serialized. This includes derived camera/world matrices, GPU/FBO/query handles, streaming/loading state, UI hover/press state, current navigation paths and request generations, playback state, and physics contact/controller state. Authoring fields and stable resource/entity references are serialized; runtime systems reconstruct transient fields after load.

---

## 🏗️ Core Components

### Transform
Implicitly handled by `SceneSerializer`. Position, Rotation, and Scale are the core spatial properties.
```yaml
Component: Transform
  Position: 0.0 5.0 -10.0   # X Y Z
  Rotation: 0.0 90.0 0.0    # Pitch Yaw Roll (Degrees)
  Scale: 1.0 1.0 1.0        # X Y Z
```

### Info
Implicitly handled.
```yaml
Tag: "player"               # Entity string tag
Layer: 1                    # Rendering/Physics layer bitmask
Parent: "MainNode"          # Parent entity name
```

---

## 🎨 Rendering Components

### Renderer
Standard mesh rendering.
```yaml
Component: Renderer
  Model: "cube_mesh"        # Registered model name or path
  Shader: "pbr_shader"      # Registered shader name
  Order: 0                  # Bucket render order
  Color: 1 1 1 1            # RGBA Multiplier
  CastShadow: 1             # 1 (On) or 0 (Off)
```

### Material
Advanced material overrides. Supports PBR and PHONG models.
```yaml
Component: Material
  Type: "PBR"               # PBR or PHONG
  Albedo: "tex/albedo.png"  # Texture path
  Normal: "tex/norm.png"    
  Roughness: 0.5            # Float (0.0 - 1.0)
  Metallic: 0.0             
  AO: 1.0                   
  Emission: 0 0 0           # RGB multiplier
  AlphaCutoff: 0.5          # For masked transparency
  BlendSrc: "SrcAlpha"      # Zero, One, SrcAlpha, OneMinusSrcAlpha
  BlendDst: "OneMinusSrcAlpha"
```

### Decal
Projected textures for surface details.
```yaml
Component: Decal
  Albedo: "decals/burn.png"
  Opacity: 1.0
  Roughness: 1.0
  Metallic: 0.0
  Reflectivity: 0.0
  TintColor: 1 1 1 1
  Lifetime: 10.0            # Seconds (-1 for infinite)
  RenderOrder: 1
  LightingMode: 2           # 0=unlit, 1=lit, 2=lit and receives shadows
  TargetTags: "Ground Wall" # Optimization: Only project on these tags
  Shader: "custom_decal"    # Optional custom shader using the deferred decal contract
```

### SkyboxRenderer
```yaml
Component: SkyboxRenderer
  Skybox: "sunset_sky"      # Registered skybox name
  Shader: "skybox_shader"
```

---

## 🎥 Camera & Lighting

### Camera
```yaml
Component: Camera
  Primary: true             # Auto-activates on scene load
  FOV: 60.0                 # Vertical field of view
  Near: 0.1                 
  Far: 1000.0
  AspectRatio: 1.777        # Width/Height (0 for auto)
  Yaw: -90.0
  Pitch: 0.0
```

### LightDir (Directional)
```yaml
Component: LightDir
  Active: 1
  Color: 1 1 0.9            # RGB
  Intensity: 1.5
  CastShadow: 1
  Ambient: 0.2
  Diffuse: 0.8
  Specular: 0.5
```

### LightPoint
```yaml
Component: LightPoint
  Radius: 15.0              # Effective range
  Color: 1 0 0              
  Intensity: 2.0
  Constant: 1.0             # Falloff parameters
  Linear: 0.09
  Quadratic: 0.032
```

---

## 🏃 Animation & Physics

### Animator
```yaml
Component: Animator
  Animation: "run jump"     # Space-separated animation names
  Speed: 1.0                # Global speed multiplier
  StartTime: 0.0            # Start offset in seconds
  Rate: 30.0                # Internal update sampling rate
```

The optional animation graph is authored from **Tools > Animation Graph**. It stores named float, bool, and trigger
parameters, clip states, conditional transitions, blend durations, and normalized exit times in the same Animator
component. Gameplay scripts drive the graph through `Entity::SetAnimationFloat`, `SetAnimationBool`, and
`SetAnimationTrigger`. Direct `PlayAnimation`, `CrossFade`, and `PlayBlend` calls remain available when the graph is
disabled.

The panel can be toggled with `Ctrl+Shift+1`. Left-dragging empty canvas space or middle-dragging pans the graph, the mouse wheel zooms around the cursor,
and the divider between canvas and inspector is draggable. Transition conditions support AND, OR, XOR, NAND, NOR,
XNOR, and per-condition NOT.

Graph data is serialized with repeated `GraphParameter`, `GraphState`, and `GraphTransitionV2` records. These records
are editor-owned; prefer editing them through the graph panel instead of hand-authoring their compact representation.

### ParticleEmitter and VFX Graph

The particle emitter supports spawn rate, finite or infinite emission duration, lifetime, directional/cone/figure-eight
emission, velocity ranges, gravity, exponential drag, color over life, size over life, textures, and custom shaders.
Open **Tools > VFX Graph** (or press `Ctrl+Shift+2`) to connect emitter modules to an Output node. Once a graph contains
links, only modules that can reach an enabled Output node through passing links affect the emitter. VFX links can use
float, bool, and trigger parameters with the same grouped logical conditions as animation transitions. Left-dragging empty canvas space or middle-dragging
pans, the mouse wheel zooms, and the canvas/inspector divider is draggable.

```yaml
Component: ParticleEmitter
  Active: true
  SpawnRate: 60
  Lifetime: 1.5
  Gravity: 0 -9.81 0
  Drag: 0.15
  Shape: CONE
  GraphEnabled: false
```

The inspector exposes the common emitter settings. The VFX Graph panel owns advanced module authoring and persists
node positions and links into `.axs` and `.axsb` scenes.

### RigidBody
Modular physics simulation.
```yaml
Component: RigidBody
  Type: "BOX"               # BOX, SPHERE, CAPSULE, CYLINDER, MESH
  Mass: 1.0                 # 0.0 for Static
  Restitution: 0.5          # Bounciness
  Friction: 0.5
  AngularFactor: 1 1 1      # Axis rotation locks (e.g., 0 1 0 locks X/Z)
  Gravity: 0 -9.81 0
  LinearDamping: 0.1
```

---

## 📐 Navigation

### PathFollower
```yaml
Component: PathFollower
  MoveSpeed: 5.0
  RotationSpeed: 10.0       # Slerp speed for orientation
  ArrivalDistance: 0.5      # Target waypoint tolerance
  RotationOffset: 0 -90 0   # Adjustment for model forward vector
```

---

## 📟 UI System (Responsive)

### UITransform
```yaml
Component: UITransform
  anchorMin: 0.0 1.0        # Normalized (0-1) screen anchor
  anchorMax: 0.0 1.0
  offsetMin: 20 -100        # Pixel offset from anchor
  offsetMax: 200 -20
  pivot: 0.5 0.5            # Rotation/Scaling center
  zIndex: 10                # Layering depth
```

### UIText
```yaml
Component: UIText
  text: "Level 1"
  font: "Arial"
  color: 1 1 1 1
  alignment: "Center"       # Left, Center, Right
  wordWrap: true
  maxWidth: 200.0           # Wrap width in pixels
```

---

## 🔊 Multimedia

### AudioSource
```yaml
Component: AudioSource
  Path: "audio/music.mp3"
  Volume: 80.0              # Playback percentage, 0.0 - 100.0
  Loop: 1
  Is3d: true                # Enables spatial attenuation
  MinDistance: 5.0          # Full-volume radius; attenuation starts outside it
  PlayOnAwake: 1
```

### VideoPlayer
```yaml
Component: VideoPlayer
  Path: "video/intro.mp4"
  Loop: 0
  Speed: 1.0
  Volume: 1.0              # Embedded audio volume, 0.0 - 1.0
  PlayOnAwake: 1
```
