#include <chrono>
#include <thread>
#include <algorithm>

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
#include <scene/scene_manager.h>

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

    uint32_t mask = m_App->GetStateMachine().GetSystemMask();

    m_App->GetSystemManager().UpdateLogic(
        m_App->GetScene(),
        deltaTime,
        realDeltaTime,
        m_App,
        m_App->GetMouse(),
        mask
    );

    m_App->GetStateMachine().Update(deltaTime);
    m_App->GetMouse().EndFrame();

    FixedUpdate();

    m_App->GetSystemManager().UpdateVisuals(
        m_App->GetScene(),
        deltaTime,
        m_App->GetResourceManager(),
        m_App->GetSoundPlayer(),
        mask
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

    if (m_App->GetSceneManager().HasPendingScene())
    {
        m_App->GetSceneManager().UpdatePendingScene();
    }
}

void EngineLoop::FixedUpdate()
{
    m_Accumulator += deltaTime;

    const int MAX_PHYSICS_STEPS = 5;
    int physicsSteps = 0;

    uint32_t mask = m_App->GetStateMachine().GetSystemMask();
    entt::registry& registry = m_App->GetScene().registry;

    if (m_MaxForceSync)
    {
        auto view = registry.view<TransformComponent>();
        for (auto entity : view)
        {
            auto& t = view.get<TransformComponent>(entity);
            t.prevPosition = t.position;
            t.prevRotation = t.rotation;
            t.prevScale = t.scale;
        }
        m_MaxForceSync = false;
    }

    while (m_Accumulator >= m_FixedDeltaTime && physicsSteps < MAX_PHYSICS_STEPS)
    {
        // Backup states for interpolation
        auto view = registry.view<TransformComponent>();
        for (auto entity : view)
        {
            auto& transform = view.get<TransformComponent>(entity);
            transform.prevPosition = transform.position;
            transform.prevRotation = transform.rotation;
            transform.prevScale = transform.scale;
        }

        auto& systemMgr = m_App->GetSystemManager();
        systemMgr.FixedUpdateSystems(m_App->GetScene(), m_App->GetPhysicsWorld(), m_FixedDeltaTime, mask);
        m_App->GetStateMachine().FixedUpdate(m_FixedDeltaTime);

        m_Accumulator -= m_FixedDeltaTime;
        physicsSteps++;
    }

    if (m_Accumulator > m_FixedDeltaTime)
    {
        m_Accumulator = m_FixedDeltaTime;
    }

    m_Alpha = m_Accumulator / m_FixedDeltaTime;

    m_App->GetSystemManager().UpdateVisuals(
        m_App->GetScene(),
        deltaTime,
        m_App->GetResourceManager(),
        m_App->GetSoundPlayer(),
        m_Alpha, // Use member
        mask
    );
}

void EngineLoop::Render()
{
    auto& systemMgr = m_App->GetSystemManager();
    uint32_t mask = m_App->GetStateMachine().GetSystemMask();

    systemMgr.RenderShadows(m_App->GetScene(), m_Alpha, mask);

    systemMgr.RenderSystems(
        m_App->GetScene(),
        m_App->GetResourceManager(),
        m_App->GetWidth(),
        m_App->GetHeight(),
        m_Alpha,
        mask
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
