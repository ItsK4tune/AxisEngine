# Scripting API Reference
![AXIS Engine Logo](../assets/logo.png)

**Engine**: AXIS Engine  
**Contributor**: Duong "Caftun" Nguyen

Common API methods available to `Scriptable` classes in the AXIS Engine.

## Input Handling
Access input via `m_App` (Application instance).

```cpp
if (m_App->GetAppHandler().GetKeyboard().GetKey(GLFW_KEY_SPACE)) {
    // Jump
}

if (m_App->GetAppHandler().GetMouse().IsButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
    // Fire
}
```

## Physics
Modify the `RigidBodyComponent` for physics-based movement.

```cpp
auto& rb = GetComponent<RigidBodyComponent>();
if (rb.body) {
    rb.body->applyCentralForce(btVector3(0, 10, 0));
}
```

## Audio
Play sounds via `SoundManager`.

```cpp
m_App->GetSoundManager().Play2D("sfx_explosion", false);
```

## Scene Management
Spawn or destroy entities.

```cpp
// Creation is usually done via SceneManager, or spawning prefabs (if supported)
// Destroy
m_Scene->registry.destroy(entity);
```
