#pragma once

#include <core/unit/engine_accessor.h>
#include <memory>
#include <stack>

// --- State ---

class State : public EngineAccessor
{
public:
    virtual ~State() = default;

    virtual void OnEnter() = 0;
    virtual void OnUpdate(float dt) = 0;
    virtual void OnFixedUpdate(float fixedDt) {}
    virtual void OnRender() = 0;
    virtual void OnRenderDebug() {}
    virtual void OnExit() = 0;
};

// --- State Machine ---

class StateMachine
{
public:
    StateMachine();

    void Initialize();
    void Shutdown();

    void PushState(std::unique_ptr<State> state);
    void PopState();
    void Clear();
    void ChangeState(std::unique_ptr<State> state);

    State *GetCurrentState();

    void Update(float dt);
    void FixedUpdate(float fixedDt);
    void Render();

private:
    std::stack<std::unique_ptr<State>> m_States;
};
