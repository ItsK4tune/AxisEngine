#include <core/app/runtime_core.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_manager.h>
#include <core/logic/config_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <core/app/state_machine.h>
#include <core/logic/time_service.h>
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
    auto& configMgr = sl.Require<ConfigManager>();
    auto& appConfig = configMgr.GetConfig();
    
    SetPhysicsStep(1.0f / appConfig.physicsTickRate);
    SetMaxSubSteps(appConfig.maxSubSteps);
    SetTimeScale(appConfig.timeScale);

    m_ConfigSubscriptionId = EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        SetPhysicsStep(1.0f / e.config.physicsTickRate);
        SetMaxSubSteps(e.config.maxSubSteps);
        SetTimeScale(e.config.timeScale);
    });
}

void EngineLoop::Shutdown()
{
    if (m_ConfigSubscriptionId != -1) {
        EventManager::Instance().Unsubscribe<ConfigChangedEvent>(m_ConfigSubscriptionId);
    }
}

EngineLoop::~EngineLoop()
{
}

void EngineLoop::Run()
{
    LOGGER_INFO("EngineLoop") << "Starting engine loop";
    while (!ServiceLocator::Instance().Require<IOHandler>().GetMonitorManager().GetWindow()->ShouldClose())
    {
        ProcessFrame();
    }
}

void EngineLoop::ProcessFrame()
{
    auto& sl = ServiceLocator::Instance();
    auto& ioHandler = sl.Require<IOHandler>();
    auto& window = *ioHandler.GetMonitorManager().GetWindow();
    auto& timeService = sl.Require<TimeService>();
    auto& resourceManager = sl.Require<ResourceManager>();
    auto& systemManager = sl.Require<SystemManager>();
    auto& runtimeCore = sl.Require<RuntimeCore>();
    auto& scene = sl.Require<Scene>();
    auto& sceneManager = sl.Require<SceneManager>();

    auto now = std::chrono::steady_clock::now();
    m_RealDeltaTime = std::chrono::duration<float>(now - m_LastFrameTime).count();
    m_DeltaTime = m_RealDeltaTime;
    m_LastFrameTime = now;

    window.PollEvents();
    ioHandler.GetMouse().Update();

    if (m_IsPaused)
    {
        m_DeltaTime = 0.0f;
    }
    else
    {
        m_DeltaTime *= m_TimeScale;
    }

    m_TotalTime += m_RealDeltaTime;
    timeService.SetTimeData(m_DeltaTime, m_RealDeltaTime, m_TotalTime);

    resourceManager.Update(m_RealDeltaTime);
    ioHandler.ProcessInput();
    ioHandler.GetInputManager().Update();

    auto& stateMachine = runtimeCore.GetStateMachine();
    systemManager.Update(scene, m_DeltaTime);
    systemManager.UpdateDebug(m_RealDeltaTime);

    stateMachine.Update(m_DeltaTime);
    ioHandler.GetMouse().EndFrame();

    FixedUpdate();
    Render();
    window.SwapBuffers();

    int frameRateLimit = ioHandler.GetMonitorManager().GetFrameRateLimit();
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

    if (sceneManager.HasPendingScene())
    {
        sceneManager.UpdatePendingScene();
    }
}

void EngineLoop::FixedUpdate()
{
    auto& sl = ServiceLocator::Instance();
    auto& sysMgr = sl.Require<SystemManager>();
    auto& rtCore = sl.Require<RuntimeCore>();
    auto& scene = sl.Require<Scene>();

    m_Accumulator += m_DeltaTime;
    int physicsSteps = 0;

    auto& stateMachine = rtCore.GetStateMachine();

    while (m_Accumulator >= m_FixedDeltaTime && physicsSteps < m_MaxSubSteps)
    {
        sysMgr.FixedUpdate(scene, m_FixedDeltaTime);
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
    auto& sysMgr = sl.Require<SystemManager>();
    auto& io = sl.Require<IOHandler>();
    auto& rtCore = sl.Require<RuntimeCore>();
    auto& scene = sl.Require<Scene>();

    sysMgr.RenderShadows(
        scene,
        io.GetMonitorManager().GetWidth(),
        io.GetMonitorManager().GetHeight(),
        m_Alpha
    );

    sysMgr.Render(
        scene,
        io.GetMonitorManager().GetWidth(),
        io.GetMonitorManager().GetHeight(),
        m_Alpha
    );

    rtCore.GetStateMachine().Render();
    sysMgr.RenderDebug(scene);
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

    if (ServiceLocator::Instance().Has<TimeService>())
    {
        ServiceLocator::Instance().Require<TimeService>().SetTimeScale(scale);
    }
}

void EngineLoop::SetPaused(bool paused)
{
    m_IsPaused = paused;
    LOGGER_INFO("EngineLoop") << (m_IsPaused ? "Engine paused" : "Engine resumed");

    if (ServiceLocator::Instance().Has<TimeService>())
    {
        ServiceLocator::Instance().Require<TimeService>().SetPaused(paused);
    }
}
