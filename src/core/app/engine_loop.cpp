#include <core/app/runtime_core.h>
#include <core/logic/service_locator.h>
#include <ecs/logic/system_manager.h>
#include <core/logic/config_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <core/logic/event_system.h>
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
    m_IOHandler = &sl.Require<IOHandler>();
    m_Scene = &sl.Require<Scene>();
    m_ResourceManager = &sl.Require<ResourceManager>();
    m_SystemManager = &sl.Require<SystemManager>();
    m_SceneManager = &sl.Require<SceneManager>();
    m_RuntimeCore = &sl.Require<RuntimeCore>();
    m_TimeService = &sl.Require<TimeService>();
    m_Window = m_IOHandler->GetMonitorManager().GetWindow();

    auto& configMgr = sl.Require<ConfigManager>();
    auto& appConfig = configMgr.GetConfig();
    
    SetPhysicsStep(1.0f / appConfig.physicsTickRate);
    SetMaxSubSteps(appConfig.maxSubSteps);
    SetTimeScale(appConfig.timeScale);

    m_ConfigSubId = EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        SetPhysicsStep(1.0f / e.config.physicsTickRate);
        SetMaxSubSteps(e.config.maxSubSteps);
        SetTimeScale(e.config.timeScale);
    });
}

void EngineLoop::Shutdown()
{
    if (m_ConfigSubId != -1) {
        EventSystem::Instance().Unsubscribe<ConfigChangedEvent>(m_ConfigSubId);
    }
}

EngineLoop::~EngineLoop()
{
}

void EngineLoop::Run()
{
    LOGGER_INFO("EngineLoop") << "Starting engine loop";
    while (!m_Window->ShouldClose())
    {
        ProcessFrame();
    }
}

void EngineLoop::ProcessFrame()
{
    auto now = std::chrono::steady_clock::now();
    m_RealDeltaTime = std::chrono::duration<float>(now - m_LastFrameTime).count();
    m_DeltaTime = m_RealDeltaTime;
    m_LastFrameTime = now;

    m_Window->PollEvents();
    m_IOHandler->GetMouse().Update();

    if (m_IsPaused)
    {
        m_DeltaTime = 0.0f;
    }
    else
    {
        m_DeltaTime *= m_TimeScale;
    }

    m_TotalTime += m_RealDeltaTime;
    m_TimeService->SetTimeData(m_DeltaTime, m_RealDeltaTime, m_TotalTime);

    m_ResourceManager->Update(m_RealDeltaTime);
    m_IOHandler->ProcessInput();
    m_IOHandler->GetInputManager().Update();

    m_SystemManager->UpdateDebugSystem(m_RealDeltaTime);

    auto& stateMachine = m_RuntimeCore->GetStateMachine();
    m_SystemManager->RunUpdate(*m_Scene, m_DeltaTime);

    stateMachine.Update(m_DeltaTime);
    m_IOHandler->GetMouse().EndFrame();

    FixedUpdate();
    Render();
    m_Window->SwapBuffers();

    int frameRateLimit = m_IOHandler->GetMonitorManager().GetFrameRateLimit();
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

    if (m_SceneManager->HasPendingScene())
    {
        m_SceneManager->UpdatePendingScene();
    }
}

void EngineLoop::FixedUpdate()
{
    m_Accumulator += m_DeltaTime;
    int physicsSteps = 0;

    auto& stateMachine = m_RuntimeCore->GetStateMachine();

    while (m_Accumulator >= m_FixedDeltaTime && physicsSteps < m_MaxSubSteps)
    {
        m_SystemManager->RunFixedUpdate(*m_Scene, m_FixedDeltaTime);
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
    m_SystemManager->PerformRenderShadows(
        *m_Scene,
        m_IOHandler->GetMonitorManager().GetWidth(),
        m_IOHandler->GetMonitorManager().GetHeight(),
        m_Alpha
    );

    m_SystemManager->RunRender(
        *m_Scene,
        m_IOHandler->GetMonitorManager().GetWidth(),
        m_IOHandler->GetMonitorManager().GetHeight(),
        m_Alpha
    );

    m_RuntimeCore->GetStateMachine().Render();
    m_SystemManager->RenderDebugSystem(*m_Scene);
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

    if (m_TimeService)
    {
        m_TimeService->SetTimeScale(scale);
    }
}

void EngineLoop::SetPaused(bool paused)
{
    m_IsPaused = paused;
    LOGGER_INFO("EngineLoop") << (m_IsPaused ? "Engine paused" : "Engine resumed");

    if (m_TimeService)
    {
        m_TimeService->SetPaused(paused);
    }
}
