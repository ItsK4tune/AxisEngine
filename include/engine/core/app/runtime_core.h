#pragma once

#include <core/app/engine_accessor.h>
#include <core/app/state_machine.h>
#include <core/logic/config_loader.h>
#include <platform/interface/cursor_mode.h>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct SceneRecord;
class IOHandler;
class Scene;
class ResourceManager;
class SystemManager;
class SceneManager;
class RuntimeCore;
class TimeService;
class IWindow;

class EngineLoop
{
public:
    EngineLoop();
    ~EngineLoop();

    void Run();

    void Initialize();
    void Shutdown();

    void SetPhysicsStep(float step);
    void SetMaxSubSteps(int steps);
    void SetTimeScale(float scale);
    void SetPaused(bool paused);

    float GetTimeScale() const
    {
        return m_TimeScale;
    }
    float GetRealDeltaTime() const
    {
        return m_RealDeltaTime;
    }
    bool IsPaused() const
    {
        return m_IsPaused;
    }
    void Stop()
    {
        m_IsRunning = false;
    }

private:
    void ProcessFrame();
    void FixedUpdate();
    void Render();

    float m_DeltaTime = 0.0f;
    float m_RealDeltaTime = 0.0f;
    std::chrono::steady_clock::time_point m_LastFrameTime;
    float m_Accumulator = 0.0f;
    float m_FixedDeltaTime = 1.0f / 60.0f;

    float m_TimeScale = 1.0f;
    bool m_IsPaused = false;
    float m_Alpha = 0.0f;
    float m_TotalTime = 0.0f;
    int m_MaxSubSteps = 10;
    int m_ConfigSubscriptionId = -1;
    bool m_IsRunning = false;
#ifdef _WIN32
    bool m_TimerResolutionEnabled = false;
#endif
};

class RuntimeCore
{
public:
    RuntimeCore();
    ~RuntimeCore() = default;

    void Initialize();
    void Shutdown();

    void Run();

    void PushState(std::unique_ptr<class State> state);
    void PopState();
    void ChangeState(std::unique_ptr<class State> state);
    class State* GetCurrentState();

    EngineLoop& GetEngineLoop()
    {
        return m_EngineLoop;
    }
    StateMachine& GetStateMachine()
    {
        return m_StateMachine;
    }

private:
    EngineLoop m_EngineLoop;
    StateMachine m_StateMachine;
};
