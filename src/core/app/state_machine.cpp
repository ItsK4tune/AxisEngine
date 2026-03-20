#include <core/app/state_machine.h>
#include <core/logic/service_locator.h>
#include <scene/logic/scene.h>

StateMachine::StateMachine() {}

void StateMachine::Initialize()
{
}

void StateMachine::Shutdown()
{
    Clear();
}

void StateMachine::PushState(std::unique_ptr<State> state)
{
    auto& scene = ServiceLocator::Instance().Require<Scene>();
    state->SetActiveScene(&scene);
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
