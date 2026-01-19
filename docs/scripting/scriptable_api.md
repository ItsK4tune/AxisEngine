# Scriptable API Reference

## Table of Contents
- [Overview](#overview)
- [Scriptable Lifecycle](#scriptable-lifecycle)
- [Creating Scripts](#creating-scripts)
- [Lifecycle Methods](#lifecycle-methods)
- [Component Access](#component-access)
- [Input Handling](#input-handling)
- [Physics Callbacks](#physics-callbacks)
- [Manager Access](#manager-access)
- [Common Script Patterns](#common-script-patterns)

---

## Overview

**Scriptable** is the base class for all gameplay scripts in AXIS Engine. Scripts are C++ classes that inherit from `Scriptable` and are attached to entities via `ScriptComponent`.

### Key Features
- **Native C++ Performance**: Zero scripting runtime overhead
- **Full Engine Access**: Direct access to all components and managers
- **Rich Lifecycle**: OnCreate, OnUpdate, OnDestroy, input callbacks, physics callbacks
- **Component Integration**: Type-safe component retrieval and manipulation
- **Input Binding**: Flexible key/button binding system

---

## Scriptable Lifecycle

```
   Entity Created with ScriptComponent
           ↓
   Init(entity, scene, app) ───────┐ Engine internal
           ↓                        │ Sets entity, scene, app refs
   OnCreate() ─────────────────────┘ User override
           ↓                          Initialize script state
           │
    ┌──────────────────────────────┐
    │    MAIN UPDATE LOOP          │
    │                              │
    │  OnUpdate(dt) ←─────────────┐│ Every frame
    │      ↓                      ││ Gameplay logic
    │  Input Callbacks            ││
    │  Physics Callbacks          ││
    │      ↓                      ││
    └──────┬───────────────────────┘│
           │                        │
           │  (Entity Destroyed)    │
           ↓                        │
   OnDestroy() ────────────────────┘ Cleanup
```

### Lifecycle Flow
1. **Init**: Engine calls `Init(entity, scene, app)` to set internal refs
2. **OnCreate**: Called once after Init - initialize your script state here
3. **OnUpdate**: Called every frame - main gameplay logic
4. **Input/Physics Callbacks**: Called when events occur
5. **OnDestroy**: Called when entity is destroyed - cleanup resources

---

## Creating Scripts

### Basic Script Template

```cpp
#include <script/scriptable.h>

class MyScript : public Scriptable {
public:
    void OnCreate() override {
        // Initialize script state
        // Get components you need
        if (HasComponent<TransformComponent>()) {
            m_Transform = &GetComponent<TransformComponent>();
        }
    }
    
    void OnUpdate(float dt) override {
        // Per-frame logic
    }
    
    void OnDestroy() override {
        // Cleanup (if needed)
    }
    
private:
    TransformComponent* m_Transform = nullptr;
    float m_Speed = 5.0f;
};
```

### Registering Scripts

Scripts must be registered with the `ScriptRegistry` before use:

**In your script header (e.g., `player_controller.h`):**
```cpp
#include <script/scriptable.h>

class PlayerController : public Scriptable {
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;
    
private:
    float m_Speed = 5.0f;
};
```

**In your script registration file (e.g., `register_scripts.cpp`):**
```cpp
#include <script/script_registry.h>
#include "player_controller.h"

void RegisterGameScripts() {
    REGISTER_SCRIPT(PlayerController);
}
```

### Attaching Scripts to Entities

**In .scene file:**
```
NEW_ENTITY Player
TRANSFORM 0 0 0  0 0 0  1 1 1
RENDERER playerModel playerShader
SCRIPT PlayerController
```

**At runtime:**
```cpp
entt::entity player = scene.createEntity();
auto& script = scene.registry.emplace<ScriptComponent>(player);
script.Bind<PlayerController>();
script.instance->Init(player, &scene, app);
script.instance->OnCreate();
```

---

## Lifecycle Methods

### OnCreate
```cpp
virtual void OnCreate() {}
```
Called once after the script is initialized. Use this to:
- Cache component references
- Initialize script state
- Set up initial values

**Example:**
```cpp
void OnCreate() override {
    m_Transform = &GetComponent<TransformComponent>();
    m_RigidBody = &GetComponent<RigidBodyComponent>();
    
    m_Health = 100.0f;
    m_MaxSpeed = 10.0f;
}
```

### OnUpdate
```cpp
virtual void OnUpdate(float dt) {}
```
Called every frame. Main gameplay logic goes here.

**Parameters:**
- `dt` - Delta time (time since last frame, affected by time scale)

**Example:**
```cpp
void OnUpdate(float dt) override {
    // Movement
    glm::vec3 velocity(0.0f);
    if (GetKeyboard().GetKey(GLFW_KEY_W)) velocity.z -= 1.0f;
    if (GetKeyboard().GetKey(GLFW_KEY_S)) velocity.z += 1.0f;
    
    if (glm::length(velocity) > 0.0f) {
        velocity = glm::normalize(velocity) * m_Speed;
        m_RigidBody->SetLinearVelocity(velocity);
    }
}
```

### OnDestroy
```cpp
virtual void OnDestroy() {}
```
Called when the entity is destroyed. Use for cleanup.

**Example:**
```cpp
void OnDestroy() override {
    // Stop any playing sounds
    auto& sound = GetSoundManager();
    sound.StopAllSounds();
    
    // Release any resources
}
```

### OnEnable / OnDisable
```cpp
virtual void OnEnable() {}
virtual void OnDisable() {}
```
Called when script is enabled/disabled via `SetEnabled()`.

**Example:**
```cpp
void SetEnabled(bool enabled);  // Call this to enable/disable script

void OnEnable() override {
    // Resume behavior
}

void OnDisable() override {
    // Pause behavior, stop movement, etc.
}
```

---

## Component Access

### GetComponent
```cpp
template <typename T>
T& GetComponent();
```
Retrieves a component from this script's entity. **Throws if component doesn't exist.**

**Example:**
```cpp
void OnUpdate(float dt) override {
    auto& transform = GetComponent<TransformComponent>();
    transform.position.y += m_JumpSpeed * dt;
}
```

### HasComponent
```cpp
template <typename T>
bool HasComponent();
```
Checks if this entity has a specific component.

**Example:**
```cpp
void OnCreate() override {
    if (HasComponent<RigidBodyComponent>()) {
        m_RigidBody = &GetComponent<RigidBodyComponent>();
    } else {
        // Entity has no physics
    }
}
```

### GetScript
```cpp
template <typename T>
T* GetScript(entt::entity targetEntity);
```
Retrieves a script instance from another entity.

**Example:**
```cpp
void OnUpdate(float dt) override {
    // Find player entity and get its script
    entt::entity playerEntity = /* ... */;
    
    if (auto* playerScript = GetScript<PlayerController>(playerEntity)) {
        // Access player's public methods/data
        float playerHealth = playerScript->GetHealth();
    }
}
```

---

## Input Handling

### Keyboard Input

```cpp
KeyboardManager& GetKeyboard();
```

**Available Methods:**
- `GetKey(int key)` - Returns true while key is held
- `GetKeyDown(int key)` - Returns true on key press (once)
- `GetKeyUp(int key)` - Returns true on key release (once)

**Example:**
```cpp
void OnUpdate(float dt) override {
    auto& keyboard = GetKeyboard();
    
    // Continuous movement while held
    if (keyboard.GetKey(GLFW_KEY_W)) {
        MoveForward(dt);
    }
    
    // Single action on press
    if (keyboard.GetKeyDown(GLFW_KEY_SPACE)) {
        Jump();
    }
}
```

### Mouse Input

```cpp
MouseManager& GetMouse();
```

**Available Methods:**
- `GetButton(int button)` - Returns true while button is held
- `GetButtonDown(int button)` - Returns true on button press (once)
- `GetButtonUp(int button)` - Returns true on button release (once)
- `GetPosition()` - Returns mouse position (screen coords)
- `GetDelta()` - Returns mouse movement delta this frame

**Example:**
```cpp
void OnUpdate(float dt) override {
    auto& mouse = GetMouse();
    
    if (mouse.GetButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
        Shoot();
    }
    
    // Camera rotation
    glm::vec2 delta = mouse.GetDelta();
    RotateCamera(delta.x, delta.y);
}
```

### Mouse Callbacks

```cpp
virtual void OnLeftClick() {}
virtual void OnLeftHold(float duration) {}
virtual void OnLeftRelease(float duration) {}

virtual void OnRightClick() {}
virtual void OnRightHold(float duration) {}
virtual void OnRightRelease(float duration) {}

virtual void OnMiddleClick() {}
virtual void OnMiddleHold(float duration) {}
virtual void OnMiddleRelease(float duration) {}

virtual void OnHoverEnter() {}
virtual void OnHoverStay() {}
virtual void OnHoverExit() {}
```

**Example:**
```cpp
void OnLeftClick() override {
    // Mouse clicked on this entity
    std::cout << "Entity clicked!" << std::endl;
}

void OnHoverEnter() override {
    // Mouse entered entity bounds
    HighlightEntity();
}

void OnHoverExit() override {
    // Mouse left entity bounds
    RemoveHighlight();
}
```

### Key Binding System

```cpp
void BindKey(int key, InputEvent event, std::function<void()> callback);
```

**InputEvent Types:**
- `InputEvent::Pressed` - On key press
- `InputEvent::Held` - While key is held
- `InputEvent::Released` - On key release

**Example:**
```cpp
void OnCreate() override {
    // Bind jump to spacebar
    BindKey(GLFW_KEY_SPACE, InputEvent::Pressed, [this]() {
        Jump();
    });
    
    // Bind sprint while shift is held
    BindKey(GLFW_KEY_LEFT_SHIFT, InputEvent::Held, [this]() {
        m_CurrentSpeed = m_SprintSpeed;
    });
    
    BindKey(GLFW_KEY_LEFT_SHIFT, InputEvent::Released, [this]() {
        m_CurrentSpeed = m_WalkSpeed;
    });
}
```

---

## Physics Callbacks

### Collision Callbacks

```cpp
virtual void OnCollisionEnter(entt::entity other) {}
virtual void OnCollisionStay(entt::entity other) {}
virtual void OnCollisionExit(entt::entity other) {}
```

Called when this entity's RigidBody collides with another.

**Example:**
```cpp
void OnCollisionEnter(entt::entity other) override {
    // Check what we collided with
    if (m_Scene->registry.all_of<InfoComponent>(other)) {
        auto& info = m_Scene->registry.get<InfoComponent>(other);
        
        if (info.tag == "Ground") {
            m_IsGrounded = true;
        } else if (info.tag == "Enemy") {
            TakeDamage(10.0f);
        }
    }
}

void OnCollisionExit(entt::entity other) override {
    if (m_Scene->registry.all_of<InfoComponent>(other)) {
        auto& info = m_Scene->registry.get<InfoComponent>(other);
        
        if (info.tag == "Ground") {
            m_IsGrounded = false;
        }
    }
}
```

### Trigger Callbacks

```cpp
virtual void OnTriggerEnter(entt::entity other) {}
virtual void OnTriggerStay(entt::entity other) {}
virtual void OnTriggerExit(entt::entity other) {}
```

Called when this entity enters a trigger volume (RigidBody with `isTrigger = true`).

**Example:**
```cpp
void OnTriggerEnter(entt::entity other) override {
    if (m_Scene->registry.all_of<InfoComponent>(other)) {
        auto& info = m_Scene->registry.get<InfoComponent>(other);
        
        if (info.tag == "Checkpoint") {
            SaveCheckpoint();
        } else if (info.tag == "Collectible") {
            CollectItem(other);
        }
    }
}
```

---

## Manager Access

### Resource Manager

```cpp
ResourceManager& GetResourceManager();
```

**Example:**
```cpp
void OnCreate() override {
    auto& res = GetResourceManager();
    
    // Load resources dynamically
    res.LoadModel("weaponModel", "models/weapon.fbx", true);
    res.LoadSound("shootSound", "audio/shoot.wav");
}
```

### Sound Manager

```cpp
SoundManager& GetSoundManager();
```

**Example:**
```cpp
void Shoot() {
    auto& sound = GetSoundManager();
    sound.PlaySound("shootSound", m_Transform->position, 1.0f);
}
```

### Scene Manager

```cpp
SceneManager& GetSceneManager();
```

**Example:**
```cpp
void OnTriggerEnter(entt::entity other) override {
    if (/* level complete */) {
        GetSceneManager().ChangeScene("scenes/next_level.scene");
    }
}
```

### Time & Utility

```cpp
void SetTimeScale(float scale);
float GetTimeScale() const;
float GetRealDeltaTime() const;  // Unaffected by time scale
```

**Example:**
```cpp
void OnCreate() override {
    // Slow-motion effect
    SetTimeScale(0.3f);
}

void OnUpdate(float dt) override {
    // dt is affected by time scale
    // GetRealDeltaTime() is not affected
    float realDt = GetRealDeltaTime();
}
```

---

## Common Script Patterns

### Pattern 1: Player Controller

```cpp
class PlayerController : public Scriptable {
public:
    void OnCreate() override {
        m_Transform = &GetComponent<TransformComponent>();
        m_RigidBody = &GetComponent<RigidBodyComponent>();
        m_Camera = &GetComponent<CameraComponent>();
        
        // Bind input
        BindKey(GLFW_KEY_SPACE, InputEvent::Pressed, [this]() { Jump(); });
    }
    
    void OnUpdate(float dt) override {
        HandleMovement(dt);
        HandleCamera(dt);
    }
    
private:
    void HandleMovement(float dt) {
        auto& keyboard = GetKeyboard();
        glm::vec3 velocity(0.0f);
        
        glm::vec3 forward = m_Camera->front;
        forward.y = 0.0f;
        forward = glm::normalize(forward);
        glm::vec3 right = m_Camera->right;
        
        if (keyboard.GetKey(GLFW_KEY_W)) velocity += forward;
        if (keyboard.GetKey(GLFW_KEY_S)) velocity -= forward;
        if (keyboard.GetKey(GLFW_KEY_A)) velocity -= right;
        if (keyboard.GetKey(GLFW_KEY_D)) velocity += right;
        
        if (glm::length(velocity) > 0.0f) {
            velocity = glm::normalize(velocity) * m_Speed;
            m_RigidBody->SetLinearVelocity(velocity);
        }
    }
    
    void HandleCamera(float dt) {
        auto& mouse = GetMouse();
        glm::vec2 delta = mouse.GetDelta();
        
        m_Camera->yaw += delta.x * m_Sensitivity;
        m_Camera->pitch -= delta.y * m_Sensitivity;
        m_Camera->pitch = glm::clamp(m_Camera->pitch, -89.0f, 89.0f);
    }
    
    void Jump() {
        if (m_IsGrounded) {
            glm::vec3 vel = m_RigidBody->body->getLinearVelocity();
            vel.y = m_JumpForce;
            m_RigidBody->SetLinearVelocity(vel);
            m_IsGrounded = false;
        }
    }
    
    void OnCollisionEnter(entt::entity other) override {
        if (m_Scene->registry.all_of<InfoComponent>(other)) {
            auto& info = m_Scene->registry.get<InfoComponent>(other);
            if (info.tag == "Ground") {
                m_IsGrounded = true;
            }
        }
    }
    
    TransformComponent* m_Transform;
    RigidBodyComponent* m_RigidBody;
    CameraComponent* m_Camera;
    
    float m_Speed = 5.0f;
    float m_JumpForce = 8.0f;
    float m_Sensitivity = 0.1f;
    bool m_IsGrounded = false;
};
```

### Pattern 2: Enemy AI

```cpp
class EnemyAI : public Scriptable {
public:
    void OnCreate() override {
        m_Transform = &GetComponent<TransformComponent>();
        m_RigidBody = &GetComponent<RigidBodyComponent>();
        
        FindPlayer();
    }
    
    void OnUpdate(float dt) override {
        if (!m_PlayerEntity || !m_Scene->registry.valid(m_PlayerEntity)) {
            FindPlayer();
            return;
        }
        
        auto& playerTrans = m_Scene->registry.get<TransformComponent>(m_PlayerEntity);
        float distance = glm::distance(m_Transform->position, playerTrans.position);
        
        if (distance < m_DetectionRange) {
            ChasePlayer(playerTrans.position, dt);
        } else {
            Patrol(dt);
        }
    }
    
private:
    void FindPlayer() {
        auto view = m_Scene->registry.view<InfoComponent, TransformComponent>();
        for (auto entity : view) {
            auto& info = view.get<InfoComponent>(entity);
            if (info.tag == "Player") {
                m_PlayerEntity = entity;
                break;
            }
        }
    }
    
    void ChasePlayer(const glm::vec3& playerPos, float dt) {
        glm::vec3 direction = playerPos - m_Transform->position;
        direction.y = 0;
        
        if (glm::length(direction) > 0.0f) {
            direction = glm::normalize(direction) * m_ChaseSpeed;
            m_RigidBody->SetLinearVelocity(direction);
        }
    }
    
    void Patrol(float dt) {
        // Simple back-and-forth patrol
        m_PatrolTime += dt;
        if (m_PatrolTime > 3.0f) {
            m_PatrolDirection *= -1.0f;
            m_PatrolTime = 0.0f;
        }
        
        glm::vec3 velocity(m_PatrolDirection * m_PatrolSpeed, 0, 0);
        m_RigidBody->SetLinearVelocity(velocity);
    }
    
    void OnCollisionEnter(entt::entity other) override {
        if (m_Scene->registry.all_of<InfoComponent>(other)) {
            auto& info = m_Scene->registry.get<InfoComponent>(other);
            if (info.tag == "Player") {
                Attack();
            }
        }
    }
    
    void Attack() {
        // Deal damage to player
        if (auto* playerScript = GetScript<PlayerController>(m_PlayerEntity)) {
            // playerScript->TakeDamage(10.0f);
        }
    }
    
    TransformComponent* m_Transform;
    RigidBodyComponent* m_RigidBody;
    entt::entity m_PlayerEntity = entt::null;
    
    float m_DetectionRange = 15.0f;
    float m_ChaseSpeed = 3.0f;
    float m_PatrolSpeed = 1.0f;
    float m_PatrolTime = 0.0f;
    float m_PatrolDirection = 1.0f;
};
```

### Pattern 3: Interactive Object

```cpp
class InteractableObject : public Scriptable {
public:
    void OnCreate() override {
        m_Transform = &GetComponent<TransformComponent>();
        m_InitialPosition = m_Transform->position;
    }
    
    void OnLeftClick() override {
        Interact();
    }
    
    void OnHoverEnter() override {
        m_IsHovered = true;
        Highlight();
    }
    
    void OnHoverExit() override {
        m_IsHovered = false;
        RemoveHighlight();
    }
    
private:
    void Interact() {
        m_IsActivated = !m_IsActivated;
        
        if (m_IsActivated) {
            // Play sound
            auto& sound = GetSoundManager();
            sound.PlaySound("activate", m_Transform->position, 1.0f);
            
            // Trigger event (e.g., open door, spawn item)
        }
    }
    
    void Highlight() {
        // Change material color or add outline
        if (HasComponent<MaterialComponent>()) {
            auto& mat = GetComponent<MaterialComponent>();
            mat.emission = glm::vec3(0.2f, 0.5f, 1.0f);
        }
    }
    
    void RemoveHighlight() {
        if (HasComponent<MaterialComponent>()) {
            auto& mat = GetComponent<MaterialComponent>();
            mat.emission = glm::vec3(0.0f);
        }
    }
    
    TransformComponent* m_Transform;
    glm::vec3 m_InitialPosition;
    bool m_IsHovered = false;
    bool m_IsActivated = false;
};
```

### Pattern 4: UI Button

```cpp
class UIButton : public Scriptable {
public:
    void OnCreate() override {
        if (HasComponent<UIRendererComponent>()) {
            m_Renderer = &GetComponent<UIRendererComponent>();
            m_NormalColor = m_Renderer->color;
        }
    }
    
    void OnLeftClick() override {
        OnClick();
    }
    
    void OnHoverEnter() override {
        if (m_Renderer) {
            m_Renderer->color = m_HoverColor;
        }
    }
    
    void OnHoverExit() override {
        if (m_Renderer) {
            m_Renderer->color = m_NormalColor;
        }
    }
    
private:
    void OnClick() {
        // Button action
        // e.g., load scene, start game, etc.
        if (m_ButtonAction == "StartGame") {
            GetSceneManager().ChangeScene("scenes/game.scene");
        } else if (m_ButtonAction == "Quit") {
            // Quit game
        }
    }
    
    UIRendererComponent* m_Renderer;
    glm::vec4 m_NormalColor;
    glm::vec4 m_HoverColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
    std::string m_ButtonAction = "StartGame";
};
```

---

## Best Practices

### ✅ DO
- Cache component pointers in `OnCreate()` for performance
- Use `HasComponent()` before `GetComponent()` to avoid crashes
- Keep `OnUpdate()` lightweight - avoid heavy computation
- Use physics callbacks instead of polling for collisions
- Use key bindings for cleaner input handling

### ❌ DON'T
- Don't call `GetComponent()` every frame - cache it
- Don't access components that may not exist
- Don't do heavy file I/O in `OnUpdate()`
- Don't forget to check `m_Scene->registry.valid()` when storing entity references
- Don't access `m_Entity`, `m_Scene`, or `m_App` before `OnCreate()` is called

---

## See Also
- [State API](state_api.md)
- [Component Reference](../guides/components_reference.md)
- [Scripting Basics Guide](../guides/scripting_basics.md)
- [Physics System API](systems/physics_system.md)
- [Input Manager API](managers/input_manager.md)
