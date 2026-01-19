# State API Reference

## Table of Contents
- [Overview](#overview)
- [State Lifecycle](#state-lifecycle)
- [Creating Custom States](#creating-custom-states)
- [State Methods](#state-methods)
- [System Control](#system-control)
- [Manager Access](#manager-access)
- [Common Patterns](#common-patterns)

---

## Overview

The **State** class represents a distinct mode or screen in your game (e.g., Menu, Gameplay, Pause, Settings). States manage their own lifecycle and can enable/disable specific engine systems as needed.

### Key Concepts
- **Lifecycle Hooks**: OnEnter, OnUpdate, OnFixedUpdate, OnRender, OnExit
- **System Control**: Enable/disable rendering, physics, audio, etc.
- **Scene Management**: Load, unload, and switch scenes
- **Manager Access**: Direct access to all engine managers and systems

---

## State Lifecycle

```
 Application::PushState<GameState>()
            ↓
       OnEnter() ────────────────┐
            ↓                     │ Initialize:
       Load Scene                 │ - Load scenes
       Enable Systems             │ - Enable systems
            ↓                     │ - Set cursor mode
            │                     │ - Load resources
            ↓                     ↓
    ┌──────────────────────────────┐
    │    MAIN GAME LOOP            │
    │                              │
    │  OnFixedUpdate(fixedDt) ←─┐  │ Fixed timestep
    │         ↓                 │  │ (Physics)
    │  OnUpdate(dt) ←──────────┼─┐│
    │         ↓                 │ ││ Frame update
    │  OnRender() ─────────────┘ ││ (Logic + Render)
    │         ↓                   ││
    └──────────┬──────────────────┘│
               │                   │
               │  (State Change)   │
               ↓                   │
          OnExit() ────────────────┘
               ↓
       Cleanup Resources
       Clear Scenes
```

### Lifecycle Methods

| Method | When Called | Purpose |
|--------|-------------|---------|
| `OnEnter()` | Once, when state becomes active | Load scenes, initialize resources, enable systems |
| `OnFixedUpdate(float fixedDt)` | Multiple times per frame (fixed timestep) | Physics updates, fixed-rate logic |
| `OnUpdate(float dt)` | Once per frame (variable timestep) | Game logic, input handling |
| `OnRender()` | Once per frame, after Update | Custom rendering (optional) |
| `OnExit()` | Once, when leaving state | Cleanup, unload scenes, save data |

---

## Creating Custom States

### Basic State Template

```cpp
#include <state/state.h>

class MyGameState : public State {
public:
    void OnEnter() override {
        // Load your game scene
        LoadScene("scenes/my_level.scene");
        
        // Enable/disable systems as needed
        EnablePhysics(true);
        EnableRender(true);
        EnableAudio(true);
        EnableLogic(true);  // Scripts, animation, particles, etc.
        
        // Configure input
        SetCursorMode(CursorMode::LockedHiddenCenter);
    }
    
    void OnUpdate(float dt) override {
        // Optional: Custom per-frame logic
        // Most logic should be in scripts
        
        // Example: Check for pause input
        if (GetKeyboard().GetKeyDown(GLFW_KEY_ESCAPE)) {
            // Transition to pause state
        }
    }
    
    void OnFixedUpdate(float fixedDt) override {
        // Optional: Custom physics-rate logic
    }
    
    void OnRender() override {
        // Optional: Custom rendering
        // Most rendering is handled automatically by RenderSystem
    }
    
    void OnExit() override {
        // Cleanup
        GetSceneManager().ClearAllScenes();
    }
};
```

### Registering and Using States

```cpp
// In main.cpp or Application setup
int main() {
    Application app;
    
    if (app.Init()) {
        // Push initial state
        app.PushState<MenuState>();
        app.Run();
    }
    
    return 0;
}
```

---

## State Methods

### Scene Management

#### LoadScene
```cpp
void LoadScene(const std::string& path);
```
Loads a .scene file and adds its entities to the current scene.

**Example:**
```cpp
void OnEnter() override {
    LoadScene("scenes/level_01.scene");
}
```

#### UnloadScene
```cpp
void UnloadScene(const std::string& path);
```
Unloads a previously loaded scene, destroying its entities.

**Example:**
```cpp
UnloadScene("scenes/ui_overlay.scene");
```

#### ChangeScene
```cpp
void ChangeScene(const std::string& path);
```
Clears all current scenes and loads a new one.

**Example:**
```cpp
void OnUpdate(float dt) override {
    if (m_LevelComplete) {
        ChangeScene("scenes/next_level.scene");
    }
}
```

---

## System Control

### Individual System Control

#### EnablePhysics
```cpp
void EnablePhysics(bool enable);
```
Enable/disable physics simulation.

**Example:**
```cpp
// Pause physics during cutscene
EnablePhysics(false);
```

#### EnableRender
```cpp
void EnableRender(bool enable);
```
Enable/disable 3D mesh rendering.

**Example:**
```cpp
// Hide 3D world in menu
EnableRender(false);
```

#### EnableAudio
```cpp
void EnableAudio(bool enable);
```
Enable/disable audio playback.

#### EnableScript
```cpp
void EnableScript(bool enable);
```
Enable/disable script execution (all Scriptable OnUpdate calls).

#### EnableAnimation
```cpp
void EnableAnimation(bool enable);
```
Enable/disable skeletal animations.

#### EnableVideo
```cpp
void EnableVideo(bool enable);
```
Enable/disable video decoding.

#### EnableUIInteract
```cpp
void EnableUIInteract(bool enable);
```
Enable/disable UI button interactions.

#### EnableUIRender
```cpp
void EnableUIRender(bool enable);
```
Enable/disable UI rendering.

#### EnableParticle
```cpp
void EnableParticle(bool enable);
```
Enable/disable particle system updates/rendering.

#### EnableSkybox
```cpp
void EnableSkybox(bool enable);
```
Enable/disable skybox rendering.

### Group System Control

#### EnableLogic
```cpp
void EnableLogic(bool enable);
```
Group control for gameplay systems:
- ScriptableSystem
- AnimationSystem
- VideoSystem
- UIInteractSystem
- ParticleSystem

**Example:**
```cpp
void OnEnter() override {
    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);  // Enables all gameplay systems at once
}
```

---

## Manager Access

### Input Managers

```cpp
KeyboardManager& GetKeyboard();
MouseManager& GetMouse();
InputManager& GetInputManager();
```

**Example:**
```cpp
void OnUpdate(float dt) override {
    auto& keyboard = GetKeyboard();
    auto& mouse = GetMouse();
    
    if (keyboard.GetKeyDown(GLFW_KEY_SPACE)) {
        // Jump
    }
    
    if (mouse.GetButtonDown(GLFW_MOUSE_BUTTON_LEFT)) {
        // Shoot
    }
}
```

### Resource & Scene Managers

```cpp
ResourceManager& GetResourceManager();
SceneManager& GetSceneManager();
SoundManager& GetSoundManager();
AppHandler& GetAppHandler();
```

**Example:**
```cpp
void OnEnter() override {
    auto& res = GetResourceManager();
    
    // Load additional resources at runtime
    res.LoadModel("bossModel", "models/boss.fbx", true);
    res.LoadSound("bossMusic", "audio/boss_theme.mp3");
}
```

### System Accessors

```cpp
RenderSystem& GetRenderSystem();
PhysicsSystem& GetPhysicsSystem();
AudioSystem& GetAudioSystem();
ScriptableSystem& GetScriptSystem();
UIRenderSystem& GetUIRenderSystem();
UIInteractSystem& GetUIInteractSystem();
ParticleSystem& GetParticleSystem();
SkyboxRenderSystem& GetSkyboxRenderSystem();
AnimationSystem& GetAnimationSystem();
VideoSystem& GetVideoSystem();
```

**Example:**
```cpp
void OnUpdate(float dt) override {
    auto& renderSystem = GetRenderSystem();
    
    // Toggle shadows dynamically
    if (GetKeyboard().GetKeyDown(GLFW_KEY_F7)) {
        bool shadows = !renderSystem.IsShadowsEnabled();
        renderSystem.SetEnableShadows(shadows);
    }
}
```

---

## Common Patterns

### Pattern 1: Menu State

```cpp
class MenuState : public State {
    void OnEnter() override {
        LoadScene("scenes/main_menu.scene");
        
        EnablePhysics(false);  // No physics in menu
        EnableRender(true);    // Show background/UI
        EnableAudio(true);     // Menu music
        EnableLogic(true);     // UI interaction
        
        SetCursorMode(CursorMode::Normal);
    }
    
    void OnUpdate(float dt) override {
        // Menu logic handled by UI scripts
    }
    
    void OnRender() override {
        // Optional: Custom menu effects
    }
    
    void OnExit() override {
        GetSceneManager().ClearAllScenes();
    }
};
```

### Pattern 2: Gameplay State

```cpp
class GameplayState : public State {
    void OnEnter() override {
        LoadScene("scenes/level_01.scene");
        
        EnablePhysics(true);
        EnableRender(true);
        EnableAudio(true);
        EnableLogic(true);
        
        SetCursorMode(CursorMode::LockedHiddenCenter);
    }
    
    void OnUpdate(float dt) override {
        // Check for pause
        if (GetKeyboard().GetKeyDown(GLFW_KEY_ESCAPE)) {
            // Push pause state (don't exit this state)
            // m_App->PushState<PauseState>();
        }
    }
    
    void OnExit() override {
        GetSceneManager().ClearAllScenes();
    }
};
```

### Pattern 3: Pause State (Overlay)

```cpp
class PauseState : public State {
    void OnEnter() override {
        LoadScene("scenes/pause_menu.scene");
        
        // Don't clear the game scene, just overlay pause UI
        EnablePhysics(false);  // Freeze gameplay
        EnableLogic(false);    // Pause scripts
        EnableRender(true);    // Keep showing game world
        EnableUIRender(true);  // Show pause UI
        
        SetCursorMode(CursorMode::Normal);
        
        // Pause the application time scale
        // m_App->SetPaused(true);
    }
    
    void OnUpdate(float dt) override {
        if (GetKeyboard().GetKeyDown(GLFW_KEY_ESCAPE)) {
            // Resume game - pop this state
            // m_App->PopState();
        }
    }
    
    void OnExit() override {
        // Only unload pause UI, not the game scene
        UnloadScene("scenes/pause_menu.scene");
        
        // Resume game
        EnablePhysics(true);
        EnableLogic(true);
        SetCursorMode(CursorMode::LockedHiddenCenter);
    }
};
```

### Pattern 4: Loading State

```cpp
class LoadingState : public State {
    std::string m_NextScene;
    float m_LoadProgress = 0.0f;
    
public:
    LoadingState(const std::string& nextScene) 
        : m_NextScene(nextScene) {}
    
    void OnEnter() override {
        LoadScene("scenes/loading_screen.scene");
        
        EnableRender(true);
        EnableUIRender(true);
        EnablePhysics(false);
        EnableLogic(false);
        
        SetCursorMode(CursorMode::Hidden);
    }
    
    void OnUpdate(float dt) override {
        // Simulate loading (in real game, check actual asset loading)
        m_LoadProgress += dt * 0.5f;
        
        if (m_LoadProgress >= 1.0f) {
            // Loading complete, transition to next scene
            ChangeScene(m_NextScene);
            // m_App->ChangeState<GameplayState>();
        }
    }
    
    void OnRender() override {
        // Update loading bar UI
        // (handled by UI scripts with m_LoadProgress)
    }
    
    void OnExit() override {
        GetSceneManager().ClearAllScenes();
    }
};
```

### Pattern 5: Cutscene State

```cpp
class CutsceneState : public State {
    void OnEnter() override {
        LoadScene("scenes/cutscene_01.scene");
        
        EnablePhysics(false);   // No physics during cutscene
        EnableRender(true);     // Show scene
        EnableAudio(true);      // Play dialogue/music
        EnableAnimation(true);  // Character animations
        EnableScript(true);     // Cutscene script
        EnableVideo(true);      // Play video if needed
        
        SetCursorMode(CursorMode::Hidden);
    }
    
    void OnUpdate(float dt) override {
        // Allow skip
        if (GetKeyboard().GetKeyDown(GLFW_KEY_SPACE)) {
            // Skip to next state
            // m_App->ChangeState<GameplayState>();
        }
    }
    
    void OnExit() override {
        GetSceneManager().ClearAllScenes();
    }
};
```

---

## Best Practices

### ✅ DO
- Load scenes in `OnEnter()`
- Cleanup in `OnExit()`
- Use `EnableLogic()` to control all logic systems at once
- Keep `OnUpdate()` lightweight - most logic should be in scripts
- Use `SetCursorMode()` appropriate to the state (Menu = Normal, Gameplay = Locked)

### ❌ DON'T
- Don't do heavy computation in `OnUpdate()` every frame
- Don't forget to cleanup scenes in `OnExit()`
- Don't enable systems you don't need (performance)
- Don't access systems before they're initialized

---

## See Also
- [State Machine Guide](../guides/state_system.md)
- [Scriptable API](scriptable_api.md)
- [Scene Format](../guides/scene_format.md)
- [Input Manager API](managers/input_manager.md)
