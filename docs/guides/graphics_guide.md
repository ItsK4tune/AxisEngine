# Graphics & Rendering Guide

The AXIS Engine uses a forward rendering pipeline with support for dynamic lighting, shadows, and post-processing.

## 1. Render Options

You can configure global render states via the `RenderSystem`.

```cpp
auto& renderSys = m_App->GetRenderSystem();

// Face Culling (Back-face culling is usually enabled by default)
renderSys.SetFaceCulling(true, GL_BACK);

// Depth Testing
renderSys.SetDepthTest(true, GL_LESS);
```

## 2. Lighting System
The engine supports three types of lights. Limits are currently hardcoded in the shader (e.g., 4 Points, 4 Spots).

- **Directional Light**: Infinite distance (Sun).
- **Point Light**: Omnidirectional (Light bulb).
- **Spot Light**: Cone angle (Flashlight, Street lamp).

> See [Component Reference](components_reference.md) for how to configure these in a Scene.

## 3. Shadows
Shadow mapping is supported for Directional Lights.
- Ensure your `MeshRendererComponent` has `castShadow = true` (default).

## 4. Frustum Culling
The engine automatically culls objects outside the camera's view.
- This relies on the Model's AABB (Axis Aligned Bounding Box).
- Ensure your models are exported with correct scale to avoid culling issues.

## 5. Skybox
To render a skybox:
1. Load it in the `Resources` block.
2. Add a `SkyboxRenderer` component to an entity.

```yaml
axis_scene:
  Resources:
    Skybox:
      Name: mySkybox
      Right: resources/skybox/right.jpg
      # ... (Left, Top, Bottom, Front, Back)
      
  Entities:
    Sky:
      Component: SkyboxRenderer
        Skybox: mySkybox
        Shader: skyboxShader
```

## 6. Particle System
The engine supports GPU-instanced 2D effects.
- **Workflow**:
1. Load Texture in `Resources`.
2. Add `ParticleEmitter` component.
    
```yaml
axis_scene:
  Resources:
    Texture:
      Name: fireTex
      Path: resources/textures/fire.png

  Entities:
    FireFX:
      Component: Transform
        Position: 0 0 0
      Component: ParticleEmitter
        Texture: fireTex
        MaxParticles: 100
        Life: 1.0
```
- Particles are simulated on CPU but rendered via Instancing for performance.
