# Tham chiếu component

> [English](../../eng/guides/components_reference.md)

Tài liệu này mô tả component được YAML `.axs` hỗ trợ. State chỉ tồn tại runtime
như world/camera matrix suy ra, GPU/FBO/query handle, loading state, UI hover,
navigation request/path, playback và physics contact không được serialize.
Runtime system dựng lại chúng sau load.

## Component lõi

### Transform

```yaml
Component: Transform
  Position: 0.0 5.0 -10.0
  Rotation: 0.0 90.0 0.0
  Scale: 1.0 1.0 1.0
```

### Info

```yaml
Tag: "player"
Layer: 1
Parent: "MainNode"
```

## Rendering

### Renderer

```yaml
Component: Renderer
  Model: "cube_mesh"
  Shader: "pbr_shader"
  Order: 0
  Color: 1 1 1 1
  CastShadow: 1
```

### Material

```yaml
Component: Material
  Type: "PBR"
  Albedo: "tex/albedo.png"
  Normal: "tex/norm.png"
  Roughness: 0.5
  Metallic: 0.0
  AO: 1.0
  Emission: 0 0 0
  AlphaCutoff: 0.5
  BlendSrc: "SrcAlpha"
  BlendDst: "OneMinusSrcAlpha"
```

### Decal

```yaml
Component: Decal
  Albedo: "decals/burn.png"
  Opacity: 1.0
  Roughness: 1.0
  Metallic: 0.0
  Reflectivity: 0.0
  TintColor: 1 1 1 1
  Lifetime: 10.0
  RenderOrder: 1
  LightingMode: 2
  TargetTags: "Ground Wall"
  Shader: "custom_decal"
```

### SkyboxRenderer

```yaml
Component: SkyboxRenderer
  Skybox: "sunset_sky"
  Shader: "skybox_shader"
```

## Camera và lighting

```yaml
Component: Camera
  Primary: true
  FOV: 60.0
  Near: 0.1
  Far: 1000.0
  AspectRatio: 1.777
  Yaw: -90.0
  Pitch: 0.0
```

```yaml
Component: LightDir
  Active: 1
  Color: 1 1 0.9
  Intensity: 1.5
  CastShadow: 1
  Ambient: 0.2
  Diffuse: 0.8
  Specular: 0.5
```

```yaml
Component: LightPoint
  Radius: 15.0
  Color: 1 0 0
  Intensity: 2.0
  Constant: 1.0
  Linear: 0.09
  Quadratic: 0.032
```

## Animation, particle và physics

```yaml
Component: Animator
  Animation: "run jump"
  Speed: 1.0
  StartTime: 0.0
  Rate: 30.0
```

Animation Graph lưu parameter float/bool/trigger, state, transition, blend và
exit time. Script điều khiển qua `SetAnimationFloat`, `SetAnimationBool`,
`SetAnimationTrigger`; khi graph tắt vẫn dùng `PlayAnimation`, `CrossFade`,
`PlayBlend`.

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

VFX Graph quyết định module nào nối được tới Output; link có thể có điều kiện
float/bool/trigger. Node position và link được lưu trong `.axs`/`.axsb`.

```yaml
Component: RigidBody
  Type: "BOX"
  Mass: 1.0
  Restitution: 0.5
  Friction: 0.5
  AngularFactor: 1 1 1
  Gravity: 0 -9.81 0
  LinearDamping: 0.1
```

## Navigation

```yaml
Component: PathFollower
  MoveSpeed: 5.0
  RotationSpeed: 10.0
  ArrivalDistance: 0.5
  RotationOffset: 0 -90 0
```

## UI

```yaml
Component: UITransform
  anchorMin: 0.0 1.0
  anchorMax: 0.0 1.0
  offsetMin: 20 -100
  offsetMax: 200 -20
  pivot: 0.5 0.5
  zIndex: 10
```

```yaml
Component: UIText
  text: "Level 1"
  font: "Arial"
  color: 1 1 1 1
  alignment: "Center"
  wordWrap: true
  maxWidth: 200.0
```

## Multimedia

```yaml
Component: AudioSource
  Path: "audio/music.mp3"
  Volume: 80.0
  Loop: 1
  Is3d: true
  MinDistance: 5.0
  PlayOnAwake: 1
```

```yaml
Component: VideoPlayer
  Path: "video/intro.mp4"
  Loop: 0
  Speed: 1.0
  Volume: 1.0
  PlayOnAwake: 1
```
