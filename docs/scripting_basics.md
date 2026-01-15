# Scripting Basics

Scripts in the engine are C++ classes inheriting from `Scriptable`. They allow you to attach logic to entities.

## 1. Creating a Script

Inherit from `Scriptable` and override lifecycle methods.

```cpp
#include <core/scriptable.h>

class PlayerController : public Scriptable
{
public:
    virtual void OnCreate() override;
    virtual void OnUpdate(float dt) override;
    
    // Lifecycle
    virtual void OnEnable() override;  // Called when enabled
    virtual void OnDisable() override; // Called when disabled
    virtual void OnDestroy() override;
};
```

### Lifecycle Methods
- **OnCreate()**: Called once when the script starts.
- **OnUpdate(dt)**: Called every frame. `dt` is delta time.
- **OnEnable()**: Called when the script becomes active.
- **OnDisable()**: Called when the script is disabled.
- **OnDestroy()**: Called when the script/entity is removed.

## 2. Registering Scripts

To use a script in a `.scene` file, you must register it.

```cpp
#include "player_controller.h"
#include <core/script_registry.h>

REGISTER_SCRIPT(PlayerController)
```

## 3. Disabling Scripts

You can enable/disable scripts at runtime. `OnUpdate` will not be called for disabled scripts.

```cpp
if (Input::GetKeyDown(KEY_P)) {
    SetEnabled(false); // Triggers OnDisable()
}
```
