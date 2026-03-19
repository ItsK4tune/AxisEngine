#include <ecs/unit/core_components.h>
#include <chrono>
#include <thread>
#include <algorithm>

#include <core/logic/runtime_core.h>
#include <core/logic/service_locator.h>
#include <core/logic/system_manager.h>
#include <core/type/app_config.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <core/logic/state_machine.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <resource/logic/resource_manager.h>
#include <platform/logic/input_manager.h>
#include <audio/logic/audio_service.h>
#include <render/interface/i_graphics_context.h>
#include <platform/interface/i_window.h>
#include <core/logic/logger.h>

EngineLoop::EngineLoop()
    : m_LastFrameTime(std::chrono::steady_clock::now())
{
}

void EngineLoop::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto& appConfig = sl.Require<AppConfig>();
    
    SetPhysicsStep(1.0f / appConfig.physicsTickRate);
    SetMaxSubSteps(appConfig.maxSubSteps);
    // Systems are created and initialized by Application to avoid double-call during refactor.
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
    IWindow* window = ServiceLocator::Instance().Require<IOHandler>().GetMonitorManager().GetWindow();
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

    auto& sl = ServiceLocator::Instance();
    auto& io = sl.Require<IOHandler>();
    auto& scene = sl.Require<Scene>();
    auto& res = sl.Require<ResourceManager>();
    auto& systems = sl.Require<SystemManager>();
    auto& sm = sl.Require<SceneManager>();
    auto& runtime = sl.Require<RuntimeCore>();

    IWindow* window = io.GetMonitorManager().GetWindow();
    window->PollEvents();
    io.GetMouse().Update();

    if (m_IsPaused)
    {
        m_DeltaTime = 0.0f;
    }
    else
    {
        m_DeltaTime *= m_TimeScale;
    }

    res.Update(m_RealDeltaTime);
    io.ProcessInput();
    io.GetInputManager().Update();

    systems.UpdateDebugSystem(m_RealDeltaTime);

    auto& stateMachine = runtime.GetStateMachine();
    systems.RunUpdate(scene, m_DeltaTime);

    stateMachine.Update(m_DeltaTime);
    io.GetMouse().EndFrame();

    FixedUpdate();
    Render();
    window->SwapBuffers();

    int frameRateLimit = io.GetMonitorManager().GetFrameRateLimit();
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

    if (sm.HasPendingScene())
    {
        sm.UpdatePendingScene();
    }
}

void EngineLoop::FixedUpdate()
{
    auto& sl = ServiceLocator::Instance();
    auto& scene = sl.Require<Scene>();
    auto& systems = sl.Require<SystemManager>();
    auto& runtime = sl.Require<RuntimeCore>();

    m_Accumulator += m_DeltaTime;

    int physicsSteps = 0;

    auto& stateMachine = runtime.GetStateMachine();
    entt::registry& registry = scene.registry;

    if (m_MaxForceSync)
    {
        auto view = registry.view<PositionComponent, RotationComponent, ScaleComponent>();
        for (auto entity : view)
        {
            auto [p, r, s] = view.get<PositionComponent, RotationComponent, ScaleComponent>(entity);
            p.prev = p.value;
            r.prev = r.value;
            s.prev = s.value;
        }
        m_MaxForceSync = false;
    }

    while (m_Accumulator >= m_FixedDeltaTime && physicsSteps < m_MaxSubSteps)
    {
        auto view = registry.view<PositionComponent, RotationComponent, ScaleComponent>();
        for (auto entity : view)
        {
            auto [p, r, s] = view.get<PositionComponent, RotationComponent, ScaleComponent>(entity);
            p.prev = p.value;
            r.prev = r.value;
            s.prev = s.value;
        }

        systems.RunFixedUpdate(scene, m_FixedDeltaTime);
        stateMachine.FixedUpdate(m_FixedDeltaTime);

        m_Accumulator -= m_FixedDeltaTime;
        physicsSteps++;
    }

    if (m_Accumulator > m_FixedDeltaTime)
    {
        m_Accumulator = m_FixedDeltaTime;
    }

    m_Alpha = m_Accumulator / m_FixedDeltaTime;
}

void EngineLoop::Render()
{
    auto& sl = ServiceLocator::Instance();
    auto& scene = sl.Require<Scene>();
    auto& systems = sl.Require<SystemManager>();
    auto& io = sl.Require<IOHandler>();
    auto& runtime = sl.Require<RuntimeCore>();

    systems.RenderShadows(scene, m_Alpha);

    systems.RunRender(
        scene,
        io.GetMonitorManager().GetWidth(),
        io.GetMonitorManager().GetHeight(),
        m_Alpha
    );

    runtime.GetStateMachine().Render();

    systems.RenderDebugSystem(scene);
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
