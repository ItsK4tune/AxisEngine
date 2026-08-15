# Serializable ECS Components & EntityBuilder Reference Guide

> [Tiếng Việt](../../vi/guides/components_reference.md) | [Scene Format](scene_format.md) | [Scriptable API](../scripting/scriptable_api.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine uses EnTT for its Entity-Component-System (ECS) runtime. In scene files (`.axs` / `.axsb`) and code construction, entities can be populated with serializable components registered in the engine's `ComponentLoader` and `SceneSerializer`.

To attach and configure these serializable components programmatically in C++, AxisEngine provides the `EntityBuilder` fluent API helper.

---

## 2. How to Use

1. **Adding Components in `.axs` Scene Files**: Specify the component serializer key under an entity (for example, `Transform:`, `Renderer:`, `RigidBody:`).
2. **Adding Components via `EntityBuilder`**: Instantiate `EntityBuilder(scene, resources, "Name")`, call builder setup methods (`WithPosition()`, `WithPBRMesh()`, `WithRigidBody()`), and call `Build()`.
3. **Adding Components via C++ API**: Use `entity.AddComponent<T>()` or query active components using `entity.HasComponent<T>()`.

---

## 3. Examples

### 1. `.axs` Scene Component Definition Example
```yaml
axis_scene:
    Version: 1.0

Entities:
    - Name: "Physics Box"
      Transform:
          Position: 0.0 2.0 0.0
          Rotation: 0.0 45.0 0.0
      Renderer:
          Model: "models/cube.obj"
          Shader: "shaders/pbr.glsl"
          CastShadow: 1
      RigidBody:
          Mass: 2.5
          Shape: BOX
```

### 2. Fluent `EntityBuilder` C++ Creation Example
```cpp
#include <axis_sdk.h>

void SpawnBoxEntity(Scene& scene, ResourceManager& resources, const Vector3& pos) {
    EntityBuilder builder(scene, resources, "Physics Box");

    builder.WithPosition(pos)
           .WithRotationEuler(Vector3(0.0f, 45.0f, 0.0f))
           .WithPBRRenderable("models/cube.obj", "shaders/pbr.glsl", pos)
           .WithRigidBody(2.5f /* mass */, false /* isStatic */, false /* isTrigger */)
           .WithTag("Interactive");

    Entity box = builder.Build();
    AXIS_LOG_INFO("Entity created via EntityBuilder successfully");
}
```

---

## 4. API & Configuration Reference

### Complete Matrix of Addable / Serializable Components

| Serializer Key | C++ Component Struct | Primary `.axs` Parameters | Category | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| `Transform` | `TransformComponent` | `Position`, `Rotation`, `Scale` | Spatial | Local 3D transform position, rotation, and scale |
| `Renderer` | `MeshRendererComponent` | `Model`, `Shader`, `Color`, `CastShadow`, `ReceiveShadow`, `Order` | Graphics | 3D mesh model geometry renderer |
| `Material` | `MaterialComponent` | `Metallic`, `Roughness`, `AO`, `Albedo`, `Normal`, `Emission` | Graphics | PBR material textures and surface parameters |
| `Camera` | `CameraComponent` | `Primary`, `FOV`, `Near`, `Far`, `AspectRatio`, `Orthographic` | Rendering | Perspective or orthographic camera projection |
| `DirectionalLight` | `DirectionalLightComponent` | `Active`, `Direction`, `Color`, `Intensity`, `CastShadow` | Lighting | Sun light directional illumination source |
| `PointLight` | `PointLightComponent` | `Active`, `Color`, `Intensity`, `Radius`, `CastShadow` | Lighting | Omnidirectional point light source |
| `SpotLight` | `SpotLightComponent` | `Active`, `Direction`, `Color`, `Intensity`, `CutOff`, `Radius` | Lighting | Focused cone spot light source |
| `LightProbe` | `LightProbeComponent` | `Radius`, `Intensity` | Lighting | Spherical harmonics ambient light probe |
| `ReflectionProbe` | `ReflectionProbeComponent` | `Type`, `Resolution`, `BoxProjection` | Lighting | Environment reflection cubemap probe |
| `Reflective` | `ReflectiveComponent` | `Active`, `Reflectivity`, `FresnelPower`, `FresnelBias` | Lighting | Specular Fresnel reflection behavior |
| `PlanarReflection` | `PlanarReflectionComponent` | `Resolution`, `Normal` | Lighting | Flat mirror planar reflection pass |
| `SkyboxRenderer` | `SkyboxRenderComponent` | `Skybox`, `Shader`, `Primary` | Graphics | Environment skybox cubemap renderer |
| `LOD` | `LODComponent` | `Models`, `Distances` | Graphics | Distance-based mesh LOD switcher |
| `Occlusion` | `OcclusionComponent` | `Active` | Graphics | Occlusion culling visibility state |
| `Streaming` | `StreamingComponent` | `ModelPath`, `LoadDistance`, `UnloadDistance` | Graphics | Distance-based asset streaming |
| `Decal` | `DecalComponent` | `AlbedoMap`, `Opacity`, `Roughness`, `Metallic`, `TintColor` | Environment | Projected surface decal texture |
| `Terrain` | `TerrainComponent` | `TerrainSize`, `MaxHeight`, `HeightMap`, `SplatMap` | Environment | Heightmap terrain renderer and physics collider |
| `PostProcess` | `PostProcessComponent` | `Active`, `Effects` | Environment | Fullscreen post-processing pipeline pass list |
| `RigidBody` | `RigidBodyComponent` | `Mass`, `IsStatic`, `IsTrigger`, `Friction`, `Restitution` | Physics | Bullet 3D rigid body dynamics |
| `RigidShape` | `RigidShapeComponent` | `ShapeType`, `Size`, `Radius`, `Height` | Physics | Collision shape primitive definition |
| `CharacterController` | `CharacterControllerComponent` | `StepHeight`, `MaxSlope`, `Radius`, `Height` | Physics | Kinematic walking character controller |
| `AudioSource` | `AudioSourceComponent` | `File`, `PlayOnAwake`, `Loop`, `Is3D`, `Volume`, `Pitch` | Audio | 2D/3D audio clip sound source |
| `VideoPlayer` | `VideoPlayerComponent` | `VideoPath`, `Loop`, `Volume`, `Speed`, `PlayOnAwake` | Media | Video decoding and texture playback |
| `ParticleEmitter` | `ParticleEmitterComponent` | `SpawnRate`, `LifeTime`, `StartSize`, `EndSize`, `MinVelocity` | Media & VFX | Particle emitter system settings |
| `Animation` | `AnimationComponent` | `Animation`, `Speed`, `Rate`, `BlendFactor` | Media & VFX | Skeletal animation playback state |
| `UITransform` | `UITransformComponent` | `Position`, `Size`, `AnchorMin`, `AnchorMax`, `Pivot`, `ZOrder` | UI Layout | 2D UI screen-space transform bounds |
| `UIRenderer` | `UIRendererComponent` | `Texture`, `Color` | UI Visual | 2D UI sprite image/panel renderer |
| `UIText` | `UITextComponent` | `Text`, `Font`, `Scale`, `Color`, `Alignment` | UI Visual | TrueType font label text renderer |
| `UIFlex` | `UIFlexLayoutComponent` | `Direction`, `Spacing` | UI Layout | Flexbox-style automatic 2D layout |
| `UIInteractive` | `UIInteractiveComponent` | `Interactable` | UI Logic | Mouse interaction and click callback target |
| `UIAnimation` | `UIAnimationComponent` | `AnimateColor`, `AnimateScale` | UI Logic | UI button animation transition effect |
| `PathFollower` | `PathFollowerComponent` | `MoveSpeed`, `RotationSpeed` | Navigation | Spline path movement follower |
| `NavMesh` | `NavMeshComponent` | `Dynamic`, `WalkableNormalY` | Navigation | Recast NavMesh navigation mesh settings |
| `NavigationGrid` | `NavigationGridComponent` | `Width`, `Height`, `CellSize` | Navigation | 2D pathfinding grid map |
| `Fragment` | `FragmentComponent` | `Path`, `Overrides` | Prefab | Reusable scene fragment prefab link |
| `Network` | `NetworkComponent` | `NetworkID`, `OwnerID`, `IsLocal` | Networking | Network synchronization identifier |
| `Script` | `ScriptComponent` | `Class`, `Properties` | Scripting | C++ `Scriptable` script attachment |

### `EntityBuilder` API Reference

| Category | Method | Description |
| :--- | :--- | :--- |
| **Resources** | `WithTextureResource(name, path, async)` | Registers texture asset with ResourceManager |
| **Resources** | `WithModelResource(name, path, isStatic)` | Registers 3D model mesh asset |
| **Resources** | `WithShaderResource(name, vert, frag, geom)` | Registers GLSL shader pipeline program |
| **Resources** | `WithFontResource(name, path, fontSize)` | Registers TrueType font asset |
| **Resources** | `WithSkyboxResource(name, faces)` | Registers cubemap 6-face texture set |
| **Core Meta** | `WithName(name)` | Sets entity debug name string |
| **Core Meta** | `WithTag(tag)` | Sets entity classification tag string |
| **Core Meta** | `WithLayer(layer)` | Sets bitwise collision/render layer mask |
| **Core Meta** | `WithActive(active)` | Sets entity active status flag |
| **Core Meta** | `WithParent(parentEntity)` | Establishes parent-child hierarchy link |
| **Transform** | `WithPosition(pos)` | Sets 3D position vector |
| **Transform** | `WithRotationEuler(rotDegrees)` | Sets 3D Euler rotation angles in degrees |
| **Transform** | `WithScale(scale)` | Sets 3D scale vector or uniform scale |
| **Rendering** | `WithMesh(modelName, shaderName)` | Attaches mesh renderer component |
| **Rendering** | `WithPBRMaterial(metallic, roughness, ao)` | Configures PBR surface material properties |
| **Rendering** | `WithPBRRenderable(model, shader, pos, rot, scale, metal, rough)` | Sets transform, mesh, and PBR material in one call |
| **Rendering** | `WithLOD(models, distances)` | Configures LOD mesh distance thresholds |
| **Physics** | `WithRigidBody(mass, isStatic, isTrigger)` | Attaches Bullet 3D rigid body physics component |
| **Physics** | `WithCharacterController(controller, stepHeight, maxSlope, radius, height)` | Attaches kinematic character controller |
| **Lighting** | `WithDirectionalLight(dir, color, intensity)` | Attaches directional sun light source |
| **Lighting** | `WithPointLight(color, intensity, radius)` | Attaches omnidirectional point light source |
| **Lighting** | `WithSpotLight(dir, color, intensity, radius)` | Attaches focused spot light source |
| **UI Layout** | `WithUITransform(pos, size, zIndex)` | Sets 2D UI position, dimensions, and render order |
| **UI Layout** | `WithUIAnchored(anchor, pos, size)` | Configures UI anchor placement |
| **UI Visual** | `WithUIText(text, fontName, scale, color)` | Attaches TrueType text rendering label |
| **UI Visual** | `WithUITexture(textureName, color)` | Attaches UI sprite texture renderer |
| **Scripting** | `WithScript(scriptName)` | Attaches registered script by factory name |
| **Scripting** | `WithScriptable(className, instantiateFunc)` | Attaches script via lambda instantiation factory |
| **Audio** | `WithAudioSource(filePath, playOnAwake, loop, is3D, volume)` | Attaches 2D/3D audio clip source component |
| **Build** | `Build()` | Finalizes configuration and returns EnTT `Entity` handle |
