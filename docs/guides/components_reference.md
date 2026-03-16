# Components Reference

This guide provides an exhaustive technical reference for all entity components supported by the AXIS Engine's YAML serialization system (.axs).

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
  Normal: "decals/burn_n.png"
  Opacity: 1.0
  Lifetime: 10.0            # Seconds (-1 for infinite)
  TargetTags: "Ground Wall" # Optimization: Only project on these tags
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
  Volume: 0.8               # 0.0 - 1.0
  Loop: 1
  Is3D: 1                   # Enables spatial attenuation
  MinDistance: 5.0          # Distance at which volume starts dropping
  PlayOnAwake: 1
```

### VideoPlayer
```yaml
Component: VideoPlayer
  Path: "video/intro.mp4"
  Loop: 0
  Speed: 1.0
  PlayOnAwake: 1
```
