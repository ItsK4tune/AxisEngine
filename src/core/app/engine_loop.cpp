#include <audio/logic/audio_service.h>
#include <core/app/runtime_core.h>
#include <audio/interface/i_audio_capture_service.h>
#include <core/app/state_machine.h>
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <core/logic/runtime_profiler.h>
#include <core/logic/service_locator.h>
#include <core/logic/time_service.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_manager.h>
#include <platform/interface/i_window.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <render/interface/i_graphics_context.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <algorithm>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif

EngineLoop::EngineLoop() : m_LastFrameTime(std::chrono::steady_clock::now())
{
}

void EngineLoop::Initialize()
{
#ifdef _WIN32
    if (!m_TimerResolutionEnabled)
        m_TimerResolutionEnabled = timeBeginPeriod(1) == TIMERR_NOERROR;
#endif

    auto& sl = ServiceLocator::Instance();
    auto& configMgr = sl.Require<ConfigManager>();
    auto appConfig = configMgr.GetConfig();

    SetPhysicsStep(1.0f / (std::max)(appConfig.physics.physicsTickRate, 1.0f));
    SetMaxSubSteps(appConfig.physics.maxSubSteps);
    SetTimeScale(appConfig.timeScale);

    m_ConfigSubscriptionId =
        EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
            SetPhysicsStep(1.0f / (std::max)(e.config.physics.physicsTickRate, 1.0f));
            SetMaxSubSteps(e.config.physics.maxSubSteps);
            SetTimeScale(e.config.timeScale);
        });
}

void EngineLoop::Shutdown()
{
#ifdef _WIN32
    if (m_TimerResolutionEnabled)
    {
        timeEndPeriod(1);
        m_TimerResolutionEnabled = false;
    }
#endif

    if (m_ConfigSubscriptionId != -1)
    {
        EventManager::Instance().Unsubscribe<ConfigChangedEvent>(m_ConfigSubscriptionId);
        m_ConfigSubscriptionId = -1;
    }
}

EngineLoop::~EngineLoop()
{
}

void EngineLoop::Run()
{
    LOGGER_INFO("EngineLoop") << "Starting engine loop";
    m_IsRunning = true;
    m_LastFrameTime = std::chrono::steady_clock::now();
    m_Accumulator = 0.0f;

    auto& sl = ServiceLocator::Instance();
    auto ioHandler = sl.Resolve<IOHandler>();

    while (m_IsRunning)
    {
        if (ioHandler)
        {
            auto window = ioHandler->GetMonitorManager().GetWindow();
            if (window && window->ShouldClose())
            {
                m_IsRunning = false;
                break;
            }
        }

        ProcessFrame();
    }
}

void EngineLoop::ProcessFrame()
{
    const auto profileStart = std::chrono::steady_clock::now();
    auto& profiler = RuntimeProfiler::Instance();
    profiler.BeginFrame();
    const auto elapsedMs = [](const auto& start) {
        return std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - start).count();
    };

    auto& sl = ServiceLocator::Instance();
    auto ioHandler = sl.Resolve<IOHandler>();
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

    auto stageStart = std::chrono::steady_clock::now();
    if (ioHandler)
    {
        auto window = ioHandler->GetMonitorManager().GetWindow();
        if (window)
        {
            window->PollEvents();
        }
        ioHandler->GetMouse().Update();
    }
    profiler.AddPassTime(ProfiledRenderPass::Input, elapsedMs(stageStart));

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

    stageStart = std::chrono::steady_clock::now();
    resourceManager.Update(m_RealDeltaTime);
    if (auto* audioService = sl.Resolve<AudioService>())
        audioService->UpdatePulses(m_RealDeltaTime);
    if (auto* audioCapture = sl.Resolve<IAudioCaptureService>())
        audioCapture->Update(m_RealDeltaTime);
    profiler.AddPassTime(ProfiledRenderPass::ResourceUpdate, elapsedMs(stageStart));

    stageStart = std::chrono::steady_clock::now();
    if (ioHandler)
    {
        ioHandler->ProcessInput();
        ioHandler->GetInputManager().Update();
    }
    profiler.AddPassTime(ProfiledRenderPass::Input, elapsedMs(stageStart));

    auto& stateMachine = runtimeCore.GetStateMachine();
    stageStart = std::chrono::steady_clock::now();
    systemManager.Update(scene, m_DeltaTime);

    stateMachine.Update(m_DeltaTime);
    profiler.AddPassTime(ProfiledRenderPass::GameUpdate, elapsedMs(stageStart));

    if (ioHandler)
    {
        ioHandler->GetMouse().EndFrame();
    }

    stageStart = std::chrono::steady_clock::now();
    FixedUpdate();
    profiler.AddPassTime(ProfiledRenderPass::FixedUpdate, elapsedMs(stageStart));
    Render();

    if (ioHandler)
    {
        auto window = ioHandler->GetMonitorManager().GetWindow();
        if (window)
        {
            stageStart = std::chrono::steady_clock::now();
            window->SwapBuffers();
            profiler.AddPassTime(ProfiledRenderPass::Swap, elapsedMs(stageStart));

            int frameRateLimit = ioHandler->GetMonitorManager().GetFrameRateLimit();
            if (frameRateLimit > 0)
            {
                stageStart = std::chrono::steady_clock::now();
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

                        while (std::chrono::duration<double>(std::chrono::steady_clock::now() - now).count() <
                               targetFrameTime)
                        {
                            std::this_thread::yield();
                        }
                    }
                }
                profiler.AddPassTime(ProfiledRenderPass::FrameLimiter, elapsedMs(stageStart));
            }
        }
    }

    if (sceneManager.HasPendingScene())
    {
        sceneManager.UpdatePendingScene();
    }

    const float fullFrameMs = elapsedMs(profileStart);
    profiler.SetCpuFrameTime(fullFrameMs);
    profiler.SetPassTime(ProfiledRenderPass::TotalFrame, fullFrameMs);
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
    if (!sl.Has<IGraphicsContext>())
        return;

    auto& sysMgr = sl.Require<SystemManager>();
    auto io = sl.Resolve<IOHandler>();
    auto& rtCore = sl.Require<RuntimeCore>();
    auto& scene = sl.Require<Scene>();

    int width = 0, height = 0;
    if (io)
    {
        width = io->GetMonitorManager().GetWidth();
        height = io->GetMonitorManager().GetHeight();
    }

    EventManager::Instance().Publish(RenderFrameBeginEvent{m_DeltaTime});
    sysMgr.Render(scene, width, height, m_Alpha);
    rtCore.GetStateMachine().Render();
    rtCore.GetStateMachine().RenderDebug();
    sysMgr.RenderDebug(scene);
    EventManager::Instance().Publish(RenderFrameEndEvent{});
}

void EngineLoop::SetPhysicsStep(float step)
{
    if (step > 0.0f && std::abs(m_FixedDeltaTime - step) > 0.0001f)
    {
        m_FixedDeltaTime = step;
        LOGGER_DEBUG("EngineLoop") << "Fixed delta time set to: " << m_FixedDeltaTime;
    }
}

void EngineLoop::SetMaxSubSteps(int steps)
{
    m_MaxSubSteps = std::clamp(steps, 1, 128);
}

void EngineLoop::SetTimeScale(float scale)
{
    if (!std::isfinite(scale))
        scale = 1.0f;
    scale = std::clamp(scale, 0.0f, 100.0f);
    if (std::abs(m_TimeScale - scale) > 0.0001f)
    {
        m_TimeScale = scale;
        LOGGER_DEBUG("EngineLoop") << "Time scale set to: " << m_TimeScale;
    }

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
