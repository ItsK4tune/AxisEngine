#pragma once

#include <core/engine_context.h>
#include <chrono>

class EngineLoop
{
public:
    EngineLoop();
    ~EngineLoop();

    void Run();

    void Init(EngineContext ctx);
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
