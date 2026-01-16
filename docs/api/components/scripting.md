# Scripting Components

## ScriptComponent
**Struct:** `ScriptComponent`

Binds a C++ `Scriptable` class to an entity.

*   `Scriptable* instance`: Pointer to the allocated script instance.
*   `InstantiateScript`: Factory function to create the specific script type.
*   `DestroyScript`: Function to clean up the memory.

**Usage:**
Normally added via `REGISTER_SCRIPT` and `.scene` files, but can be added manually:

```cpp
auto& sc = registry.emplace<ScriptComponent>(entity);
sc.Bind<MyScriptClass>();
sc.instance = sc.InstantiateScript();
sc.instance->Init(entity, scenePtr, appPtr);
sc.instance->OnCreate();
```
