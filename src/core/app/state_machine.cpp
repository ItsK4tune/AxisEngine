#include <core/app/state_machine.h>
#include <core/logic/service_locator.h>
#include <scene/logic/scene.h>
#include <algorithm>

StateMachine::StateMachine()
{
}

void StateMachine::Initialize()
{
}

void StateMachine::Shutdown()
{
    Clear();
}

void StateMachine::PushState(std::unique_ptr<State> state)
{
    if (!m_States.empty())
    {
        m_States.back()->OnPause();
    }
    auto& scene = ServiceLocator::Instance().Require<Scene>();
    state->SetActiveScene(&scene);
    state->OnEnter();
    m_States.push_back(std::move(state));
}

void StateMachine::PopState()
{
    if (!m_States.empty())
    {
        m_States.back()->OnExit();
        m_States.pop_back();
        if (!m_States.empty())
        {
            m_States.back()->OnResume();
        }
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

State* StateMachine::GetCurrentState()
{
    return m_States.empty() ? nullptr : m_States.back().get();
}

std::vector<State*> StateMachine::GetStates() const
{
    std::vector<State*> result;
    result.reserve(m_States.size());
    for (const auto& s : m_States)
    {
        result.push_back(s.get());
    }
    return result;
}

void StateMachine::Update(float dt)
{
    if (State* s = GetCurrentState())
        s->OnUpdate(dt);
}

void StateMachine::FixedUpdate(float fixedDt)
{
    if (State* s = GetCurrentState())
        s->OnFixedUpdate(fixedDt);
}

void StateMachine::Render()
{
    if (State* s = GetCurrentState())
        s->OnRender();
}
