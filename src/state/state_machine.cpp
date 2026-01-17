#include <state/state_machine.h>

StateMachine::StateMachine(Application *app) : m_App(app) {}

void StateMachine::PushState(std::unique_ptr<State> state)
{
    state->SetContext(m_App);
    state->OnEnter();
    m_States.push(std::move(state));
}

void StateMachine::PopState()
{
    if (!m_States.empty())
    {
        m_States.top()->OnExit();
        m_States.pop();
    }
}

void StateMachine::Clear()
{
    while (!m_States.empty())
    {
        PopState();
    }
}

void StateMachine::ChangeState(std::unique_ptr<State> state)
{
    PopState();
    PushState(std::move(state));
}

State *StateMachine::GetCurrentState()
{
    return m_States.empty() ? nullptr : m_States.top().get();
}

void StateMachine::Update(float dt)
{
    if (State *s = GetCurrentState())
        s->OnUpdate(dt);
}

void StateMachine::FixedUpdate(float fixedDt)
{
    if (State *s = GetCurrentState())
        s->OnFixedUpdate(fixedDt);
}

void StateMachine::Render()
{
    if (State *s = GetCurrentState())
        s->OnRender();
}
