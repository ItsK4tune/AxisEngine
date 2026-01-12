# C++ Scripting Guide

The game engine utilizes a robust C++ scripting system based on the `Scriptable` class. Scripts are standard C++ classes that inherit from `Scriptable` and are attached to entities.

## 1. Creating a Script

Inherit from `Scriptable` and override lifecycle methods.

```cpp
#include <engine/core/scriptable.h>
#include <engine/engine.h> // For Application, Input, etc.

class PlayerController : public Scriptable
{
public:
    virtual void OnCreate() override
    {
        // Called when the script is instantiated
        auto& transform = GetComponent<TransformComponent>();
        transform.position = glm::vec3(0, 5, 0);
    }

    virtual void OnUpdate(float dt) override
    {
        // Called every frame
        // 'm_App' gives access to Input and Window
        if (m_App->GetInput()->GetKey(GLFW_KEY_W))
        {
             // Move Logic
        }
    }

    virtual void OnDestroy() override
    {
        // Cleanup
    }
};
```

## 2. Registering and Using Scripts

Scripts must be registered in the `GameState` or `Application` initialization logic so they can be instantiated by name from string (e.g., from `.scene` files).

**In `GameState::Init`:**
```cpp
// Assuming you have a helper or manually adding a component
// Currently, the engine uses magic string mapping in SceneManager if implemented, 
// OR you manually bind them in code:

auto& script = m_Scene->registry.emplace<ScriptComponent>(entity);
script.Bind<PlayerController>();
```

## 3. Accessing Components

Use `GetComponent<T>()` to access components on the same entity.

```cpp
auto& rb = GetComponent<RigidBodyComponent>();
rb.body->setLinearVelocity(btVector3(0, 10, 0));
```

Use `HasComponent<T>()` to check existence.

```cpp
if (HasComponent<CameraComponent>())
{
    // ...
}
```

## 4. Accessing Application & Input

The `Scriptable` base class provides protected members:
- `m_Entity`: Handle to the owning entity.
- `m_Scene`: Pointer to the Scene.
- `m_App`: Pointer to the Application instance.

**Example Input:**
```cpp
if (m_App->GetInput()->GetKey(GLFW_KEY_SPACE))
{
    // Jump
}
```
