#pragma once

#include <core/unit/engine_context.h>
#include <core/logic/config_loader.h>
#include <core/logic/state_management.h>
#include <core/unit/engine_accessor.h>
#include <platform/interface/cursor_mode.h>
#include <chrono>
#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declarations
struct SceneRecord;

// --- Engine Loop ---

class EngineLoop
{
public:
    EngineLoop();
    ~EngineLoop();

    void Run();

    void Initialize(EngineContext ctx);
    void Shutdown();

    void SetPhysicsStep(float step);
    void SetTimeScale(float scale);
    void SetPaused(bool paused);

    float GetTimeScale() const { return m_TimeScale; }
    float GetRealDeltaTime() const { return m_RealDeltaTime; }
    bool IsPaused() const { return m_IsPaused; }

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
    bool m_MaxForceSync = true;
    float m_Alpha = 0.0f;

    EngineContext m_Ctx;
};

// --- Runtime Core ---

class RuntimeCore
{
public:
    RuntimeCore();
    ~RuntimeCore() = default;

    void Initialize(EngineContext ctx, const AppConfig& config, std::function<void(const AppConfig&)> applyFn);
    void Shutdown();

    void Run();

    void PushState(std::unique_ptr<class State> state);
    void PopState();
    void ChangeState(std::unique_ptr<class State> state);
    class State* GetCurrentState();

    void SetTimeScale(float scale);
    void SetPaused(bool paused);
    void SetPhysicsStep(float step);

    float GetTimeScale() const;
    float GetRealDeltaTime() const;
    bool IsPaused() const;

    const AppConfig& GetConfig() const;
    void ApplyConfig(const AppConfig& config);

    EngineLoop& GetEngineLoop() { return m_EngineLoop; }
    StateMachine& GetStateMachine() { return m_StateMachine; }

private:
    EngineLoop m_EngineLoop;
    StateMachine m_StateMachine;
    AppConfig m_Config;
    std::function<void(const AppConfig&)> m_ApplyConfigFn;
};
