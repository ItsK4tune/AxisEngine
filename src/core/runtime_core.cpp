#include <app/config_loader.h>
#include <core/engine_context.h>
#include <core/runtime_core.h>

RuntimeCore::RuntimeCore()
{
}

void RuntimeCore::Init(EngineContext ctx, const AppConfig& config, std::function<void(const AppConfig&)> applyFn)
{
    m_Config = config;
    m_ApplyConfigFn = std::move(applyFn);
    m_EngineLoop.Init(ctx);
    m_StateMachine.Init(ctx);
}

void RuntimeCore::Shutdown()
{
    m_StateMachine.Shutdown();
    m_EngineLoop.Shutdown();
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

const AppConfig& RuntimeCore::GetConfig() const
{
    return m_Config;
}

void RuntimeCore::ApplyConfig(const AppConfig& config)
{
    m_Config = config;
    if (m_ApplyConfigFn)
        m_ApplyConfigFn(config);
}
