# State System

The AXIS Engine uses a Stack-based State Machine to manage high-level game flow (e.g., Intro -> Menu -> Gameplay).

## 1. The State Class

A `State` represents a distinct phase of the application.

### Lifecycle Methods
Every state must implement the following lifecycle methods:

*   **`OnEnter()`**: Called when the state becomes currently active (pushed or changed to). Use this to load scenes, lock cursor, or play music.
*   **`OnUpdate(float dt)`**: Called every frame. Handle game logic, system updates (Audio, Animation, Particles) here.
*   **`OnFixedUpdate(float fixedDt)`**: Called on a fixed interval (default 60Hz). **MUST** handle Physics updates here.
*   **`OnRender()`**: Called after update. Render scenes and UI here.
*   **`OnExit()`**: Called when the state is removed or switched away from. Cleanup scenes and resources here.

### API Accessors
States have direct access to engine systems and managers via helper methods:

**Systems:**
*   `GetRenderSystem()`
*   `GetPhysicsSystem()`
*   `GetAudioSystem()`
*   `GetScriptSystem()`
*   `GetUIRenderSystem()`

**Managers:**
*   `GetSceneManager()`
*   `GetResourceManager()`
*   `GetSoundManager()`
*   `GetInputManager()` / `GetKeyboard()` / `GetMouse()`

## 2. StateMachine

The `StateMachine` manages the stack of states.

### Operations
*   **`PushState(state)`**: Pauses current state and adds new state on top.
*   **`PopState()`**: Removes current state and resumes the previous one.
*   **`ChangeState(state)`**: Removes current state and replaces it with the new one (Swap).
*   **`Clear()`**: Removes all states.

### Example: Creating a GameState

```cpp
#include <engine/core/state.h>
#include <states/menu_state.h>

class GameState : public State {
public:
    void OnEnter() override {
        GetSceneManager().LoadScene("scenes/level1.scene");
        GetMouse().SetCursorMode(CursorMode::Locked);
    }

    void OnUpdate(float dt) override {
        GetScriptSystem().Update(GetSceneManager().GetActiveScene(), dt, m_App);
        
        if (GetKeyboard().GetKeyDown(GLFW_KEY_ESCAPE)) {
            // Switch back to menu
            m_App->GetStateMachine().ChangeState(std::make_unique<MenuState>());
        }
    }

    void OnFixedUpdate(float fixedDt) override {
        GetPhysicsSystem().Update(GetSceneManager().GetActiveScene(), m_App->GetPhysicsWorld(), fixedDt);
    }

    void OnRender() override {
        GetRenderSystem().Render(GetSceneManager().GetActiveScene());
    }

    void OnExit() override {
        GetSceneManager().ClearAllScenes();
    }
};
```
