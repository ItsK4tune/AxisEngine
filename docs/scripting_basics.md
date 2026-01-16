# Scripting Basics

Scripting in the AXIS Engine is done using Native C++ classes. Scripts are components derived from the `Scriptable` class.

## 1. Creating a Script

Inherit from `Scriptable` and implement the lifecycle methods. You must also include the `script_registry.h` header and use the `REGISTER_SCRIPT` macro if you want the script to be instantiable by name (e.g., from scene definitions).

```cpp
#include <engine/core/scriptable.h>
#include <engine/core/script_registry.h>

class MyScript : public Scriptable
{
public:
    void OnCreate() override
    {
        // Called once when the script is instantiated
    }

    void OnUpdate(float dt) override
    {
        // Called every frame if the script is enabled
    }

    void OnDestroy() override
    {
        // Called when the script or entity is destroyed
    }
};

// Register the class name to the factory
REGISTER_SCRIPT(MyScript)
```

## 2. Script Lifecycle

The `Scriptable` class defines several virtual methods that track the lifecycle of the script:

*   **`OnCreate()`**: Called once when the script instance is first created and attached to the entity. Use this for initialization.
*   **`OnEnable()`**: Called when the script becomes enabled (initially true, or after `SetEnabled(true)`).
*   **`OnUpdate(float dt)`**: Called every frame, but ONLY if `IsEnabled()` is true.
*   **`OnDisable()`**: Called when the script becomes disabled (`SetEnabled(false)`).
*   **`OnDestroy()`**: Called when the script is removed or the entity is destroyed.

### Physics Callbacks
These are called when the entity's `RigidBodyComponent` interacts with others:
*   `OnCollisionEnter(entt::entity other)`
*   `OnCollisionStay(entt::entity other)`
*   `OnCollisionExit(entt::entity other)`
*   `OnTriggerEnter(entt::entity other)`
*   `OnTriggerStay(entt::entity other)`
*   `OnTriggerExit(entt::entity other)`

## 3. Registering Scripts
To make your script available to the engine (e.g., for loading from `.scene` files), you must use the `REGISTER_SCRIPT(ClassName)` macro at the end of your header or source file.

```cpp
REGISTER_SCRIPT(PlayerController)
```

## 4. Using in Scene
Attach the script to an entity using the `SCRIPT` command in your scene file:

```text
NEW_ENTITY MyEntity
SCRIPT MyScript
```

## 5. Entity Access
Inside a script, you have access to:
*   `m_Entity`: The ID of the entity this script is attached to.
*   `m_Scene`: Pointer to the `Scene`.
*   `m_App`: Pointer to the `Application`.

```cpp
void OnUpdate(float dt) override
{
    // Access Transform helper
    if (HasComponent<TransformComponent>()) {
        auto& transform = GetComponent<TransformComponent>();
        transform.position.x += 5.0f * dt;
    }
}
```
