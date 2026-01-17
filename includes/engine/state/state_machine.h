#pragma once

#include <stack>
#include <memory>
#include "state.h"

class StateMachine
{
public:
    StateMachine(Application *app);

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
    Application *m_App;
};