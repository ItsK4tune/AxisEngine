#pragma once

#include <GLFW/glfw3.h>

class Application;
class SystemManager;
class StateMachine;

class EngineLoop
{
public:
    EngineLoop(Application* app);
    ~EngineLoop();

    void Run();

    void SetPhysicsStep(float step);
    void SetTimeScale(float scale);
    void SetPaused(bool paused);
    
    float GetTimeScale() const { return m_TimeScale; }
    float GetRealDeltaTime() const { return realDeltaTime; }
    bool IsPaused() const { return m_IsPaused; }

private:
    void ProcessFrame();
    void FixedUpdate();
    void Update();
    void Render();

    Application* m_App;

    float deltaTime = 0.0f;
    float realDeltaTime = 0.0f;
    float lastFrame = 0.0f;
    float m_Accumulator = 0.0f;
    float m_FixedDeltaTime = 1.0f / 60.0f;

    float m_TimeScale = 1.0f;
    bool m_IsPaused = false;
};
