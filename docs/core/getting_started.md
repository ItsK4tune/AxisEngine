# Getting Started with AXIS Engine

## Table of Contents
- [Introduction](#introduction)
- [First Project Setup](#first-project-setup)
- [Creating Your First Scene](#creating-your-first-scene)
- [Writing Your First Script](#writing-your-first-script)
- [Building and Running](#building-and-running)
- [Next Steps](#next-steps)

---

## Introduction

This guide will walk you through creating your first game with AXIS Engine. We'll build a simple scene with a player character, camera, and basic interaction.

**What you'll learn:**
- How to create a scene file
- How to write a gameplay script
- How to build and run your game
- How to use the debug system

**Prerequisites:**
- AXIS Engine built and ready (see [Build Guide](build_guide.md))
- Basic C++ knowledge
- Text editor for `.scene` files

---

## First Project Setup

AXIS Engine uses the `/game` directory for your game project. The engine provides the framework, and you create your game logic in this directory.

### Project Structure
```
GameEngine/
├── game/
│   ├── src/
│   │   ├── main.cpp           # Entry point
│   │   ├── states/           # Your game states
│   │   └── scripts/           # Your gameplay scripts
│   └── includes/
│       └── states/
│       └── scripts/
├── scenes/                    # Your scene files
├── includes/engine/asset/                # Default assets (shaders, fonts, models)
└── bin/Release/              # Built executable
```

### Default Assets

The engine automatically loads default assets from `includes/engine/asset/load.axs` at startup:
- Shaders (Phong, PBR, Unlit, UI, Text, Skybox, Particle, VideoMap)
- Fonts (`time.ttf`)
- Models (plane, dummy)
- Skybox (default cubemap)

You can add more assets there or load them in your scenes.

---

## Creating Your First Scene

Let's create a simple scene with a player, ground plane, and a light.

### Step 1: Create Scene File

Create `scenes/my_first_level.axs`:

```yaml
axis_scene:
  Config:
    SHADOWS: 1
    DEPTH_TEST: 1 LESS
    VSYNC: 1

  Entities:
    MainCamera:
      Tag: MainCamera
      Component: Transform
        Position: 0 5 10
      Component: Camera
        Primary: 1
        FOV: 45.0
        Yaw: -90.0
        Pitch: 0.0
      Component: Script
        Class: CameraController

    Ground:
      Tag: default
      Component: Transform
        Scale: 10 1 10
      Component: Renderer
        Model: planeModel
        Shader: phongLitShadowShader
      Component: Material
        Type: PHONG
        Shininess: 32
        Specular: 0.5 0.5 0.5
      Component: RigidBody
        Type: BOX
        Size: 100 0.1 100
        Mass: 0.0
        BodyType: STATIC

    Player:
      Tag: Player
      Component: Transform
        Position: 0 2 0
      Component: Renderer
        Model: capsuleModel
        Shader: phongLitShadowShader
      Component: Material
        Type: PHONG
      Component: RigidBody
        Type: CAPSULE
        Radius: 1.0
        Height: 1.8
        Mass: 1.0
        BodyType: DYNAMIC
      Component: Script
        Class: PlayerController

    Sun:
      Tag: default
      Component: Transform
        Rotation: -45 -30 0
      Component: LightDir
        Color: 1.0 1.0 1.0

    FPSText:
      Tag: UI
      Component: UITransform
        Position: 10 10
        Size: 200 40
      Component: UIText
        Text: "FPS: 60"
        Font: time
        Color: 0 1 0
```

### Scene Syntax Explanation

Scenes use a YAML-like hierarchy.

**Entities** - Groups components under a unique name (e.g., `MainCamera`).

**Transform** - Defines Position, Rotation, and Scale. Omitted values default to 0 (or 1 for scale).

**Camera** - Marks an entity as a viewpoint. `Primary: 1` sets the active camera.

**Renderer** - Uses `Model` and `Shader` keys to reference loaded assets.

**Material** - Configures shading properties like `Shininess` and `Type`.

**RigidBody** - Sets physical properties. `Type: BOX` with `Mass: 0.0` creates a static floor.

**Script** - Attaches C++ logic using the `Class` name.

**LightDir** - Adds sunlight. The direction comes from the entity's `Transform` rotation.

**UIText** - Renders 2D text elements on the screen.

---

## Writing Your First Script

Scripts are C++ classes that control entity behavior.

### Step 1: Create Script Header

Create `game/includes/scripts/player_controller.h`:

```cpp
#pragma once
#include <script/scriptable.h>

class PlayerController : public Scriptable {
public:
    void OnCreate() override;
    void OnUpdate(float dt) override;
    void OnCollisionEnter(entt::entity other) override;
    
private:
    void HandleMovement(float dt);
    void HandleJump();
    
    TransformComponent* m_Transform = nullptr;
    RigidBodyComponent* m_RigidBody = nullptr;
    
    float m_Speed = 5.0f;
    float m_JumpForce = 8.0f;
    bool m_IsGrounded = false;
};
```

### Step 2: Implement Script

Create `game/src/scripts/player_controller.cpp`:

```cpp
#include "scripts/player_controller.h"
#include <GLFW/glfw3.h>

void PlayerController::OnCreate() {
    // Cache component references
    m_Transform = &GetComponent<TransformComponent>();
    m_RigidBody = &GetComponent<RigidBodyComponent>();
}

void PlayerController::OnUpdate(float dt) {
    HandleMovement(dt);
    HandleJump();
}

void PlayerController::HandleMovement(float dt) {
    auto& keyboard = GetKeyboard();
    glm::vec3 velocity(0.0f);
    
    // WASD movement
    if (keyboard.GetKey(GLFW_KEY_W)) velocity.z -= 1.0f;
    if (keyboard.GetKey(GLFW_KEY_S)) velocity.z += 1.0f;
    if (keyboard.GetKey(GLFW_KEY_A)) velocity.x -= 1.0f;
    if (keyboard.GetKey(GLFW_KEY_D)) velocity.x += 1.0f;
    
    // Apply movement
    if (glm::length(velocity) > 0.0f) {
        velocity = glm::normalize(velocity) * m_Speed;
        m_RigidBody->SetLinearVelocity(velocity);
    }
}

void PlayerController::HandleJump() {
    auto& keyboard = GetKeyboard();
    
    if (keyboard.GetKeyDown(GLFW_KEY_SPACE) && m_IsGrounded) {
        // Apply upward force
        glm::vec3 vel = m_RigidBody->body->getLinearVelocity();
        vel.y = m_JumpForce;
        m_RigidBody->SetLinearVelocity(vel);
        m_IsGrounded = false;
    }
}

void PlayerController::OnCollisionEnter(entt::entity other) {
    // Check if we landed on ground
    if (m_Scene->registry.all_of<InfoComponent>(other)) {
        auto& info = m_Scene->registry.get<InfoComponent>(other);
        if (info.name == "Ground") {
            m_IsGrounded = true;
        }
    }
}
```

#### Collision Filtering and Toggling
AXIS Engine makes it easy to filter collisions by entity `Tag` or `Name`. You can also configure a global **Collision Matrix** or toggle collisions dynamically:

```cpp
void PlayerController::OnCreate() {
    // Player and Bullets will never trigger OnCollision and will pass right through each other physically
    IgnoreTagCollision("Player", "Bullet");
}

void PlayerController::OnCollisionEnter(entt::entity other) {
    // Easily check what we hit
    if (CompareTag(other, "Enemy")) {
        TakeDamage();
        
        // Turn off our own physics temporarily (invincibility frames)
        SetCollisionEnabled(false);
    }
}
```

If `SetCollisionEnabled(false)` is called, the entity operates like a ghost (no physics resolution and no events fired) until set back to `true`.

#### Collision Event Alternative (Advanced)
If you need a system-wide manager (like an Achievement System) to listen to collisions without inheriting from `Scriptable`, you can subscribe to `EntityCollisionEvent` or `EntityTriggerEvent`:

```cpp
#include <event/physics_events.h>
#include <event/event_system.h>

void SystemInit() {
    EventSystem::Instance().Subscribe<EntityCollisionEvent>([](const auto& e) {
        if (e.type == CollisionEventType::Enter) {
            // Check if e.entityA or e.entityB is the player...
        }
    });
}
```

#### Event-Driven Input Alternative (Advanced)
Instead of polling `GetKeyboard().GetKeyDown(...)` every frame, AXIS Engine supports an **Event-Driven / Observer** model. You can subscribe to Input Events like `KeyPressedEvent`, `InputActionPressedEvent`, or `MouseMovedEvent`.

```cpp
#include <event/input_events.h>
#include <event/event_system.h>

void PlayerController::OnCreate() {
    // Other init...
    EventSystem::Instance().Subscribe<KeyPressedEvent>([this](const auto& e) {
        if (e.key == GLFW_KEY_SPACE && e.mods == 0) {
            HandleJumpEvent();
        }
    });
}
```
This avoids per-frame overhead and fires exactly when the OS reports a key press!

### Step 3: Register Script

In `game/src/main.cpp` or create `game/src/register_scripts.cpp`:

```cpp
#include <script/script_registry.h>
#include "scripts/player_controller.h"

// Register all your custom scripts
void RegisterGameScripts() {
    REGISTER_SCRIPT(PlayerController);
}
```

Then call in `main.cpp` BEFORE `app.Init()`:

```cpp
#include <app/application.h>
#include "states/game_state.h"

void RegisterGameScripts();  // Forward declaration

int main() {
    RegisterGameScripts();  // Register scripts before init
    
    Application app;
    if (app.Init()) {
        app.PushState<GameState>();
        app.Run();
    }
    return 0;
}
```

### Step 4: Create Game State

Create `game/includes/states/game_state.h`:

```cpp
#pragma once
#include <state/state.h>

class GameState : public State {
public:
    void OnEnter() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnExit() override;
};
```

Create `game/src/states/game_state.cpp`:

```cpp
#include "states/game_state.h"
#include <GLFW/glfw3.h>

void GameState::OnEnter() {
    // Load our scene
    LoadScene("scenes/my_first_level.scene");
    
    // Enable all systems
    EnablePhysics(true);
    EnableRender(true);
    EnableAudio(true);
    EnableLogic(true);
    
    // Hide and lock cursor for FPS-style control
    SetCursorMode(CursorMode::LockedHidden);
}

void GameState::OnUpdate(float dt) {
    // Check for exit
    if (GetKeyboard().GetKeyDown(GLFW_KEY_ESCAPE)) {
        // Back to menu or quit
    }
}

void GameState::OnRender() {
    // Optional: Custom rendering
}

void GameState::OnExit() {
    GetSceneManager().ClearAllScenes();
}
```

---

## Building and Running

### Step 1: Build

```bash
cd L:\C++\GameEngine
.\build_engine.bat
```

This will:
1. Configure CMake
2. Compile the engine
3. Compile your game project
4. Link everything
5. Output `bin/Release/GameEngine.exe`

### Step 2: Run

```bash
bin\Release\GameEngine.exe
```

### Step 3: Test

You should see:
- **Camera** at position (0, 5, 10) looking at the scene
- **Ground Plane** (large gray plane)
- **Player** (red dummy model) standing on the ground
- **FPS Counter** (green text, top-left)
- **Lighting** from the sun

**Controls:**
- `W` `A` `S` `D` - Move player
- `Space` - Jump
- `Mouse` - Look around (if camera script is attached)
- `F10` - Toggle FPS overlay
- `Escape` - Exit (if implemented in GameState)

---

## Using the Debug System

AXIS Engine has a powerful built-in debug system accessed via F-keys:

| Key | Function |
|-----|----------|
| `F1` | Show all debug controls |
| `F2` | Show device info (CPU, GPU, monitors, audio) |
| `F3` | Show performance stats |
| `F4` | Show entity statistics |
| `F5` | Dump scene graph to console |
| `F6` | Toggle wireframe mode |
| `F7` | Toggle no-texture mode |
| `F8` | Toggle physics debug rendering |
| `F10` | Toggle FPS overlay |
| `F11` | Pause game |
| `Shift+F11` | Toggle free debug camera |
| `F12` | Cycle time scale (0.25x, 0.5x, 1x, 1.5x, 2x) |

**Try this:**
1. Press `F10` to show FPS overlay
2. Press `F8` to see physics collision boxes
3. Press `F6` to see wireframe
4. Press `Shift+F11` to enter free camera mode
5. Press `F5` to print all entities to console

---

## Next Steps

### Add More Features

####1. Camera Follow Script**

Create `CameraFollow` script to make camera follow player:

```cpp
class CameraFollow : public Scriptable {
    entt::entity m_Target;
    glm::vec3 m_Offset = glm::vec3(0, 3, 5);
    
    void OnCreate() override {
        // Find player
        auto view = m_Scene->registry.view<InfoComponent>();
        for (auto entity : view) {
            if (view.get<InfoComponent>(entity).name == "Player") {
                m_Target = entity;
                break;
            }
        }
    }
    
    void OnUpdate(float dt) override {
        if (!m_Scene->registry.valid(m_Target)) return;
        
        auto& targetTr = m_Scene->registry.get<TransformComponent>(m_Target);
        auto& myTr = GetComponent<TransformComponent>();
        
        myTr.position = targetTr.position + m_Offset;
    }
};
```

#### 2. Add Enemies

Create new entities in your scene with `EnemyAI` script.

#### 3. Add UI

Create pause menu, health bar, score display using UI components.

#### 4. Add Audio

```cpp
void OnCollisionEnter(entt::entity other) override {
    GetSoundManager().PlaySound("hitSound", m_Transform->position, 1.0f);
}
```

### Learn More

- [Scripting Basics](../guides/scripting_basics.md) - Detailed scripting guide  
- [Scene Format Reference](../guides/scene_format.md) - All scene commands
- [Component Reference](../guides/components_reference.md) - All available components
- [State API](../state/state_api.md) - Complete State API
- [Scriptable API](../scripting/scriptable_api.md) - Complete Scriptable API

---

## Common Issues

### Script Not Found
```
[SceneLoader] 'PlayerController' script not found!
```
**Solution:** Make sure you called `REGISTER_SCRIPT(PlayerController)` before `app.Init()`.

### Model Not Loaded
```
ERROR: Model not found: playerModel
```
**Solution:** Check that the model is loaded in `load.scene` or your scene file with `LOAD_STATIC_MODEL`.

### Shader Errors
```
ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ
```
**Solution:** Verify shader paths point to `includes/engine/asset/shaders/...` not `resources/shaders/...`.

### Physics Not Working
```
Entity falls through ground
```
**Solution:** 
- Make sure ground has `STATIC` rigidbody
- Make sure player has `DYNAMIC` rigidbody
- Check Physics is enabled: `EnablePhysics(true)`

---

## Congratulations! 🎉

You've created your first AXIS Engine game with:
- ✅ A custom scene
- ✅ A player controller script
- ✅ Physics simulation
- ✅ Camera system
- ✅ Lighting
- ✅ UI

Keep experimenting and building! Check out the [examples](../examples/) folder for more complex patterns.
