#pragma once

#include <core/engine_context.h>
#include <memory>
#include <stack>
#include <core/state/state.h>

class StateMachine
{
public:
    StateMachine();

    void Init(EngineContext ctx);
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
    EngineContext m_Ctx;
};
