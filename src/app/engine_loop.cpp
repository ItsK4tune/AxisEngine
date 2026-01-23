#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <app/engine_loop.h>
#include <app/application.h>
#include <app/system_manager.h>
#include <state/state_machine.h>
#include <scene/scene.h>
#include <app/monitor_manager.h>
#include <app/app_handler.h>
#include <resource/resource_manager.h>
#include <audio/sound_manager.h>
#include <physic/physic_world.h>
#include <utils/logger.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <debug/debug_system.h>
#endif

#include <iostream>

EngineLoop::EngineLoop(Application* app)
    : m_App(app)
{
}

EngineLoop::~EngineLoop()
{
}

void EngineLoop::Run()
{
    LOGGER_INFO("EngineLoop") << "Starting engine loop";
    while (!glfwWindowShouldClose(m_App->GetWindow()))
    {
        ProcessFrame();
    }
}

void EngineLoop::ProcessFrame()
{
    float currentFrame = (float)glfwGetTime();
    realDeltaTime = currentFrame - lastFrame;
    deltaTime = realDeltaTime;
    lastFrame = currentFrame;

    glfwPollEvents();
    m_App->GetMouse().Update();

    if (m_IsPaused)
    {
        deltaTime = 0.0f;
    }
    else
    {
        deltaTime *= m_TimeScale;
    }

    m_App->GetResourceManager().Update();
    m_App->GetAppHandler().ProcessInput(m_App->GetWindow());

#ifdef ENABLE_DEBUG_SYSTEM
    m_App->GetSystemManager().UpdateDebugSystem(realDeltaTime);
#endif

    FixedUpdate();
    Update();
    Render();

    glfwSwapBuffers(m_App->GetWindow());

    int frameRateLimit = m_App->GetMonitorManager().GetFrameRateLimit();
    if (frameRateLimit > 0)
    {
        double targetFrameTime = 1.0 / (double)frameRateLimit;
        double frameEnd = glfwGetTime();
        double frameElapsed = frameEnd - currentFrame;

        while (frameElapsed < targetFrameTime)
        {
            frameEnd = glfwGetTime();
            frameElapsed = frameEnd - currentFrame;
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

void EngineLoop::Update()
{
    auto& systemMgr = m_App->GetSystemManager();
    
    systemMgr.UpdateSystems(
        m_App->GetScene(),
        deltaTime,
        realDeltaTime,
        m_App,
        m_App->GetResourceManager(),
        m_App->GetSoundManager(),
        m_App->GetMouse()
    );

    m_App->GetStateMachine().Update(deltaTime);
    m_App->GetMouse().EndFrame();
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
        LOGGER_DEBUG("EngineLoop") << "Fixed delta time set to: " << m_FixedDeltaTime;
    }
}

void EngineLoop::SetTimeScale(float scale)
{
    m_TimeScale = scale;
    LOGGER_DEBUG("EngineLoop") << "Time scale set to: " << m_TimeScale;
}

void EngineLoop::SetPaused(bool paused)
{
    m_IsPaused = paused;
    LOGGER_DEBUG("EngineLoop") << (m_IsPaused ? "Engine paused" : "Engine resumed");
}
