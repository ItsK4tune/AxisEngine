# Graphics & Rendering Guide
![AXIS Engine Logo](../assets/logo.png)

**Engine**: AXIS Engine  
**Contributor**: Duong "Caftun" Nguyen

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
1. Load it in the Scene (`LOAD_SKYBOX`).
2. Add a `SKYBOX_RENDERER` component to an entity.

```text
NEW_ENTITY Sky
SKYBOX_RENDERER <skybox_name> <shader_name>
```

## 6. Particle System
The engine supports GPU-instanced 2D effects.
- **Workflow**:
    1. Load Texture: `LOAD_PARTICLE <name> <path>`.
    2. Create Entity: Add `PARTICLE_EMITTER`.
    
```text
NEW_ENTITY FireFX
PARTICLE_EMITTER <texture_name> <max_particles> <lifetime>
TRANSFORM ... (Controls Emitter Position)
```
- Particles are simulated on CPU but rendered via Instancing for performance.
