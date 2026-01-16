# Scripting API Reference

Common API methods available to `Scriptable` classes in the AXIS Engine.

## Core Methods

| Method | Description |
| :--- | :--- |
| `SetEnabled(bool enabled)` | Enables or disables the script. Triggers `OnEnable`/`OnDisable`. |
| `IsEnabled()` | Returns true if the script is currently active. |
| `LoadScene(std::string path)` | Requests the application to load a new scene file. |

## Component Access

| Method | Description |
| :--- | :--- |
| `GetComponent<T>()` | Returns a reference to component `T` on this entity. Will throw/crash if missing. |
| `HasComponent<T>()` | Returns `true` if this entity has component `T`. |
| `GetScript<T>(entity target)` | Tries to get script of type `T` from `target`. Returns `nullptr` if not found. |

## Input Handling

Helper methods are provided to check input actions defined in `settings.json` or code:

```cpp
bool GetAction(const std::string& name);      // Returns true if button is currently held
bool GetActionDown(const std::string& name);  // Returns true during the frame the button was pressed
bool GetActionUp(const std::string& name);    // Returns true during the frame the button was released
```

Example:
```cpp
if (GetAction("Jump")) {
    // Player is holding the jump button
}
```

Direct access via `m_App` is also available:
```cpp
if (m_App->GetAppHandler().GetKeyboard().GetKey(GLFW_KEY_SPACE)) { ... }
```

## Physics Events

Override these methods to handle physics interactions:

```cpp
virtual void OnCollisionEnter(entt::entity other);
virtual void OnCollisionStay(entt::entity other);
virtual void OnCollisionExit(entt::entity other);

virtual void OnTriggerEnter(entt::entity other);
virtual void OnTriggerStay(entt::entity other);
virtual void OnTriggerExit(entt::entity other);
```

## Service Access

You can access global services via `m_App`:

*   **Sound**: `m_App->GetSoundManager()`
*   **Input**: `m_App->GetAppHandler()`
*   **Resources**: `m_App->GetResourceManager()` (if available)

### Audio Example
```cpp
m_App->GetSoundManager().Play2D("sfx_explosion", false);
```

### Entity Management
To spawn or destroy entities, interact with `m_Scene->registry`.

```cpp
// Destroy this entity
m_Scene->registry.destroy(m_Entity);
```
