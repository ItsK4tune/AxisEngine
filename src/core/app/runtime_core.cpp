#include <core/app/runtime_core.h>
#include <core/logic/config_manager.h>
#include <core/app/state_machine.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_manager.h>

RuntimeCore::RuntimeCore()
{
}

void RuntimeCore::Initialize()
{
    auto& config = ServiceLocator::Instance().Require<ConfigManager>().GetConfig();
    
    // m_EngineLoop.Initialize() will set physics parameters from AppConfig
    
    m_EngineLoop.Initialize();
    m_StateMachine.Initialize();

    LOGGER_INFO("RuntimeCore") << "Initialized";
}

void RuntimeCore::Shutdown()
{
    m_StateMachine.Shutdown();
    m_EngineLoop.Shutdown();
    LOGGER_INFO("RuntimeCore") << "Shutdown";
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
    return ServiceLocator::Instance().Require<ConfigManager>().GetConfig();
}

#include <core/logic/event_system.h>
#include <core/type/event_types.h>

void RuntimeCore::ApplyConfig(const AppConfig& config)
{
    ServiceLocator::Instance().Require<ConfigManager>().UpdateConfig(config);
    EventSystem::Instance().Publish(ConfigChangedEvent{config});
}
