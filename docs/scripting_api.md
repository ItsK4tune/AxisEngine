# Scripting API Reference

This guide covers helper methods available inside `Scriptable` classes.

## 1. Input Mapping
Checks actions bound in `InputManager`.

```cpp
// Check if pressed this frame
if (GetActionDown("Jump")) { ... }

// Check if held
if (GetAction("MoveForward")) { ... }

// Check if released
if (GetActionUp("Fire")) { ... }
```

## 2. Scene Management
To switch scenes safely (deferred to end of frame):

```cpp
LoadScene("scenes/level2.scene");
```

## 3. Physics Callbacks
Override these methods to handle collision events.

```cpp
virtual void OnCollisionEnter(entt::entity other);
virtual void OnCollisionStay(entt::entity other);
virtual void OnCollisionExit(entt::entity other);

// For sensors/triggers
virtual void OnTriggerEnter(entt::entity other);
virtual void OnTriggerStay(entt::entity other);
virtual void OnTriggerExit(entt::entity other);
```

## 4. Components
Access components on the current entity.

```cpp
auto& rb = GetComponent<RigidBodyComponent>();
if (HasComponent<CameraComponent>()) { ... }
```

## 5. Core Systems Access
Access engine systems via `m_App`.

```cpp
m_App->GetSoundManager().Play2D("audio/click.wav");
m_App->GetPhysicsWorld()...
```
