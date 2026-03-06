#pragma once

#include <app/config_loader.h>
#include <core/engine_context.h>
#include <core/engine_loop.h>
#include <functional>
#include <memory>
#include <state/state_machine.h>

class RuntimeCore
{
public:
    RuntimeCore();
    ~RuntimeCore() = default;

    void Init(EngineContext ctx, const AppConfig& config, std::function<void(const AppConfig&)> applyFn);
    void Shutdown();

    void Run();

    void PushState(std::unique_ptr<State> state);
    void PopState();
    void ChangeState(std::unique_ptr<State> state);
    State* GetCurrentState();

    void SetTimeScale(float scale);
    void SetPaused(bool paused);
    void SetPhysicsStep(float step);

    float GetTimeScale() const;
    float GetRealDeltaTime() const;
    bool IsPaused() const;

    const class AppConfig& GetConfig() const;
    void ApplyConfig(const class AppConfig& config);


    EngineLoop& GetEngineLoop() { return m_EngineLoop; }
    StateMachine& GetStateMachine() { return m_StateMachine; }

private:
    EngineLoop m_EngineLoop;
    StateMachine m_StateMachine;
    AppConfig m_Config;
    std::function<void(const AppConfig&)> m_ApplyConfigFn;
};
