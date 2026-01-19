# ECS System Overview

## Table of Contents
- [Introduction to ECS](#introduction-to-ecs)
- [Entities](#entities)
- [Components](#components)
- [Systems](#systems)
- [Working with ECS](#working-with-ecs)
- [Best Practices](#best-practices)

---

## Introduction to ECS

AXIS Engine uses **Entity-Component-System** (ECS) architecture via the [EnTT](https://github.com/skypjack/entt) library. ECS is a data-oriented design pattern that separates **data** from **logic**:

- **Entities**: Unique IDs representing game objects
- **Components**: Pure data structures attached to entities
- **Systems**: Logic that operates on entities with specific components

### Why ECS?

**Performance Benefits:**
- ✅ Cache-friendly memory layout (components stored contiguously)
- ✅ Fast iteration over entities with specific components
- ✅ Easy to parallelize (systems operate independently)
- ✅ Minimal memory overhead (entities are just IDs)

**Design Benefits:**
- ✅ Composition over inheritance
- ✅ Flexible entity creation (mix and match components)
- ✅ Clear separation of concerns
- ✅ Easy to add new behaviors without modifying existing code

---

## Entities

**Entities** are lightweight identifiers (`entt::entity` = `uint32_t`) that represent game objects. They have **no data or behavior** themselves—all data is stored in Components.

### Creating Entities

```cpp
// In a script or system
Scene& scene = /* get scene */;

// Create a new entity
entt::entity player = scene.createEntity();

// Destroy an entity (and all its components)
scene.destroyEntity(player);
```

### Entity IDs

Entities are just integers, so they're cheap to copy and pass around:
```cpp
entt::entity myEntity = /* ... */;

// Check if entity is valid
if (scene.registry.valid(myEntity)) {
    // Entity exists
}

// Get entity as uint32
uint32_t entityID = static_cast<uint32_t>(myEntity);
```

---

## Components

**Components** are pure data structures with **no logic**. They define what an entity **is** or **has**.

### Available Components

#### Core Components
| Component | Purpose | Key Data |
|-----------|---------|----------|
| `InfoComponent` | Entity metadata | `name`, `tag` |
| `TransformComponent` | Position, rotation, scale | `position`, `rotation`, `scale`, `parent`, `children` |

#### Rendering Components
| Component | Purpose | Key Data |
|-----------|---------|----------|
| `MeshRendererComponent` | 3D model rendering | `model`, `shader`, `castShadow`, `color` |
| `MaterialComponent` | Surface properties | `type`, `shininess`, `specular`, `emission`, `uvScale` |
| `CameraComponent` | Viewport and projection | `isPrimary`, `fov`, `nearPlane`, `farPlane`, `yaw`, `pitch` |
| `SkyboxRenderComponent` | Environment rendering | `skybox`, `shader` |

#### Lighting Components
| Component | Purpose | Key Data |
|-----------|---------|----------|
| `DirectionalLightComponent` | Directional light (sun) | `direction`, `color`, `intensity`, `ambient`, `diffuse` |
| `PointLightComponent` | Point light (bulb) | `color`, `intensity`, `radius`, `attenuation` |
| `SpotLightComponent` | Spotlight | `color`, `intensity`, `cutOff`, `outerCutOff` |

#### Physics Components
| Component | Purpose | Key Data |
|-----------|---------|----------|
| `RigidBodyComponent` | Physics simulation | `body`, `constraint`, `isAttachedToParent` |

#### Scripting & Animation
| Component | Purpose | Key Data |
|-----------|---------|----------|
| `ScriptComponent` | Attach scripts | `instance`, `InstantiateScript`, `DestroyScript` |
| `AnimationComponent` | Skeletal animation | `animator` |

#### Audio & Effects
| Component | Purpose | Key Data |
|-----------|---------|----------|
| `AudioSourceComponent` | 3D spatial audio | `filePath`, `volume`, `loop`, `is3D`, `sound` |
| `ParticleEmitterComponent` | Particle effects | `emitter`, `isActive` |
| `VideoPlayerComponent` | Video playback | `filePath`, `isPlaying`, `isLooping`, `decoder` |

#### UI Components
| Component | Purpose | Key Data |
|-----------|---------|----------|
| `UITransformComponent` | Screen-space position | `position`, `size`, `zIndex` |
| `UIRendererComponent` | UI quad rendering | `model`, `shader`, `color` |
| `UITextComponent` | Text rendering | `text`, `font`, `color`, `scale` |
| `UIInteractiveComponent` | Button interaction | `isHovered`, `isPressed`, `onClick`, `onHoverEnter` |
| `UIAnimationComponent` | UI animations | `hoverColor`, `normalColor`, `speed` |

### Adding/Removing Components

```cpp
// Add a component
auto& transform = scene.registry.emplace<TransformComponent>(entity);
transform.position = glm::vec3(0, 5, 0);

// Get a component (throws if doesn't exist)
auto& transform = scene.registry.get<TransformComponent>(entity);

// Check if entity has component
if (scene.registry.all_of<TransformComponent>(entity)) {
    // Entity has TransformComponent
}

// Remove a component
scene.registry.remove<TransformComponent>(entity);
```

### Component Example: Creating a Lit Cube

```cpp
// Create entity
entt::entity cube = scene.createEntity();

// Add metadata
scene.registry.emplace<InfoComponent>(cube, "MyCube", "Prop");

// Add transform
auto& transform = scene.registry.emplace<TransformComponent>(cube);
transform.position = glm::vec3(0, 2, 0);
transform.scale = glm::vec3(1, 1, 1);

// Add renderer
auto& renderer = scene.registry.emplace<MeshRendererComponent>(cube);
renderer.model = resourceManager.GetModel("cubeModel");
renderer.shader = resourceManager.GetShader("phongLitShadowShader");

// Add material
auto& material = scene.registry.emplace<MaterialComponent>(cube);
material.type = MaterialType::PHONG;
material.shininess = 32.0f;
material.specular = glm::vec3(0.5f);

// Add physics
auto& rb = scene.registry.emplace<RigidBodyComponent>(cube);
// (RigidBody setup via PhysicsLoader)
```

---

## Systems

**Systems** are classes that process entities with specific component combinations. They contain **all the logic** in an ECS architecture.

### System Execution Order

Systems run in a specific order each frame:

```
Fixed Timestep (Physics):
  1. PhysicsSystem::Update()

Variable Timestep (Gameplay):
  2. ScriptableSystem::Update()      // Scripts
  3. AnimationSystem::Update()       // Skeletal animation
  4. VideoSystem::Update()           // Video decoding
  5. UIInteractSystem::Update()      // UI input
  6. AudioSystem::Update()           // 3D audio
  7. ParticleSystem::Update()        // Particles

Render (Once per frame):
  8. RenderSystem::RenderShadows()   // Shadow pass
  9. SkyboxRenderSystem::Render()    // Skybox
 10. RenderSystem::Render()          // 3D models
 11. ParticleSystem::Render()        // Particles
 12. UIRenderSystem::Render()        // UI overlay
```

### Available Systems

| System | Processes | Purpose |
|--------|-----------|---------|
| `PhysicsSystem` | `TransformComponent` + `RigidBodyComponent` | Physics simulation, collision detection |
| `ScriptableSystem` | `ScriptComponent` | Execute script `OnUpdate()` |
| `AnimationSystem` | `AnimationComponent` | Update skeletal animations |
| `RenderSystem` | `TransformComponent` + `MeshRendererComponent` | Render 3D models |
| `SkyboxRenderSystem` | `SkyboxRenderComponent` | Render skybox |
| `AudioSystem` | `TransformComponent` + `AudioSourceComponent` | Play 3D spatial audio |
| `UIRenderSystem` | `UITransformComponent` + `UIRendererComponent` | Render UI elements |
| `UIInteractSystem` | `UITransformComponent` + `UIInteractiveComponent` | Handle UI clicks/hovers |
| `ParticleSystem` | `ParticleEmitterComponent` | Update & render particles |
| `VideoSystem` | `VideoPlayerComponent` | Decode video frames |

### How Systems Work

Systems query the ECS registry for entities with specific components:

```cpp
void RenderSystem::Render(Scene& scene) {
    // Query all entities with Transform + MeshRenderer
    auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
    
    // Iterate over matching entities
    for (auto entity : view) {
        // Get components
        auto& transform = view.get<TransformComponent>(entity);
        auto& renderer = view.get<MeshRendererComponent>(entity);
        
        // Render the entity
        renderer.shader->use();
        renderer.shader->setMat4("model", transform.GetWorldModelMatrix(scene.registry));
        renderer.model->Draw(*renderer.shader);
    }
}
```

---

## Working with ECS

### Pattern 1: Finding Entities by Name

```cpp
// In a script
void OnCreate() override {
    // Find player entity
    auto view = m_Scene->registry.view<InfoComponent>();
    for (auto entity : view) {
        auto& info = view.get<InfoComponent>(entity);
        if (info.name == "Player") {
            m_PlayerEntity = entity;
            break;
        }
    }
}
```

### Pattern 2: Finding Entities by Tag

```cpp
// Find all enemies
std::vector<entt::entity> enemies;
auto view = m_Scene->registry.view<InfoComponent>();
for (auto entity : view) {
    auto& info = view.get<InfoComponent>(entity);
    if (info.tag == "Enemy") {
        enemies.push_back(entity);
    }
}
```

### Pattern 3: Iterating Over Entities with Multiple Components

```cpp
// Find all entities with Transform + RigidBody + Renderer
auto view = scene.registry.view<TransformComponent, RigidBodyComponent, MeshRendererComponent>();

for (auto entity : view) {
    auto [transform, rb, renderer] = view.get(entity);
    
    // Do something with all three components
}
```

### Pattern 4: Creating a Complex Entity

```cpp
void SpawnEnemy(Scene& scene, const glm::vec3& position) {
    entt::entity enemy = scene.createEntity();
    
    // Info
    scene.registry.emplace<InfoComponent>(enemy, "Enemy", "Enemy");
    
    // Transform
    auto& transform = scene.registry.emplace<TransformComponent>(enemy);
    transform.position = position;
    
    // Renderer
    auto& renderer = scene.registry.emplace<MeshRendererComponent>(enemy);
    renderer.model = GetResourceManager().GetModel("enemyModel");
    renderer.shader = GetResourceManager().GetShader("phongLitShadowShader");
    
    // Material
    auto& material = scene.registry.emplace<MaterialComponent>(enemy);
    material.type = MaterialType::PHONG;
    material.specular = glm::vec3(0.8f, 0.2f, 0.2f); // Red specular
    
    // Physics (details omitted)
    // auto& rb = scene.registry.emplace<RigidBodyComponent>(enemy);
    // ... setup rigidbody
    
    // Script
    auto& scriptComp = scene.registry.emplace<ScriptComponent>(enemy);
    scriptComp.Bind<EnemyAI>();
    scriptComp.instance = scriptComp.InstantiateScript();
    scriptComp.instance->Init(enemy, &scene, app);
    scriptComp.instance->OnCreate();
}
```

### Pattern 5: Removing Components Conditionally

```cpp
// Remove physics from all entities tagged "UI"
auto view = scene.registry.view<InfoComponent, RigidBodyComponent>();
for (auto entity : view) {
    auto& info = view.get<InfoComponent>(entity);
    if (info.tag == "UI") {
        scene.registry.remove<RigidBodyComponent>(entity);
    }
}
```

---

## Best Practices

### ✅ DO

1. **Keep Components Pure Data**
   ```cpp
   // ✅ Good - Pure data
   struct TransformComponent {
       glm::vec3 position;
       glm::quat rotation;
       glm::vec3 scale;
   };
   ```

2. **Use Scripts for Entity-Specific Logic**
   ```cpp
   // ✅ Good - Logic in script
   class PlayerController : public Scriptable {
       void OnUpdate(float dt) override {
           // Player-specific logic here
       }
   };
   ```

3. **Cache Component Pointers in Scripts**
   ```cpp
   // ✅ Good - Cache in OnCreate
   void OnCreate() override {
       m_Transform = &GetComponent<TransformComponent>();
   }
   
   void OnUpdate(float dt) override {
       m_Transform->position.x += 1.0f; // Fast access
   }
   ```

4. **Check Component Existence**
   ```cpp
   // ✅ Good - Safe access
   if (scene.registry.all_of<TransformComponent>(entity)) {
       auto& transform = scene.registry.get<TransformComponent>(entity);
   }
   ```

5. **Use Views for Iteration**
   ```cpp
   // ✅ Good - Efficient iteration
   auto view = scene.registry.view<TransformComponent, RigidBodyComponent>();
   for (auto entity : view) {
       // Process entities
   }
   ```

### ❌ DON'T

1. **Don't Add Logic to Components**
   ```cpp
   // ❌ Bad - Logic in component
   struct PlayerComponent {
       void Update(float dt) {  // NO!
           // Logic doesn't belong here
       }
   };
   ```

2. **Don't Call GetComponent Every Frame**
   ```cpp
   // ❌ Bad - Expensive lookup every frame
   void OnUpdate(float dt) override {
       auto& transform = GetComponent<TransformComponent>(); // Slow!
   }
   ```

3. **Don't Assume Components Exist**
   ```cpp
   // ❌ Bad - Will crash if component missing
   auto& transform = scene.registry.get<TransformComponent>(entity);
   ```

4. **Don't Store Entity References Long-Term Without Validation**
   ```cpp
   // ❌ Bad - Entity might be destroyed
   entt::entity m_CachedEntity;
   
   void OnUpdate(float dt) override {
       auto& transform = scene.registry.get<TransformComponent>(m_CachedEntity);
       // Crash if entity was destroyed!
   }
   
   // ✅ Good - Validate before use
   void OnUpdate(float dt) override {
       if (scene.registry.valid(m_CachedEntity)) {
           auto& transform = scene.registry.get<TransformComponent>(m_CachedEntity);
       }
   }
   ```

---

## Summary

AXIS Engine's ECS provides:
- ✅ **High Performance**: Cache-friendly data layout
- ✅ **Flexibility**: Mix-and-match components
- ✅ **Clarity**: Clear separation of data (Components) and logic (Systems)
- ✅ **Extensibility**: Easy to add new Components and Systems

**Remember:**
- **Entities** = IDs
- **Components** = Data
- **Systems** = Logic
- **Scripts** = Entity-specific logic (via ScriptComponent)

---

## See Also
- [Architecture Overview](architecture.md)
- [Component Reference](components_reference.md)
- [Scriptable API](../api/scriptable_api.md)
- [Scene Format](scene_format.md)
