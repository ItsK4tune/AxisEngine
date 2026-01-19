# Architecture Overview

## Table of Contents
- [Introduction](#introduction)
- [High-Level Architecture](#high-level-architecture)
- [Core Systems](#core-systems)
- [Data Flow](#data-flow)
- [Execution Model](#execution-model)
- [Memory Management](#memory-management)

---

## Introduction

AXIS Engine is built on **data-oriented design** principles with a focus on cache-friendly memory access and modularity. The architecture separates **data** (Components) from **behavior** (Systems), allowing for flexible composition of game objects and optimal performance.

### Design Philosophy
- **Data-Oriented**: Components as pure data, Systems as pure logic
- **Cache-Friendly**: Contiguous memory layout via EnTT
- **Modular**: Systems can be enabled/disabled independently
- **Extensible**: Easy to add new Components and Systems
- **Predictable**: Clear execution order and lifecycle hooks

---

## High-Level Architecture

```
┌──────────────────────────────────────────────────────────┐
│                      APPLICATION                         │
│  ┌────────────────────────────────────────────────────┐  │
│  │             State Machine                          │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐         │  │
│  │  │MenuState │  │GameState │  │PauseState│  ...    │  │
│  │  └──────────┘  └──────────┘  └──────────┘         │  │
│  └────────────────────────────────────────────────────┘  │
│                           │                              │
│                           ▼                              │
│  ┌────────────────────────────────────────────────────┐  │
│  │               SCENE MANAGEMENT                     │  │
│  │  ┌──────────────────────────────────────────────┐  │  │
│  │  │  Scene (ECS Registry)                        │  │  │
│  │  │    - Entities (entt::entity IDs)             │  │  │
│  │  │    - Components (Data)                       │  │  │
│  │  └──────────────────────────────────────────────┘  │  │
│  │                                                    │  │
│  │  SceneManager: Load/Unload/Change                 │  │
│  │  SceneLoader: Parse .scene files                  │  │
│  └────────────────────────────────────────────────────┘  │
│                           │                              │
│                           ▼                              │
│  ┌────────────────────────────────────────────────────┐  │
│  │                  SYSTEMS LAYER                     │  │
│  │                                                    │  │
│  │  RenderSystem      │  PhysicsSystem                │  │
│  │  ScriptableSystem  │  AnimationSystem              │  │
│  │  AudioSystem       │  ParticleSystem               │  │
│  │  UIRenderSystem    │  VideoSystem                  │  │
│  │  UIInteractSystem  │  SkyboxRenderSystem           │  │
│  │                                                    │  │
│  └────────────────────────────────────────────────────┘  │
│                           │                              │
│                           ▼                              │
│  ┌────────────────────────────────────────────────────┐  │
│  │                MANAGERS LAYER                      │  │
│  │                                                    │  │
│  │  ResourceManager  │  SoundManager                  │  │
│  │  InputManager     │  MonitorManager                │  │
│  │  AppHandler       │  PhysicsWorld                  │  │
│  │                                                    │  │
│  └────────────────────────────────────────────────────┘  │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

---

## Core Systems

### 1. Entity-Component-System (ECS)

AXIS Engine uses [EnTT](https://github.com/skypjack/entt), a header-only ECS library optimized for performance.

#### Entities
- **IDs Only**: Entities are lightweight `uint32_t` identifiers
- **No Data**: All entity data is stored in Components
- **Lifetime**: Managed by the Scene's ECS registry

#### Components
Components are **pure data structures** with no logic:

| Component Category | Components |
|-------------------|------------|
| **Core** | `InfoComponent`, `TransformComponent` |
| **Rendering** | `MeshRendererComponent`, `MaterialComponent`, `CameraComponent` |
| **Lighting** | `DirectionalLightComponent`, `PointLightComponent`, `SpotLightComponent` |
| **Physics** | `RigidBodyComponent` |
| **Animation** | `AnimationComponent` |
| **Scripting** | `ScriptComponent` |
| **Audio** | `AudioSourceComponent` |
| **UI** | `UITransformComponent`, `UIRendererComponent`, `UITextComponent`, `UIInteractiveComponent`, `UIAnimationComponent` |
| **Effects** | `ParticleEmitterComponent`, `SkyboxRenderComponent`, `VideoPlayerComponent` |

**Example Component:**
```cpp
struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    
    entt::entity parent = entt::null;
    std::vector<entt::entity> children;
    
    glm::mat4 GetWorldModelMatrix(entt::registry& registry) const;
};
```

#### Systems
Systems process entities with specific component combinations:

| System | Purpose | Operates On |
|--------|---------|-------------|
| **RenderSystem** | Renders 3D models | `TransformComponent` + `MeshRendererComponent` |
| **PhysicsSystem** | Simulates physics | `TransformComponent` + `RigidBodyComponent` |
| **ScriptableSystem** | Executes scripts | `ScriptComponent` |
| **AnimationSystem** | Animates models | `AnimationComponent` |
| **AudioSystem** | Plays spatial audio | `TransformComponent` + `AudioSourceComponent` |
| **UIRenderSystem** | Renders UI | `UITransformComponent` + `UIRendererComponent` |
| **UIInteractSystem** | Handles UI input | `UITransformComponent` + `UIInteractiveComponent` |
| **ParticleSystem** | Updates/renders particles | `ParticleEmitterComponent` |
| **VideoSystem** | Decodes video frames | `VideoPlayerComponent` |
| **SkyboxRenderSystem** | Renders skybox | `SkyboxRenderComponent` |

**Example System Update:**
```cpp
void RenderSystem::Render(Scene& scene) {
    // Query entities with Transform + MeshRenderer
    auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
    
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& renderer = view.get<MeshRendererComponent>(entity);
        
        // Bind shader, upload uniforms, draw model
        renderer.shader->use();
        renderer.shader->setMat4("model", transform.GetWorldModelMatrix(scene.registry));
        renderer.model->Draw(*renderer.shader);
    }
}
```

---

### 2. State Machine

The **State Machine** manages game modes (Menu, Gameplay, Pause, etc.) with clean lifecycle hooks.

#### State Lifecycle
```
        OnEnter()
            ↓
    ┌──── OnUpdate(dt) ←──────┐
    │       ↓                  │
    │   OnFixedUpdate(fixedDt) │ (Loop)
    │       ↓                  │
    └────  OnRender() ─────────┘
            ↓
        OnExit()
```

#### State Methods
```cpp
class State {
public:
    virtual void OnEnter() = 0;       // Load scene, init resources
    virtual void OnUpdate(float dt) = 0;   // Frame logic
    virtual void OnFixedUpdate(float fixedDt) {} // Physics timestep
    virtual void OnRender() = 0;      // Custom rendering
    virtual void OnExit() = 0;        // Cleanup
    
    // Accessors for systems, managers, scene
    SceneManager& GetSceneManager();
    ResourceManager& GetResourceManager();
    KeyboardManager& GetKeyboard();
    // ... and more
};
```

#### Example State
```cpp
class GameState : public State {
    void OnEnter() override {
        LoadScene("scenes/game.scene");
        EnablePhysics(true);
        EnableRender(true);
        SetCursorMode(CursorMode::LockedHiddenCenter);
    }
    
    void OnUpdate(float dt) override {
        // Game loop logic (optional, mostly handled by systems)
    }
    
    void OnRender() override {
        // Custom rendering (optional)
    }
    
    void OnExit() override {
        GetSceneManager().ClearAllScenes();
    }
};
```

---

### 3. Scriptable API

**Scriptable** is the base class for all gameplay scripts. Scripts are attached to entities via `ScriptComponent`.

#### Scriptable Lifecycle
```
        Init(entity, scene, app)
            ↓
        OnCreate()
            ↓
    ┌───── OnUpdate(dt) ←──────┐
    │         ↓                │ (Loop)
    └─── (Input Callbacks) ────┘
            ↓
        OnDestroy()
```

#### Scriptable Methods
```cpp
class Scriptable {
public:
    // Lifecycle
    void Init(entt::entity entity, Scene* scene, Application* app);
    virtual void OnCreate() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnDestroy() {}
    
    // Input Callbacks
    virtual void OnLeftClick() {}
    virtual void OnHoverEnter() {}
    virtual void OnHoverExit() {}
    
    // Physics Callbacks
    virtual void OnCollisionEnter(entt::entity other) {}
    virtual void OnCollisionStay(entt::entity other) {}
    virtual void OnCollisionExit(entt::entity other) {}
    
    // Component Access
    template <typename T>
    T& GetComponent();
    
    template <typename T>
    bool HasComponent();
    
    // Manager Access
    KeyboardManager& GetKeyboard();
    MouseManager& GetMouse();
    ResourceManager& GetResourceManager();
    // ... and more
    
protected:
    entt::entity m_Entity;
    Scene* m_Scene;
    Application* m_App;
};
```

#### Example Script
```cpp
class PlayerController : public Scriptable {
    float m_Speed = 5.0f;
    
    void OnUpdate(float dt) override {
        auto& transform = GetComponent<TransformComponent>();
        auto& rb = GetComponent<RigidBodyComponent>();
        auto& keyboard = GetKeyboard();
        
        glm::vec3 velocity(0.0f);
        if (keyboard.GetKey(GLFW_KEY_W)) velocity.z -= 1.0f;
        if (keyboard.GetKey(GLFW_KEY_S)) velocity.z += 1.0f;
        if (keyboard.GetKey(GLFW_KEY_A)) velocity.x -= 1.0f;
        if (keyboard.GetKey(GLFW_KEY_D)) velocity.x += 1.0f;
        
        if (glm::length(velocity) > 0.0f) {
            velocity = glm::normalize(velocity) * m_Speed;
            rb.SetLinearVelocity(velocity);
        }
    }
    
    void OnCollisionEnter(entt::entity other) override {
        if (m_Scene->registry.all_of<InfoComponent>(other)) {
            auto& info = m_Scene->registry.get<InfoComponent>(other);
            if (info.tag == "Enemy") {
                // Handle enemy collision
            }
        }
    }
};
```

---

## Data Flow

### Scene Loading Flow
```
1. State::LoadScene("path.scene")
       ↓
2. SceneManager::LoadScene(path)
       ↓
3. SceneLoader::Load(path, scene, resources, ...)
       ↓
4. Parse .scene file line by line
   - LOAD_SHADER → ResourceManager::LoadShader()
   - LOAD_MODEL → ResourceManager::LoadModel()
   - NEW_ENTITY → scene.createEntity()
   - TRANSFORM → scene.registry.emplace<TransformComponent>()
   - RENDERER → scene.registry.emplace<MeshRendererComponent>()
   - SCRIPT → ScriptRegistry::Create() + script->OnCreate()
   - etc.
       ↓
5. Scene loaded with all entities, components, and scripts initialized
```

### Frame Update Flow
```
Application::Run() {
    while (running) {
        1. glfwPollEvents()  // Input events
        2. MouseManager::Update()
        
        3. Fixed Timestep Loop:
           PhysicsSystem::Update(fixedDt)
           State::FixedUpdate(fixedDt)
        
        4. Variable Timestep Systems:
           ScriptableSystem::Update(dt)  // Execute all scripts
           AnimationSystem::Update(dt)
           VideoSystem::Update(dt)
           UIInteractSystem::Update(dt, mouse)
           AudioSystem::Update()
           ParticleSystem::Update(dt)
           State::Update(dt)
        
        5. Rendering:
           RenderSystem::RenderShadows()  // Shadow pass
           SkyboxRenderSystem::Render()
           RenderSystem::Render()         // Main pass
           ParticleSystem::Render()
           State::Render()                // Custom rendering
           UIRenderSystem::Render()       // UI overlay
           DebugSystem::Render()          // Debug overlay
        
        6. glfwSwapBuffers()  // Present frame
    }
}
```

---

## Execution Model

### System Execution Order
The order of system updates is critical for correct behavior:

1. **Input Polling** (Once per frame)
2. **Fixed Update** (Multiple times if needed to catch up)
   - `PhysicsSystem` (Always runs at fixed timestep)
   - `State::FixedUpdate`
3. **Variable Update** (Once per frame)
   - `ScriptableSystem` (Gameplay logic)
   - `AnimationSystem` (Skeletal animation)
   - `VideoSystem` (Video decoding)
   - `UIInteractSystem` (UI input)
   - `AudioSystem` (3D audio positioning)
   - `ParticleSystem` (Particle sim)
   - `State::Update` (Custom state logic)
4. **Rendering** (Once per frame)
   - Shadow pass
   - Skybox
   - Opaque geometry
   - Particles
   - UI
   - Debug overlays

### Fixed vs. Variable Timestep

**Fixed Timestep (Physics)**:
- Runs at constant rate (e.g., 60 Hz)
- Accumulator ensures consistent behavior
- Can run multiple times per frame if lagging
```cpp
m_Accumulator += deltaTime;
while (m_Accumulator >= m_FixedDeltaTime) {
    PhysicsSystem::Update(m_FixedDeltaTime);
    m_Accumulator -= m_FixedDeltaTime;
}
```

**Variable Timestep (Rendering/Logic)**:
- Runs once per frame
- Uses actual frame delta time
- Responsive to frame rate changes

---

## Memory Management

### Component Storage
- **Contiguous Arrays**: EnTT stores components in tightly packed arrays
- **Cache-Friendly**: Iterating over components has excellent cache locality
- **Sparse Sets**: O(1) entity-component lookup via sparse sets

### Resource Caching
- **Centralized**: All assets managed by `ResourceManager`
- **Shared Pointers**: Models, shaders, textures use smart pointers
- **Hot-Reload**: File watcher detects changes and reloads assets

### String References
- **Scene Format**: Uses string names for resources (shaders, models, fonts)
- **Runtime**: ResourceManager maps names to pointers
- **No Duplication**: Multiple entities can reference the same resource by name

---

## Summary

AXIS Engine's architecture provides:
- ✅ **Performance**: Data-oriented ECS with cache-friendly memory
- ✅ **Flexibility**: Modular systems can be enabled/disabled
- ✅ **Clarity**: Clear separation of data (Components) and logic (Systems)
- ✅ **Extensibility**: Easy to add new Components, Systems, and Scripts
- ✅ **Productivity**: High-level State and Scriptable APIs for rapid development

For detailed API documentation, see:
- [State API](../api/state_api.md)
- [Scriptable API](../api/scriptable_api.md)
- [Component Reference](components_reference.md)
- [System APIs](../api/systems/)
