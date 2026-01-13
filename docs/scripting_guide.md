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

Scripts use a self-registration macro, so you don't need to touch the engine core code.

**In your script source file (.cpp):**
```cpp
#include "your_script.h"
#include <engine/core/script_registry.h>

REGISTER_SCRIPT(YourClassName)
```

The engine will automatically register `YourClassName` so it can be used in `.scene` files:
```text
SCRIPT YourClassName
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

## 5. Manual Script Binding (Advanced)

If you are creating entities via C++ code (instead of `.scene` files), you can manually bind scripts using `ScriptComponent::Bind<T>()`. This bypasses the registry lookup and provides type safety.

```cpp
#include <game/scripts/my_script.h>

// ... inside a system or initialization function
auto entity = m_Scene.registry.create();
auto& scriptComp = m_Scene.registry.emplace<ScriptComponent>(entity);

// Bind the script class directly
scriptComp.Bind<MyScript>();

// Initialize manual script
scriptComp.instance = scriptComp.InstantiateScript();
scriptComp.instance->Init(entity, &m_Scene, m_App);
scriptComp.instance->OnCreate();
```

## 6. Physics Callbacks

Override these methods to handle collision events.

```cpp
virtual void OnCollisionEnter(entt::entity other) {}
virtual void OnCollisionStay(entt::entity other) {}
virtual void OnCollisionExit(entt::entity other) {}

virtual void OnTriggerEnter(entt::entity other) {}
virtual void OnTriggerStay(entt::entity other) {}
virtual void OnTriggerExit(entt::entity other) {}
```

**Note:**
- `Other` is the entity ID you collided with.
- Use `GetComponent<InfoComponent>(other)` to identify the object.
- **Triggers**: Occur if one of the bodies has the `No Contact Response` flag (often used for sensors/zones).
