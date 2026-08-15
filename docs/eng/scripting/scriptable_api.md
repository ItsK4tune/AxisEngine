# Scriptable API & Entity Behavior Lifecycle Guide

> [Tiếng Việt](../../vi/scripting/scriptable_api.md) | [State API](../state/state_api.md) | [Components Reference](../guides/components_reference.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine enables C++ entity behavior scripting by subclassing `Scriptable`. Attached to entities via the `ScriptComponent`, `Scriptable` instances receive lifecycle callbacks, physics collision hooks, input events, and delayed action timers (`Invoke`).

### Complete `Scriptable` Lifecycle Stages
1. **Binding (`Initialize`)**: Engine links the `entt::entity` handle and active `Scene*` pointer to the script instance.
2. **Creation (`OnCreate`)**: Executed once when the entity and script enter the active scene. Ideal for component lookups and initial state setup.
3. **Activation & Deactivation (`OnEnable` / `OnDisable`)**: Executed whenever `SetEnabled(true)` or `SetEnabled(false)` is called. Controls active update execution.
4. **Frame Execution (`OnUpdate`, `OnFixedUpdate`)**:
   - `OnUpdate(float dt)`: Executed during variable logic frame updates for movement, timers, and input handling.
   - `OnFixedUpdate(float fixedDt)`: Executed during fixed physics timesteps for forces and kinematic updates.
5. **Physics Intersections (`OnCollision*`, `OnTrigger*`)**:
   - Collision hooks: `OnCollisionEnter(other)`, `OnCollisionStay(other)`, `OnCollisionExit(other)`.
   - Trigger hooks: `OnTriggerEnter(other)`, `OnTriggerStay(other)`, `OnTriggerExit(other)`.
6. **Input Events (`OnKey*`, `OnMouse*`)**:
   - Keyboard: `OnKeyPress(key)`, `OnKeyRelease(key)`.
   - Mouse: `OnMouseButtonPress(btn)`, `OnMouseButtonRelease(btn)`, `OnMouseEnter()`, `OnMouseExit()`, `OnMouseOver()`, `OnMouseClicked(btn)`.
7. **Teardown (`OnDestroy`)**: Executed when the script component or entity is deleted.

---

## 2. How to Use

1. **Subclass `Scriptable`**: Create a class inheriting `Scriptable` and override necessary lifecycle callbacks (`OnCreate()`, `OnUpdate(float dt)`, `OnCollisionEnter(other)`, `OnDestroy()`).
2. **Register Script Factory**: Call `app.RegisterScript<MyScript>("MyScript")` inside `Application::RegisterUserScripts()`.
3. **Attach to Entity**: Call `entity.AddScript<MyScript>("MyScript")` in C++ code or specify `Script:` in `.axs` scene files.
4. **Schedule Delayed Actions**: Call `Invoke([this]() { DoSomething(); }, delaySeconds)` to schedule asynchronous delayed callbacks.

---

## 3. Examples

### Player Controller Script with Complete Lifecycle Callbacks Example
```cpp
#include <axis_sdk.h>

class PlayerScript final : public Scriptable {
private:
    float m_moveSpeed = 6.0f;
    int m_health = 100;

public:
    void OnCreate() override {
        AXIS_LOG_INFO("PlayerScript initialized on entity ID: " + std::to_string(static_cast<uint32_t>(m_Entity)));

        // Schedule delayed health regen after 5 seconds
        Invoke([this]() {
            m_health = 100;
            AXIS_LOG_INFO("Player health restored!");
        }, 5.0f);
    }

    void OnEnable() override {
        AXIS_LOG_INFO("PlayerScript enabled");
    }

    void OnUpdate(float dt) override {
        auto& input = InputManager::Get();
        auto& transform = GetComponent<TransformComponent>();

        if (input.IsKeyDown(KeyCode::W)) {
            transform.Translate(Vector3(0.0f, 0.0f, m_moveSpeed * dt));
        }
    }

    void OnFixedUpdate(float fixedDt) override {
        // Physics updates
    }

    void OnTriggerEnter(entt::entity other) override {
        if (CompareTag(other, "Pickup")) {
            AXIS_LOG_INFO("Item pickup collected!");
            Destroy(other);
        }
    }

    void OnDisable() override {
        AXIS_LOG_INFO("PlayerScript disabled");
    }

    void OnDestroy() override {
        AXIS_LOG_INFO("PlayerScript destroyed");
    }
};

void RegisterAppScripts(Application& app) {
    app.RegisterScript<PlayerScript>("PlayerScript");
}
```

---

## 4. API & Configuration Reference

### `Scriptable` Lifecycle Callbacks Reference

| Method Name | Call Timing / Trigger Condition | Description |
| :--- | :--- | :--- |
| `Initialize(entity, scene)` | System Binding | Links EnTT entity handle and scene pointer |
| `OnCreate()` | Entity Initialization | Executed once when entity script enters active scene |
| `OnEnable()` | Activation | Executed when script is enabled (`SetEnabled(true)`) |
| `OnDisable()` | Deactivation | Executed when script is disabled (`SetEnabled(false)`) |
| `OnUpdate(float dt)` | Every Frame | Executed during variable logic update loop |
| `OnFixedUpdate(float fixedDt)` | Fixed Physics Timestep | Executed during fixed Bullet 3D physics step |
| `OnCollisionEnter(other)` | Rigid Body Collision | Triggered when physical collision begins |
| `OnCollisionStay(other)` | Rigid Body Collision | Triggered continuously while collision persists |
| `OnCollisionExit(other)` | Rigid Body Collision | Triggered when physical collision separates |
| `OnTriggerEnter(other)` | Trigger Volume | Triggered when entity enters trigger volume |
| `OnTriggerStay(other)` | Trigger Volume | Triggered while entity remains inside trigger volume |
| `OnTriggerExit(other)` | Trigger Volume | Triggered when entity exits trigger volume |
| `OnKeyPress(key)` | Keyboard Input | Triggered when key is pressed down |
| `OnKeyRelease(key)` | Keyboard Input | Triggered when key is released |
| `OnMouseEnter()` | Mouse Hover | Triggered when mouse cursor enters entity UI/bounds |
| `OnMouseExit()` | Mouse Hover | Triggered when mouse cursor leaves entity UI/bounds |
| `OnMouseClicked(btn)` | Mouse Click | Triggered when entity UI/bounds is clicked |
| `OnDestroy()` | Teardown | Executed when script or entity is removed |

### `Scriptable` Helper Utilities Reference

| Method Name | Return Type | Description |
| :--- | :--- | :--- |
| `GetComponent<T>()` | `T&` | Retrieves reference to component `T` on current entity |
| `HasComponent<T>()` | `bool` | Checks if current entity possesses component `T` |
| `GetScript<T>(targetEntity)` | `T*` | Retrieves pointer to script `T` on target entity |
| `SetEnabled(enabled)` | `void` | Enables or disables script ticks (`OnEnable`/`OnDisable`) |
| `SetRunWhenPaused(run)` | `void` | Configures script to continue ticking when game is paused |
| `Invoke(callback, delaySeconds)` | `void` | Schedules a delayed lambda callback |
| `Spawn(name, pos, rot, scale)` | `entt::entity` | Spawns a new entity into the active scene |
| `Destroy(targetEntity)` | `void` | Removes target entity from active scene |
| `CompareTag(entity, tag)` | `bool` | Checks if target entity matches tag string |
| `CompareName(entity, name)` | `bool` | Checks if target entity matches name string |
