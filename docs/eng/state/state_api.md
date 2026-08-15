# State Machine API & State Lifecycle Guide

> [Tiếng Việt](../../vi/state/state_api.md) | [Scriptable API](../scripting/scriptable_api.md) | [Getting Started Guide](../core/getting_started.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine organizes high-level application flow (Boot, Main Menu, Gameplay, Pause Menu, Game Over) using a **Stack-Based State Machine** (`StateMachine`). Each application state is written by subclassing `State` and implementing its lifecycle callbacks.

### State Lifecycle Stages
1. **Creation & Push (`OnEnter`)**: Executed when a state instance is pushed onto the stack. Allocates state-specific resources and loads scenes.
2. **Execution (`OnUpdate`, `OnFixedUpdate`, `OnRender`)**:
   - `OnUpdate(float dt)`: Called every variable frame tick for game logic, timer updates, and user input polling.
   - `OnFixedUpdate(float fixedDt)`: Called during fixed physics timesteps for deterministic physics queries.
   - `OnRender()`: Called during the rendering phase.
   - `OnRenderDebug()`: Called during the debug overlay pass.
3. **Suspension (`OnPause`)**: Executed when a new state is pushed on top of this state (for example, pushing a modal Pause Menu overlay over active Gameplay). The suspended state remains in memory.
4. **Resumption (`OnResume`)**: Executed when the overlay state on top is popped off, resuming this underlying state as the active top state.
5. **Teardown & Exit (`OnExit`)**: Executed when the state is popped off the stack or cleared via `ChangeState()`. Releases state resources and unloads scenes.

---

## 2. How to Use

1. **Subclass `State`**: Inherit from `State` and override `OnEnter()`, `OnUpdate(float dt)`, `OnFixedUpdate(float fixedDt)`, `OnRender()`, `OnPause()`, `OnResume()`, and `OnExit()`.
2. **Push Modal Overlays**: Call `Application::Get().PushState<PauseState>()` to push a modal menu over current gameplay. Current state receives `OnPause()`, and `PauseState` receives `OnEnter()`.
3. **Pop Active Overlays**: Call `Application::Get().PopState()` to return to the underlying state. `PauseState` receives `OnExit()`, and underlying state receives `OnResume()`.
4. **Transition Main Modes**: Call `Application::Get().ChangeState<GameplayState>()` to clear all states on the stack and enter `GameplayState`.

---

## 3. Examples

### Complete State Machine Stack Transition Example
```cpp
#include <axis_sdk.h>

class PauseState final : public State {
public:
    std::string GetName() const override { return "PauseState"; }

    void OnEnter() override {
        AXIS_LOG_INFO("PauseState::OnEnter - Modal pause overlay opened");
    }

    void OnUpdate(float dt) override {
        if (InputManager::Get().IsKeyPressed(KeyCode::Escape)) {
            // Pop pause overlay and resume underlying GameplayState
            Application::Get().PopState();
        }
    }

    void OnRender() override {}

    void OnExit() override {
        AXIS_LOG_INFO("PauseState::OnExit - Modal pause overlay closed");
    }
};

class GameplayState final : public State {
public:
    std::string GetName() const override { return "GameplayState"; }

    void OnEnter() override {
        AXIS_LOG_INFO("GameplayState::OnEnter - Loading level scene");
        SceneManager::Get().LoadScene("scenes/level1.axs");
    }

    void OnUpdate(float dt) override {
        if (InputManager::Get().IsKeyPressed(KeyCode::Escape)) {
            // Push PauseState on top without destroying GameplayState
            Application::Get().PushState<PauseState>();
        }
    }

    void OnFixedUpdate(float fixedDt) override {
        // Fixed physics step updates
    }

    void OnRender() override {}

    void OnPause() override {
        AXIS_LOG_INFO("GameplayState::OnPause - Suspended by modal pause menu");
    }

    void OnResume() override {
        AXIS_LOG_INFO("GameplayState::OnResume - Resumed gameplay after pause menu closed");
    }

    void OnExit() override {
        AXIS_LOG_INFO("GameplayState::OnExit - Unloading level scene");
    }
};
```

---

## 4. API & Configuration Reference

### State Lifecycle Callbacks Reference

| Method Name | Call Timing | Purpose |
| :--- | :--- | :--- |
| `OnEnter()` | State Activation | Called once when state is pushed to active stack |
| `OnUpdate(float dt)` | Variable Frame Loop | Called every variable frame tick for top active state |
| `OnFixedUpdate(float fixedDt)` | Fixed Physics Timestep | Called every fixed physics update tick |
| `OnRender()` | Frame Render Pass | Called during rendering phase |
| `OnRenderDebug()` | Debug Render Pass | Called during debug overlay rendering pass |
| `OnPause()` | Modal Suspension | Called when a new state is pushed on top of this state |
| `OnResume()` | Modal Resumption | Called when top state is popped off and this state resumes |
| `OnExit()` | State Teardown | Called when state is popped off stack or swapped out |

### StateMachine Stack Operations Reference

| Method Name | Target Stack Behavior | Lifecycle Method Triggers |
| :--- | :--- | :--- |
| `PushState<T>()` | Pushes new state `T` onto top of stack | Current top state receives `OnPause()`; new state receives `OnEnter()` |
| `PopState()` | Removes active top state from stack | Top state receives `OnExit()`; underlying state receives `OnResume()` |
| `ChangeState<T>()` | Replaces active state stack with state `T` | All active states receive `OnExit()`; new state receives `OnEnter()` |
| `Clear()` | Destroys all states on stack | Calls `OnExit()` on all active states and clears memory |
| `GetCurrentState()` | Queries top active state | Returns raw pointer `State*` to top active state |
| `GetStates()` | Queries full state stack | Returns `std::vector<State*>` ordered from bottom to top |
