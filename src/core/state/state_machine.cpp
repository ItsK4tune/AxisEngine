#include <core/engine_context.h>
#include <core/state/state_machine.h>

StateMachine::StateMachine() {}

void StateMachine::Init(EngineContext ctx)
{
    m_Ctx = ctx;
}

void StateMachine::Shutdown()
{
    Clear();
}

void StateMachine::PushState(std::unique_ptr<State> state)
{
    state->SetContext(m_Ctx);
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
