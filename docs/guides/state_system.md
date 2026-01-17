# State System

The AXIS Engine uses a Stack-based State Machine to manage high-level game flow (e.g., Intro -> Menu -> Gameplay).

## 1. The State Class

A `State` represents a distinct phase of the application (e.g., Menu, Gameplay, Pause).

### Lifecycle Methods
Every state must implement the following lifecycle methods:

*   **`OnEnter()`**: Called when the state becomes currently active. Use this to `LoadScene`, configure the cursor, or enable specific systems.
*   **`OnUpdate(float dt)`**: Called every frame. Handle High-Level game logic here (e.g., checking for Pause key). **Note:** Systems (Script, Audio, Animation) are updated automatically by the Engine, you do NOT need to update them here.
*   **`OnFixedUpdate(float fixedDt)`**: Called on a fixed interval.
*   **`OnRender()`**: Called after update. **Note:** Rendering is handled automatically by the Engine. Use this only for custom, state-specific rendering if absolutely necessary.
*   **`OnExit()`**: Called when the state is removed. Cleanup scenes and resources here.

### API Wrappers (New)
States now have direct wrapper methods to control the engine, simplifying your code:

**Scene Management:**
*   `LoadScene(path)`: Loads a scene additively.
*   `UnloadScene(path)`: Unloads a specific scene.
*   `ChangeScene(path)`: Unloads ALL current scenes and loads the new one.

**System Control:**
*   `EnablePhysics(bool)`: Enable/Disable physics simulation.
*   `EnableRender(bool)`: Enable/Disable rendering.
*   `EnableAudio(bool)`: Enable/Disable audio.
*   `EnableLogic(bool)`: Enable/Disable script updates.
*   `SetCursorMode(mode)`: `CursorMode::Normal`, `Hidden`, or `Locked`.

**Accessors:**
*   `GetSceneManager()`, `GetResourceManager()`, `GetInputManager()`...
*   `GetRenderSystem()`, `GetPhysicsSystem()`...

## 2. StateMachine

The `StateMachine` manages the stack of states.

### Operations
*   **`PushState(state)`**: Pauses current state and adds new state on top.
*   **`PopState()`**: Removes current state and resumes the previous one.
*   **`ChangeState(state)`**: Removes current state and replaces it with the new one.

### Example: Creating a GameState

```cpp
#include <engine/core/state.h>
#include <states/menu_state.h>

class GameState : public State {
public:
    void OnEnter() override {
        // 1. Setup Scene
        LoadScene("scenes/level1.scene");
        SetCursorMode(CursorMode::Locked);

        // 2. Enable necessary systems
        EnablePhysics(true);
        EnableRender(true);
        EnableLogic(true);
    }

    void OnUpdate(float dt) override {
        // Systems are updated AUTOMATICALLY by Application.
        // Only handle specific state logic here.
        
        if (GetKeyboard().GetKeyDown(GLFW_KEY_ESCAPE)) {
            m_App->GetStateMachine().ChangeState(std::make_unique<MenuState>());
        }
    }

    void OnExit() override {
        GetSceneManager().ClearAllScenes();
    }
};
```
