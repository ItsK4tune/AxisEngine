#include <chrono>
#include <thread>
#include <algorithm>

#include <core/engine_loop.h>
#include <core/engine_context.h>
#include <core/system_manager.h>
#include <core/runtime_core.h>
#include <systems/window/io_handler.h>
#include <systems/window/monitor_manager.h>
#include <core/state/state_machine.h>
#include <scene/scene.h>
#include <scene/scene_manager.h>
#include <resource/resource_manager.h>
#include <systems/input/input_manager.h>
#include <systems/audio/sound_player.h>
#include <rendering/interfaces/i_graphics_context.h>
#include <systems/window/interfaces/i_window.h>

#include <core/utils/logger.h>


#include <iostream>

EngineLoop::EngineLoop()
    : m_LastFrameTime(std::chrono::steady_clock::now())
{
}

void EngineLoop::Init(EngineContext ctx)
{
    m_Ctx = ctx;
}

void EngineLoop::Shutdown()
{
}

EngineLoop::~EngineLoop()
{
}

void EngineLoop::Run()
{
    LOGGER_INFO("EngineLoop") << "Starting engine loop";
    IWindow* window = m_Ctx.io->GetMonitorManager().GetWindow();
    while (!window->ShouldClose())
    {
        ProcessFrame();
    }
}

void EngineLoop::ProcessFrame()
{
    static int frameCounter = 0;
    frameCounter++;

    auto now = std::chrono::steady_clock::now();
    m_RealDeltaTime = std::chrono::duration<float>(now - m_LastFrameTime).count();
    m_DeltaTime = m_RealDeltaTime;
    m_LastFrameTime = now;

    IWindow* window = m_Ctx.io->GetMonitorManager().GetWindow();
    window->PollEvents();
    m_Ctx.io->GetMouse().Update();

    if (m_IsPaused)
    {
        m_DeltaTime = 0.0f;
    }
    else
    {
        m_DeltaTime *= m_TimeScale;
    }

    m_Ctx.resources->Update(m_RealDeltaTime);
    m_Ctx.io->ProcessInput();
    m_Ctx.io->GetInputManager().Update();

    m_Ctx.systems->UpdateDebugSystem(m_RealDeltaTime);

    auto& stateMachine = m_Ctx.runtime->GetStateMachine();
    m_Ctx.systems->RunUpdate(*m_Ctx.scene, m_DeltaTime);

    stateMachine.Update(m_DeltaTime);
    m_Ctx.io->GetMouse().EndFrame();

    FixedUpdate();
    Render();
    window->SwapBuffers();

    int frameRateLimit = m_Ctx.io->GetMonitorManager().GetFrameRateLimit();
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

    if (m_Ctx.sceneManager->HasPendingScene())
    {
        m_Ctx.sceneManager->UpdatePendingScene();
    }
}

void EngineLoop::FixedUpdate()
{
    m_Accumulator += m_DeltaTime;

    const int MAX_PHYSICS_STEPS = 5;
    int physicsSteps = 0;

    auto& stateMachine = m_Ctx.runtime->GetStateMachine();
    entt::registry& registry = m_Ctx.scene->registry;

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
        auto view = registry.view<TransformComponent>();
        for (auto entity : view)
        {
            auto& transform = view.get<TransformComponent>(entity);
            transform.prevPosition = transform.position;
            transform.prevRotation = transform.rotation;
            transform.prevScale = transform.scale;
        }

        m_Ctx.systems->RunFixedUpdate(*m_Ctx.scene, m_FixedDeltaTime);
        stateMachine.FixedUpdate(m_FixedDeltaTime);

        m_Accumulator -= m_FixedDeltaTime;
        physicsSteps++;
    }

    if (m_Accumulator > m_FixedDeltaTime)
    {
        m_Accumulator = m_FixedDeltaTime;
    }

    m_Alpha = m_Accumulator / m_FixedDeltaTime;

    // Visual update run alongside high priority logic now
}

void EngineLoop::Render()
{
    auto& systemMgr = *m_Ctx.systems;

    systemMgr.RenderShadows(*m_Ctx.scene, m_Alpha);

    systemMgr.RunRender(
        *m_Ctx.scene,
        m_Ctx.io->GetMonitorManager().GetWidth(),
        m_Ctx.io->GetMonitorManager().GetHeight(),
        m_Alpha
    );

    m_Ctx.runtime->GetStateMachine().Render();

    systemMgr.RenderDebugSystem(*m_Ctx.scene);
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
