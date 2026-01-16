# Scripting Basics
![AXIS Engine Logo](../assets/logo.png)

**Engine**: AXIS Engine  
**Contributor**: Duong "Caftun" Nguyen

Scripting in the AXIS Engine is done using Native C++ classes. Scripts are components derived from the `Scriptable` class.

## 1. Creating a Script

Inherit from `Scriptable` and implement the lifecycle methods.

```cpp
#include <engine/scriptable.h>

class MyScript : public Scriptable
{
public:
    void OnStart() override
    {
        // Called when the script starts
    }

    void OnUpdate(float dt) override
    {
        // Called every frame
    }
};
```

## 2. Registering Scripts
Scripts must be registered in the `SceneManager` or via `Application` to be usable in `.scene` files.

## 3. Using in Scene
Attach the script to an entity using the `SCRIPT` command.

```text
NEW_ENTITY MyEntity
SCRIPT MyScript
```

## 4. Entity Access
Inside a script, you have access to the attached `entity` (ID) and the `scene`.

```cpp
void OnUpdate(float dt) override
{
    // Access Transform
    auto& transform = GetComponent<TransformComponent>();
    transform.position.x += 5.0f * dt;
}
```
