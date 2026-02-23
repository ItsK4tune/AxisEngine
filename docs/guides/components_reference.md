# Component Reference

This page lists all components supported in `.axs` Axis Scene files for the AXIS Engine.

## General

### Transform
```yaml
      Component: Transform
        Position: 0.0 0.0 0.0
        Rotation: 0.0 0.0 0.0
        Scale: 1.0 1.0 1.0
```

### Renderer
```yaml
      Component: Renderer
        Model: unityChan
        Shader: defaultShader
```

### Script
```yaml
      Component: Script
        Class: PlayerController
```

### Camera
```yaml
      Component: Camera
        Primary: 1
        FOV: 90.0
        Yaw: -90.0
        Pitch: 0.0
        Near: 0.1
        Far: 1000.0
```

## Physics

### RigidBody
Adds a physics body to the entity.

**Syntax**:
```yaml
      Component: RigidBody
        Type: BOX          # Options: BOX, SPHERE, CAPSULE, COMPOUND
        Mass: 1.0          # >0.0 is DYNAMIC, 0.0 is STATIC
        Size: 1.0 1.0 1.0  # Used for BOX
        Radius: 0.5        # Used for SPHERE, CAPSULE
        Height: 1.0        # Used for CAPSULE
        Offset: 0.0 0.5 0.0
        Restitution: 0.8
        AngularFactor: 0 1 0
        LinearFactor: 1 1 1
        BodyType: DYNAMIC  # Options: STATIC, DYNAMIC, KINEMATIC
        AttachToParent: false
```

## Material

### Phong (Legacy/Cartoonish)
```yaml
      Component: Material
        Type: PHONG
        Shininess: 32.0
        Specular: 0.5 0.5 0.5
        Emission: 0.0 0.0 0.0
        Ambient: 1.0 1.0 1.0
```

### PBR (Realistic)
```yaml
      Component: Material
        Type: PBR
        Roughness: 0.5
        Metallic: 0.0
        AO: 1.0
        Emission: 0.0 0.0 0.0
```

## Lighting

All light components support color and intensity.

### Directional Light
```yaml
      Component: LightDir
        Color: 1.0 0.95 0.8
        Intensity: 1.0
        AmbientStr: 0.2
        DiffuseStr: 0.8
        SpecularStr: 0.5
```
- **Direction**: Determined by the entity's `Transform` rotation.

### Point Light
```yaml
      Component: LightPoint
        Color: 1.0 0.0 0.0
        Intensity: 1.0
        Radius: 10.0
        Constant: 1.0
        Linear: 0.09
        Quadratic: 0.032
        AmbientStr: 0.1
        DiffuseStr: 1.0
        SpecularStr: 1.0
```

### Spot Light
```yaml
      Component: LightSpot
        Color: 1.0 1.0 1.0
        Intensity: 1.0
        CutOff: 12.5
        OuterCutOff: 17.5
        Constant: 1.0
        Linear: 0.09
        Quadratic: 0.032
        AmbientStr: 0.1
        DiffuseStr: 1.0
```

## Audio

### Audio Source
```yaml
      Component: AudioSource
        Path: resources/audio/jump.wav
        Volume: 1.0
        Loop: false
        Is3D: true
        MinDistance: 1.0
        PlayOnAwake: true
```

## User Interface (UI)

### UI Transform
```yaml
      Component: UITransform
        Position: 0 0
        Size: 100 100
        ZOrder: 0
```

### UI Renderer
```yaml
      Component: UIRenderer
        Color: 1.0 1.0 1.0 1.0
        Shader: uiShader
```

### UI Text
```yaml
      Component: UIText
        Text: "Hello World"
        Font: arial
        Color: 1.0 1.0 1.0
        Scale: 1.0
```

## Particle System

### Particle Emitter
```yaml
      Component: ParticleEmitter
        Texture: fireTex
        MaxParticles: 1000
        Life: 1.5
```

## Video

### Video Player
Plays an MP4 video. Note: Requires FFmpeg libraries.

```yaml
      Component: VideoPlayer
        Path: assets/videos/intro.mp4
        Loop: true
        Speed: 1.0
        PlayOnAwake: true
```
