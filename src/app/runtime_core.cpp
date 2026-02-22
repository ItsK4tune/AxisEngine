#include <app/runtime_core.h>

RuntimeCore::RuntimeCore(std::shared_ptr<Application> app)
    : m_EngineLoop(app)
    , m_StateMachine(app)
{
}

void RuntimeCore::Run()
{
    m_EngineLoop.Run();
}

void RuntimeCore::PushState(std::unique_ptr<State> state)
{
    m_StateMachine.PushState(std::move(state));
}

void RuntimeCore::PopState()
{
    m_StateMachine.PopState();
}

void RuntimeCore::ChangeState(std::unique_ptr<State> state)
{
    m_StateMachine.ChangeState(std::move(state));
}

State* RuntimeCore::GetCurrentState()
{
    return m_StateMachine.GetCurrentState();
}

void RuntimeCore::SetTimeScale(float scale)
{
    m_EngineLoop.SetTimeScale(scale);
}

void RuntimeCore::SetPaused(bool paused)
{
    m_EngineLoop.SetPaused(paused);
}

void RuntimeCore::SetPhysicsStep(float step)
{
    m_EngineLoop.SetPhysicsStep(step);
}

float RuntimeCore::GetTimeScale() const
{
    return m_EngineLoop.GetTimeScale();
}

float RuntimeCore::GetRealDeltaTime() const
{
    return m_EngineLoop.GetRealDeltaTime();
}

bool RuntimeCore::IsPaused() const
{
    return m_EngineLoop.IsPaused();
}
