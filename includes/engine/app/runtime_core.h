#pragma once

#include <memory>
#include <app/engine_loop.h>
#include <state/state_machine.h>

class RuntimeCore
{
public:
    RuntimeCore(Application* app);

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

    EngineLoop& GetEngineLoop() { return m_EngineLoop; }
    StateMachine& GetStateMachine() { return m_StateMachine; }

private:
    EngineLoop m_EngineLoop;
    StateMachine m_StateMachine;
};
