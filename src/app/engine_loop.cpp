#include <chrono>
#include <thread>

#include <app/engine_loop.h>
#include <app/application.h>
#include <app/system_manager.h>
#include <app/io_handler.h>
#include <state/state_machine.h>
#include <scene/scene.h>
#include <app/monitor_manager.h>
#include <interface/graphic/i_graphics_context.h>
#include <resource/resource_manager.h>
#include <input/input_manager.h>

#include <utils/logger.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <debug/debug_system.h>
#endif

#include <iostream>

EngineLoop::EngineLoop(Application* app)
    : m_App(app)
    , m_LastFrameTime(std::chrono::steady_clock::now())
{
}

EngineLoop::~EngineLoop()
{
}

void EngineLoop::Run()
{
    LOGGER_INFO("EngineLoop") << "Starting engine loop";
    while (!m_App->GetWindow()->ShouldClose())
    {
        ProcessFrame();
    }
}

void EngineLoop::ProcessFrame()
{
    auto now = std::chrono::steady_clock::now();
    realDeltaTime = std::chrono::duration<float>(now - m_LastFrameTime).count();
    deltaTime = realDeltaTime;
    m_LastFrameTime = now;

    m_App->GetWindow()->PollEvents();
    m_App->GetMouse().Update();

    if (m_IsPaused)
    {
        deltaTime = 0.0f;
    }
    else
    {
        deltaTime *= m_TimeScale;
    }

    m_App->GetResourceManager().Update(realDeltaTime);
    m_App->GetIOHandler().ProcessInput();
    m_App->GetInputManager().Update();

#ifdef ENABLE_DEBUG_SYSTEM
    m_App->GetSystemManager().UpdateDebugSystem(realDeltaTime);
#endif

    m_App->GetSystemManager().UpdateLogic(
        m_App->GetScene(),
        deltaTime,
        realDeltaTime,
        m_App,
        m_App->GetMouse()
    );

    m_App->GetStateMachine().Update(deltaTime);
    m_App->GetMouse().EndFrame();

    FixedUpdate();

    m_App->GetSystemManager().UpdateVisuals(
        m_App->GetScene(),
        deltaTime,
        m_App->GetResourceManager(),
        m_App->GetSoundPlayer()
    );

    Render();

    m_App->GetWindow()->SwapBuffers();

    int frameRateLimit = m_App->GetMonitorManager().GetFrameRateLimit();
    if (frameRateLimit > 0)
    {
        double targetFrameTime = 1.0 / (double)frameRateLimit;
        auto frameEnd = std::chrono::steady_clock::now();
        double frameElapsed = std::chrono::duration<double>(frameEnd - now).count();
        if (frameElapsed < targetFrameTime)
        {
            double sleepTime = targetFrameTime - frameElapsed;
            if (sleepTime > 0.0)
            {

                if (sleepTime > 0.002)
                {
                    std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime - 0.001));
                }

                while (std::chrono::duration<double>(std::chrono::steady_clock::now() - now).count() < targetFrameTime)
                {
                    std::this_thread::yield();
                }
            }
        }
    }
}

void EngineLoop::FixedUpdate()
{
    m_Accumulator += deltaTime;

    int physicsSteps = 0;
    const int MAX_PHYSICS_STEPS = 5;

    while (m_Accumulator >= m_FixedDeltaTime && physicsSteps < MAX_PHYSICS_STEPS)
    {
        auto& systemMgr = m_App->GetSystemManager();
        systemMgr.FixedUpdateSystems(m_App->GetScene(), m_App->GetPhysicsWorld(), m_FixedDeltaTime);
        m_App->GetStateMachine().FixedUpdate(m_FixedDeltaTime);

        m_Accumulator -= m_FixedDeltaTime;
        physicsSteps++;
    }

    if (m_Accumulator > m_FixedDeltaTime)
    {
        m_Accumulator = 0.0f;
    }
}

void EngineLoop::Render()
{
    auto& systemMgr = m_App->GetSystemManager();

    systemMgr.RenderShadows(m_App->GetScene());

    systemMgr.RenderSystems(
        m_App->GetScene(),
        m_App->GetResourceManager(),
        m_App->GetWidth(),
        m_App->GetHeight()
    );

    m_App->GetStateMachine().Render();

#ifdef ENABLE_DEBUG_SYSTEM
    systemMgr.RenderDebugSystem(m_App->GetScene());
#endif
}

void EngineLoop::SetPhysicsStep(float step)
{
    if (step > 0.0f)
    {
        m_FixedDeltaTime = step;
        LOGGER_INFO("EngineLoop") << "Fixed delta time set to: " << m_FixedDeltaTime;
    }
}

void EngineLoop::SetTimeScale(float scale)
{
    m_TimeScale = scale;
    LOGGER_INFO("EngineLoop") << "Time scale set to: " << m_TimeScale;
}

void EngineLoop::SetPaused(bool paused)
{
    m_IsPaused = paused;
    LOGGER_INFO("EngineLoop") << (m_IsPaused ? "Engine paused" : "Engine resumed");
}
