# C++ Scripting Guide

The game engine utilizes a robust C++ scripting system based on the `Scriptable` class. This guide covers how to create, register, and use scripts, including interaction with Physics and Input systems.

---

## 1. Core Concepts

### Creating a Script
Inherit from `Scriptable` and override lifecycle methods.

```cpp
#include <engine/core/scriptable.h>
#include <engine/engine.h> 

class PlayerController : public Scriptable
{
public:
    virtual void OnCreate() override { /* Called on start */ }
    virtual void OnUpdate(float dt) override { /* Called every frame */ }
    virtual void OnDestroy() override { /* Called on destroy */ }
};
```

### Registering Scripts
To use scripts in `.scene` files, register them in your `.cpp` file:
```cpp
#include "your_script.h"
#include <engine/core/script_registry.h>

REGISTER_SCRIPT(PlayerController)
```

### Using in Scene
```text
NEW_ENTITY Player
SCRIPT PlayerController
```

---

## 2. Entity & Components

### Accessing Components
Use templates to get components attached to the same entity.

```cpp
// Get Component
auto& transform = GetComponent<TransformComponent>();
transform.position.y += 1.0f * dt;

// Check Component
if (HasComponent<RigidBodyComponent>()) {
    auto& rb = GetComponent<RigidBodyComponent>();
}
```

### Accessing Other Entities
You can manipulate other entities if you have their ID.
```cpp
// Example: Get script from another entity
if (m_Scene->registry.all_of<ScriptComponent>(otherEntity)) {
    ScriptComponent& sc = m_Scene->registry.get<ScriptComponent>(otherEntity);
    // Cast to your specific script type if needed
}
```

---

## 3. Input System

The engine provides an Action-based input system. Bind keys to actions in `GameState`, then check actions in scripts.

### 1. Bindings (in GameState)
```cpp
auto& input = m_App->GetInputManager();
input.BindAction("Jump", InputType::Key, GLFW_KEY_SPACE);
input.BindAction("Fire", InputType::MouseButton, GLFW_MOUSE_BUTTON_LEFT);
input.BindAction("MoveX", InputType::Key, GLFW_KEY_D); // Simplified example
```

### 2. Checking Input (in Script)
```cpp
void OnUpdate(float dt) override
{
    // Check if pressed this frame
    if (GetActionDown("Jump")) {
        // Jump logic
    }

    // Check if held down
    if (GetAction("Fire")) {
        // Shooting logic
    }
}
```

---

## 4. Physics System

### Callbacks
Override these virtual methods to handle collisions.

```cpp
virtual void OnCollisionEnter(entt::entity other) override;
virtual void OnCollisionStay(entt::entity other) override;
virtual void OnCollisionExit(entt::entity other) override;

virtual void OnTriggerEnter(entt::entity other) override;
virtual void OnTriggerStay(entt::entity other) override;
virtual void OnTriggerExit(entt::entity other) override;
```

### Example Usage
```cpp
void OnCollisionEnter(entt::entity other) override
{
    // Check what we hit
    if (m_Scene->registry.all_of<InfoComponent>(other)) {
        std::cout << "Hit object!" << std::endl;
    }
}
```

**Concepts:**
- **Collision**: Physical interaction between two solid objects.
- **Trigger**: Interaction where at least one object is a "Trigger" (sensor). Set the `No Contact Response` flag on the body.

---

## 5. Advanced Topics

### Manual Script Binding
For entities created purely in C++ (not from Scene file), you can bind scripts manually:

```cpp
auto entity = m_Scene.registry.create();
auto& scriptComp = m_Scene.registry.emplace<ScriptComponent>(entity);

scriptComp.Bind<MyScript>(); // Type-safe binding
scriptComp.instance = scriptComp.InstantiateScript();
scriptComp.instance->Init(entity, &m_Scene, m_App);
scriptComp.instance->OnCreate();
```

### Accessing Core Systems
The `Scriptable` base class gives you access to the engine core via `m_App`.
```cpp
m_App->GetSoundManager().Play2D("audio/click.wav");
m_App->GetPhysicsWorld()...
```
