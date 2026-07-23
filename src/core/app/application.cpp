#include <core/app/application.h>
#include <core/app/app_builder.h>
#include <core/app/runtime_core.h>
#include <audio/interface/i_audio_capture_service.h>
#include <core/logic/data_manager.h>
#include <audio/logic/audio_service.h>
#include <core/logic/config_manager.h>
#include <core/logic/config_validation.h>
#include <core/logic/config_serializer.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/job_system.h>
#include <core/logic/log_manager.h>
#include <core/logic/logger.h>
#include <core/logic/runtime_profiler.h>
#include <core/logic/service_locator.h>
#include <core/logic/time_service.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <engine/platform/logic/io_handler.h>
#include <physics/interface/i_physics_world.h>
#include <physics/logic/collision_matrix.h>
#include <platform/interface/i_window.h>
#include <platform/interface/i_keyboard_input_router.h>
#include <platform/logic/input_serializer.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/monitor_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/logic/post_process_pipeline.h>
#include <render/logic/render_service_impl.h>
#include <render/logic/renderer_initializer.h>
#include <render/logic/transient_buffer_ring.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <scene/logic/component_codec_registry.h>
#include <script/logic/script_registry.h>
#include <csignal>
#include <atomic>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>

static std::atomic<Application*> s_Instance = nullptr;

namespace
{
class InitializationRollback
{
public:
    explicit InitializationRollback(std::function<void()> rollback) : m_Rollback(std::move(rollback))
    {
    }
    ~InitializationRollback()
    {
        if (!m_Committed)
            m_Rollback();
    }
    void Commit()
    {
        m_Committed = true;
    }

private:
    std::function<void()> m_Rollback;
    bool m_Committed = false;
};

AudioCaptureSettings MakeAudioCaptureSettings(const AudioConfig& config)
{
    AudioCaptureSettings settings;
    settings.inputVolume = config.captureInputVolume;
    settings.noiseGate = config.captureNoiseGate;
    settings.gain = config.captureGain;
    settings.attackSeconds = config.captureAttackSeconds;
    settings.releaseSeconds = config.captureReleaseSeconds;
    settings.peakDecaySeconds = config.capturePeakDecaySeconds;
    settings.calibrationSeconds = config.captureCalibrationSeconds;
    settings.pulseThreshold = config.capturePulseThreshold;
    settings.pulseCooldown = config.capturePulseCooldown;
    settings.pulseDuration = config.capturePulseDuration;
    return settings;
}

void ApplyOptimizationConfig(ResourceManager* resources, SystemManager* systems, const OptimizationConfig& config)
{
    if (resources)
        resources->ApplyOptimizationConfig(config);
    if (systems)
        systems->ApplyOptimizationConfig(config);
}
}  // namespace

void Application::HandleQuitSignal(int)
{
    if (auto* instance = s_Instance.load(std::memory_order_relaxed))
        instance->GetRuntimeCore().GetEngineLoop().Stop();
}

#if defined(_WIN32)
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

struct Application::Impl
{
    explicit Impl(AppBuilder providers) : m_Providers(std::move(providers))
    {
    }

    AppBuilder m_Providers;
    ServiceLocator m_Services;
    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<IPhysicsWorld> m_PhysicsWorld;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<AudioService> m_AudioService;
    std::unique_ptr<IAudioCaptureService> m_AudioCaptureService;
    std::unique_ptr<RenderServiceImpl> m_RenderService;
    std::unique_ptr<SceneManager> m_SceneManager;

    std::unique_ptr<IOHandler> m_IOHandler;
    std::unique_ptr<IGraphicsContext> m_GraphicsContext;
    std::unique_ptr<TimeService> m_TimeService;
    std::unique_ptr<RuntimeCore> m_RuntimeCore;
    std::unique_ptr<SystemManager> m_SystemManager;
    std::unique_ptr<ConfigManager> m_ConfigManager;
    std::unique_ptr<ScriptRegistry> m_ScriptRegistry;
    std::unique_ptr<CollisionMatrix> m_CollisionMatrix;
    std::unique_ptr<DataManager> m_DataManager;
    std::unique_ptr<ComponentCodecRegistry> m_ComponentCodecs;
    std::string m_CaptureDeviceId;
    int m_ConfigSubId = -1;
    LogLevel m_PreviousLogLevel = LogLevel::None;
    bool m_OwnsLogManager = false;
    ApplicationLifecycle m_Lifecycle = ApplicationLifecycle::Created;
};

Application::Application() : Application(AppBuilder{})
{
}

Application::Application(AppBuilder providers) : m_Impl(std::make_unique<Impl>(std::move(providers)))
{
    m_Impl->m_Scene = std::make_unique<Scene>();
}

Application::~Application()
{
    Shutdown();
    if (m_Impl)
        ServiceLocator::ClearProcessDefault(&m_Impl->m_Services);
    Application* expected = this;
    s_Instance.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
}

void Application::Shutdown()
{
    if (!m_Impl)
        return;
    auto serviceActivation = m_Impl->m_Services.Activate();

    if (m_Impl->m_Lifecycle == ApplicationLifecycle::ShuttingDown ||
        m_Impl->m_Lifecycle == ApplicationLifecycle::Stopped)
        return;

    const bool initialized = m_Impl->m_Lifecycle == ApplicationLifecycle::Initialized ||
                             m_Impl->m_Lifecycle == ApplicationLifecycle::Running;
    if (!m_Impl->m_RuntimeCore && !m_Impl->m_ConfigManager && !m_Impl->m_ResourceManager)
    {
        m_Impl->m_Services.ClearAll();
        JobSystem::Instance().Shutdown();
        if (m_Impl->m_OwnsLogManager)
        {
            LogManager::Instance().Shutdown();
            LogManager::Instance().SetLogLevel(m_Impl->m_PreviousLogLevel);
            m_Impl->m_OwnsLogManager = false;
        }
        EventManager::Instance().Clear();
        RuntimeProfiler::Instance().Reset();
        m_Impl->m_Lifecycle = ApplicationLifecycle::Stopped;
        ServiceLocator::ClearProcessDefault(&m_Impl->m_Services);
        Application* expected = this;
        s_Instance.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
        return;
    }

    m_Impl->m_Lifecycle = ApplicationLifecycle::ShuttingDown;
    if (initialized)
        EventManager::Instance().Publish(EngineShutdownEvent{});

    LOGGER_INFO("Application") << "Shutting down application...";

    if (m_Impl->m_RuntimeCore)
        m_Impl->m_RuntimeCore->Shutdown();

    if (m_Impl->m_SystemManager)
        m_Impl->m_SystemManager->Reset();

    if (m_Impl->m_SystemManager)
    {
        m_Impl->m_SystemManager->Shutdown();
    }

    if (m_Impl->m_Scene)
    {
        try
        {
            m_Impl->m_Scene->GetRegistry().clear();
        }
        catch (...)
        {
            LOGGER_ERROR("Application") << "Destructor CRASH during registry.clear()";
        }
    }

    if (m_Impl->m_SceneManager)
    {
        m_Impl->m_SceneManager->Shutdown();
    }

    if (m_Impl->m_Scene)
    {
        m_Impl->m_Scene->ShutdownManagers();
    }

    if (m_Impl->m_ConfigSubId != -1)
    {
        EventManager::Instance().Unsubscribe<ConfigChangedEvent>(m_Impl->m_ConfigSubId);
        m_Impl->m_ConfigSubId = -1;
    }

    m_Impl->m_SystemManager.reset();
    m_Impl->m_RenderService.reset();
    m_Impl->m_SceneManager.reset();
    m_Impl->m_RuntimeCore.reset();
    if (m_Impl->m_AudioCaptureService)
        m_Impl->m_AudioCaptureService->Shutdown();

    // Resource managers retain references to the graphics and audio backends,
    // so release their assets before either backend is destroyed.
    if (m_Impl->m_ResourceManager)
        m_Impl->m_ResourceManager->Shutdown();

    // Explicit shutdown is complete. Remove service aliases before ownership
    // starts disappearing so no later callback can resolve a dangling object.
    ServiceLocator::Instance().ClearAll();

    m_Impl->m_AudioCaptureService.reset();
    m_Impl->m_DataManager.reset();
    m_Impl->m_ComponentCodecs.reset();
    m_Impl->m_Scene.reset();
    m_Impl->m_ResourceManager.reset();
    m_Impl->m_AudioService.reset();
    m_Impl->m_PhysicsWorld.reset();

    RendererInitializer::Shutdown();

    if (m_Impl->m_GraphicsContext)
        m_Impl->m_GraphicsContext->Shutdown();
    m_Impl->m_GraphicsContext.reset();
    m_Impl->m_IOHandler.reset();
    m_Impl->m_TimeService.reset();
    m_Impl->m_ScriptRegistry.reset();
    m_Impl->m_CollisionMatrix.reset();

    JobSystem::Instance().Shutdown();
    m_Impl->m_ConfigManager.reset();

    LOGGER_INFO("Application") << "Application shutdown completed.";
    if (m_Impl->m_OwnsLogManager)
    {
        LogManager::Instance().Shutdown();
        LogManager::Instance().SetLogLevel(m_Impl->m_PreviousLogLevel);
        m_Impl->m_OwnsLogManager = false;
    }
    EventManager::Instance().Clear();
    RuntimeProfiler::Instance().Reset();
    m_Impl->m_Lifecycle = ApplicationLifecycle::Stopped;
    ServiceLocator::ClearProcessDefault(&m_Impl->m_Services);
    Application* expected = this;
    s_Instance.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
}

bool Application::Initialize()
{
    AppConfig config;
    ConfigSerializer serializer(config.headlessMode);
    if (!serializer.Deserialize(FileSystem::getPath("config.axs"), config))
        serializer.Deserialize(FileSystem::getEngineAssetPath("config.axs"), config);
    return Initialize(config);
}

bool Application::Initialize(const AppConfig& incomingConfig)
{
    if (!m_Impl || m_Impl->m_Lifecycle != ApplicationLifecycle::Created)
    {
        LOGGER_WARN("Application") << "Initialize called outside the Created lifecycle state";
        return false;
    }
    Application* expectedApplication = nullptr;
    if (!s_Instance.compare_exchange_strong(expectedApplication, this, std::memory_order_acq_rel) &&
        expectedApplication != this)
    {
        LOGGER_ERROR("Application") << "Another Application is already active in this process";
        return false;
    }
    if (!ServiceLocator::SetProcessDefault(&m_Impl->m_Services))
    {
        LOGGER_ERROR("Application") << "Another application service context is already active in this process";
        Application* currentApplication = this;
        s_Instance.compare_exchange_strong(currentApplication, nullptr, std::memory_order_acq_rel);
        return false;
    }
    auto serviceActivation = m_Impl->m_Services.Activate();
    m_Impl->m_Lifecycle = ApplicationLifecycle::Initializing;
    InitializationRollback rollback([this] {
        m_Impl->m_Lifecycle = ApplicationLifecycle::Failed;
        Shutdown();
        m_Impl->m_Lifecycle = ApplicationLifecycle::Failed;
    });

    try
    {
        const auto providerCapabilities = m_Impl->m_Providers.GetCapabilities();
        const ConfigValidationPolicy validationPolicy{providerCapabilities.customGraphicsContext,
                                                      providerCapabilities.customPhysicsWorld,
                                                      providerCapabilities.customAudioEngine};
        ConfigValidationResult validation = ValidateAndSanitizeConfig(incomingConfig, validationPolicy);
        AppConfig config = validation.config;

        JobSystem::Instance().Initialize(config.numJobThreads);
        m_Impl->m_PreviousLogLevel = LogManager::Instance().GetLogLevel();
        m_Impl->m_OwnsLogManager = true;
        LogManager::Instance().Initialize(config.logLevel);
        for (const auto& issue : validation.issues) LOGGER_WARN("Config") << issue.field << ": " << issue.message;

        std::signal(SIGINT, HandleQuitSignal);
        std::signal(SIGTERM, HandleQuitSignal);

        m_Impl->m_ResourceManager = std::make_unique<ResourceManager>();
        m_Impl->m_SceneManager = std::make_unique<SceneManager>();
        m_Impl->m_RuntimeCore = std::make_unique<RuntimeCore>();
        m_Impl->m_SystemManager = std::make_unique<SystemManager>();
        m_Impl->m_ConfigManager = std::make_unique<ConfigManager>();
        m_Impl->m_ConfigManager->Initialize(config, validationPolicy);
        m_Impl->m_TimeService = std::make_unique<DefaultTimeService>();

        m_Impl->m_ScriptRegistry = std::make_unique<ScriptRegistry>();
        m_Impl->m_CollisionMatrix = std::make_unique<CollisionMatrix>();
        m_Impl->m_DataManager = std::make_unique<DataManager>();
        m_Impl->m_ComponentCodecs = std::make_unique<ComponentCodecRegistry>();

        const bool effectiveHeadless = config.headlessMode;

        if (!effectiveHeadless)
        {
            m_Impl->m_GraphicsContext = m_Impl->m_Providers.CreateGraphicsContext(config);
            auto audioEngine = m_Impl->m_Providers.CreateAudioEngine(config);
            auto window = m_Impl->m_Providers.MakeWindow();

            m_Impl->m_IOHandler = std::make_unique<IOHandler>();
            m_Impl->m_AudioService = std::make_unique<AudioService>();
            m_Impl->m_AudioService->Initialize(std::move(audioEngine));
            m_Impl->m_AudioCaptureService = m_Impl->m_Providers.CreateAudioCaptureService();
            const AudioCaptureSettings captureSettings = MakeAudioCaptureSettings(config.audio);
            if (m_Impl->m_AudioCaptureService->Initialize(captureSettings) && config.audio.captureEnabled)
            {
                const auto result = m_Impl->m_AudioCaptureService->Start(config.audio.captureDevice);
                if (result != AudioCaptureResult::Success)
                    LOGGER_WARN("Application")
                        << "Microphone capture could not start (result=" << static_cast<int>(result) << ")";
                else
                    m_Impl->m_CaptureDeviceId = config.audio.captureDevice;
            }

            if (!m_Impl->m_IOHandler->Initialize(std::move(window), config.title, config.window.width,
                                                 config.window.height, (int)config.window.windowMode,
                                                 config.window.monitorIndex, config.window.refreshRate,
                                                 config.window.vsync, config.window.frameRateLimit))
            {
                LOGGER_ERROR("Application") << "Failed to initialize IOHandler";
                m_Impl->m_Lifecycle = ApplicationLifecycle::Failed;
                return false;
            }
            m_Impl->m_IOHandler->GetInputManager().SetGamepadDeadZone(config.input.gamepadDeadZone);

            auto& mm = m_Impl->m_IOHandler->GetMonitorManager();
            m_Impl->m_ConfigManager->SetResolution(mm.GetWidth(), mm.GetHeight());

            if (!m_Impl->m_GraphicsContext->Initialize())
            {
                LOGGER_ERROR("Application") << "Failed to initialize graphics context";
                m_Impl->m_Lifecycle = ApplicationLifecycle::Failed;
                return false;
            }
            m_Impl->m_GraphicsContext->SetDepthTest(true);
            m_Impl->m_GraphicsContext->SetStateCacheEnabled(config.optimization.renderStateCacheEnabled);
            TransientBufferRing::SetGloballyEnabled(config.optimization.persistentMappedBuffersEnabled);

            auto& context = *m_Impl->m_GraphicsContext;
            RendererInitializer::Initialize(context);

            if (!config.culling.depthTestEnabled)
                context.SetDepthTest(false);
            context.SetCullFace(config.culling.cullFaceEnabled);

            auto* configMgr = m_Impl->m_ConfigManager.get();
            auto* ioHandler = m_Impl->m_IOHandler.get();
            auto* graphicsCtx = m_Impl->m_GraphicsContext.get();

            IWindow* appWindow = m_Impl->m_IOHandler->GetMonitorManager().GetWindow();
            appWindow->SetResizeCallback([configMgr, ioHandler, graphicsCtx](int width, int height) {
                if (width <= 0 || height <= 0)
                    return;
                EventManager::Instance().Publish(WindowResizedEvent{width, height});
                if (configMgr)
                    configMgr->SetResolution(width, height);
                if (ioHandler)
                    ioHandler->OnResize(width, height);
                if (graphicsCtx)
                    graphicsCtx->SetViewport(0, 0, width, height);
            });

            appWindow->SetCursorPosCallback([ioHandler](double x, double y) {
                if (ioHandler)
                    ioHandler->OnMouseMove(x, y);
            });

            appWindow->SetMouseButtonCallback([ioHandler](int button, int action, int mods) {
                if (ioHandler)
                    ioHandler->OnMouseButton(button, action, mods);
                if (action == 1)
                    EventManager::Instance().Publish(MouseButtonPressedEvent{button, mods});
                else if (action == 0)
                    EventManager::Instance().Publish(MouseButtonReleasedEvent{button, mods});
            });

            appWindow->SetScrollCallback([ioHandler](double x, double y) {
                if (ioHandler)
                    ioHandler->OnScroll(x, y);
                EventManager::Instance().Publish(MouseScrolledEvent{x, y});
            });

            appWindow->SetKeyCallback([ioHandler](int key, int scancode, int action, int mods) {
                const Key inputKey = static_cast<Key>(key);
                bool consumed = ioHandler && ioHandler->GetKeyboard().IsKeyConsumed(inputKey);
                if (action != 0 && !consumed)
                {
                    if (const auto* router = ServiceLocator::Instance().Resolve<IKeyboardInputRouter>();
                        router && router->ShouldConsumeKey(inputKey))
                    {
                        ioHandler->GetKeyboard().ConsumeKey(inputKey);
                        consumed = true;
                    }
                }
                if (action == 0 && consumed && ioHandler)
                    ioHandler->GetKeyboard().ReleaseConsumedKey(inputKey);

                if (consumed)
                    return;
                if (action == 1)
                    EventManager::Instance().Publish(KeyPressedEvent{key, mods});
                else if (action == 0)
                    EventManager::Instance().Publish(KeyReleasedEvent{key, mods});
            });

            appWindow->SetFocusCallback(
                [](bool focused) { EventManager::Instance().Publish(WindowFocusEvent{focused}); });

            if (m_Impl->m_AudioService->GetEngine() && !config.audio.audioDevice.empty() &&
                config.audio.audioDevice != "default")
            {
                const auto result = m_Impl->m_AudioService->GetEngine()->SetOutputDevice(config.audio.audioDevice);
                if (result != AudioOutputDeviceResult::Success)
                    LOGGER_WARN("Application") << "Playback device preference is unsupported or unavailable (result="
                                               << static_cast<int>(result) << ")";
            }
        }
        else
        {
            LOGGER_INFO("Application") << "Running in HEADLESS MODE. Graphics, Window, and Audio engine skipped.";
            m_Impl->m_AudioService = nullptr;
        }

        m_Impl->m_PhysicsWorld = m_Impl->m_Providers.CreatePhysicsWorld(config);
        m_Impl->m_PhysicsWorld->Initialize();
        if (m_Impl->m_GraphicsContext)
            m_Impl->m_PhysicsWorld->SetDebugRenderContext(&m_Impl->m_GraphicsContext->GetBufferManager(),
                                                          &m_Impl->m_GraphicsContext->GetDrawContext());

        if (m_Impl->m_GraphicsContext)
        {
            m_Impl->m_ResourceManager->Initialize(
                &m_Impl->m_GraphicsContext->GetShaderManager(), &m_Impl->m_GraphicsContext->GetTextureManager(),
                m_Impl->m_AudioService ? m_Impl->m_AudioService->GetEngine() : nullptr);
        }
        else
        {
            m_Impl->m_ResourceManager->InitializeHeadless();
        }

        m_Impl->m_ResourceManager->SetTextureAsyncEnabled(config.graphics.asyncResourceLoading);
        m_Impl->m_ResourceManager->SetTextureMaxAnisotropy(config.graphics.maxAnisotropy);
        m_Impl->m_ResourceManager->SetStrictAssetLoading(config.graphics.strictAssetLoading);
        m_Impl->m_Scene->InitializeManagers();

        auto& sl = ServiceLocator::Instance();
        sl.Register<IApplicationLifecycle>(this);
        sl.Register<Scene>(m_Impl->m_Scene.get());
        sl.Register<IPhysicsWorld>(m_Impl->m_PhysicsWorld.get());
        sl.Register<ResourceManager>(m_Impl->m_ResourceManager.get());
        sl.Register<IShaderLibrary>(m_Impl->m_Providers.GetShaderLibrary()
                                        ? m_Impl->m_Providers.GetShaderLibrary()
                                        : static_cast<IShaderLibrary*>(m_Impl->m_ResourceManager.get()));
        sl.Register<ITextureLibrary>(m_Impl->m_Providers.GetTextureLibrary()
                                         ? m_Impl->m_Providers.GetTextureLibrary()
                                         : static_cast<ITextureLibrary*>(m_Impl->m_ResourceManager.get()));
        sl.Register<IModelLibrary>(m_Impl->m_Providers.GetModelLibrary()
                                       ? m_Impl->m_Providers.GetModelLibrary()
                                       : static_cast<IModelLibrary*>(m_Impl->m_ResourceManager.get()));
        sl.Register<ISoundLibrary>(m_Impl->m_Providers.GetSoundLibrary()
                                       ? m_Impl->m_Providers.GetSoundLibrary()
                                       : static_cast<ISoundLibrary*>(m_Impl->m_ResourceManager.get()));
        sl.Register<IFontLibrary>(m_Impl->m_Providers.GetFontLibrary()
                                      ? m_Impl->m_Providers.GetFontLibrary()
                                      : static_cast<IFontLibrary*>(m_Impl->m_ResourceManager.get()));
        sl.Register<ISkyboxLibrary>(m_Impl->m_Providers.GetSkyboxLibrary()
                                        ? m_Impl->m_Providers.GetSkyboxLibrary()
                                        : static_cast<ISkyboxLibrary*>(m_Impl->m_ResourceManager.get()));
        sl.Register<SceneManager>(m_Impl->m_SceneManager.get());
        if (m_Impl->m_IOHandler)
            sl.Register<IOHandler>(m_Impl->m_IOHandler.get());
        if (m_Impl->m_AudioService)
            sl.Register<AudioService>(m_Impl->m_AudioService.get());
        if (m_Impl->m_AudioCaptureService)
            sl.Register<IAudioCaptureService>(m_Impl->m_AudioCaptureService.get());
        sl.Register<SystemManager>(m_Impl->m_SystemManager.get());
        sl.Register<ISystemRegistry>(m_Impl->m_SystemManager.get());
        sl.Register<RuntimeCore>(m_Impl->m_RuntimeCore.get());
        if (m_Impl->m_GraphicsContext)
            sl.Register<IGraphicsContext>(m_Impl->m_GraphicsContext.get());
        sl.Register<ConfigManager>(m_Impl->m_ConfigManager.get());
        sl.Register<TimeService>(m_Impl->m_TimeService.get());
        sl.Register<IScriptRegistry>(m_Impl->m_ScriptRegistry.get());
        sl.Register<CollisionMatrix>(m_Impl->m_CollisionMatrix.get());
        sl.Register<DataManager>(m_Impl->m_DataManager.get());
        sl.Register<IComponentCodecRegistry>(m_Impl->m_ComponentCodecs.get());

        m_Impl->m_RenderService = std::make_unique<RenderServiceImpl>();
        m_Impl->m_RenderService->Initialize();

        RegisterUserSystems(*m_Impl->m_SystemManager);
        m_Impl->m_SystemManager->CreateSystems();
        ApplyOptimizationConfig(m_Impl->m_ResourceManager.get(), m_Impl->m_SystemManager.get(), config.optimization);

        m_Impl->m_SceneManager->Initialize();
        m_Impl->m_ScriptRegistry->Initialize();
        RegisterUserStates();
        RegisterUserScripts();

        m_Impl->m_ConfigSubId =
            EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
                if (m_Impl->m_IOHandler)
                {
                    if (e.bitmask & ConfigChangedEvent::Window)
                    {
                        auto& mm = m_Impl->m_IOHandler->GetMonitorManager();
                        if (mm.GetWidth() != e.config.window.width || mm.GetHeight() != e.config.window.height ||
                            mm.GetWindowMode() != e.config.window.windowMode)
                        {
                            mm.SetWindowConfiguration(e.config.window.width, e.config.window.height,
                                                      e.config.window.windowMode, e.config.window.monitorIndex,
                                                      e.config.window.refreshRate);
                            mm.SetVsync(e.config.window.vsync);
                            if (m_Impl->m_IOHandler)
                                m_Impl->m_IOHandler->OnResize(mm.GetWidth(), mm.GetHeight());
                            if (m_Impl->m_GraphicsContext)
                                m_Impl->m_GraphicsContext->SetViewport(0, 0, mm.GetWidth(), mm.GetHeight());
                        }
                    }

                    if (e.bitmask & (ConfigChangedEvent::Window | ConfigChangedEvent::Graphics))
                    {
                        m_Impl->m_IOHandler->GetMonitorManager().SetVsync(e.config.window.vsync);
                        m_Impl->m_IOHandler->GetMonitorManager().SetFrameRateLimit(e.config.window.frameRateLimit);
                    }
                }
                if (m_Impl->m_ResourceManager && HasConfigChanged(e, ConfigChangedEvent::Graphics))
                {
                    m_Impl->m_ResourceManager->SetTextureAsyncEnabled(e.config.graphics.asyncResourceLoading);
                    m_Impl->m_ResourceManager->SetTextureMaxAnisotropy(e.config.graphics.maxAnisotropy);
                    m_Impl->m_ResourceManager->SetStrictAssetLoading(e.config.graphics.strictAssetLoading);
                }
                if (HasConfigChanged(e, ConfigChangedEvent::Optimization))
                {
                    if (m_Impl->m_GraphicsContext)
                        m_Impl->m_GraphicsContext->SetStateCacheEnabled(
                            e.config.optimization.renderStateCacheEnabled);
                    TransientBufferRing::SetGloballyEnabled(
                        e.config.optimization.persistentMappedBuffersEnabled);
                    ApplyOptimizationConfig(m_Impl->m_ResourceManager.get(), m_Impl->m_SystemManager.get(),
                                            e.config.optimization);
                }
                if (HasConfigChanged(e, ConfigChangedEvent::Input))
                {
                    if (m_Impl->m_IOHandler)
                    {
                        auto mode = m_Impl->m_IOHandler->GetMouse().GetCursorMode();
                        m_Impl->m_IOHandler->GetMouse().SetCursorMode(mode);
                        m_Impl->m_IOHandler->GetInputManager().SetGamepadDeadZone(e.config.input.gamepadDeadZone);
                    }
                }
                if (m_Impl->m_AudioCaptureService && HasConfigChanged(e, ConfigChangedEvent::Audio))
                {
                    const AudioCaptureSettings settings = MakeAudioCaptureSettings(e.config.audio);
                    m_Impl->m_AudioCaptureService->SetSettings(settings);
                    if (!e.config.audio.captureEnabled)
                        m_Impl->m_AudioCaptureService->Stop();
                    else if (!m_Impl->m_AudioCaptureService->IsCapturing() ||
                             m_Impl->m_CaptureDeviceId != e.config.audio.captureDevice)
                    {
                        m_Impl->m_AudioCaptureService->Stop();
                        m_Impl->m_AudioCaptureService->BeginCalibration(settings.calibrationSeconds);
                        const auto result = m_Impl->m_AudioCaptureService->Start(e.config.audio.captureDevice);
                        if (result == AudioCaptureResult::Success)
                            m_Impl->m_CaptureDeviceId = e.config.audio.captureDevice;
                        else
                            LOGGER_WARN("Application")
                                << "Microphone capture restart failed (result=" << static_cast<int>(result) << ")";
                    }
                }
            });

        m_Impl->m_RuntimeCore->Initialize();

        if (config.loadDefaultAssets)
        {
            const std::string defaultAssets = FileSystem::getPath(config.defaultAssetManifest);
            LOGGER_INFO("Application") << "Loading default assets from " << defaultAssets;
            m_Impl->m_SceneManager->LoadScene(defaultAssets, true);
            if (!m_Impl->m_SceneManager->IsLoaded(defaultAssets))
            {
                LOGGER_ERROR("Application") << "Default asset manifest failed to load: " << defaultAssets;
                if (config.graphics.strictAssetLoading)
                {
                    m_Impl->m_Lifecycle = ApplicationLifecycle::Failed;
                    return false;
                }
            }
        }
        m_Impl->m_ResourceManager->InitializePostLoad();

        m_Impl->m_SystemManager->Initialize(*m_Impl->m_ResourceManager, config.window.width, config.window.height);

        if (!config.iconPath.empty() && m_Impl->m_IOHandler)
        {
            LOGGER_INFO("Application") << "Setting window icon from: " << config.iconPath;
            m_Impl->m_IOHandler->GetMonitorManager().SetWindowIcon(FileSystem::getPath(config.iconPath));
        }

        LOGGER_INFO("Application") << "Application initialized successfully.";
        m_Impl->m_Lifecycle = ApplicationLifecycle::Initialized;
        EventManager::Instance().Publish(EngineInitializedEvent{});
        rollback.Commit();
        return true;
    }
    catch (const std::exception& exception)
    {
        LOGGER_ERROR("Application") << "Initialization failed: " << exception.what();
        return false;
    }
    catch (...)
    {
        LOGGER_ERROR("Application") << "Initialization failed with an unknown exception";
        return false;
    }
}

void Application::Run()
{
    if (!m_Impl || m_Impl->m_Lifecycle != ApplicationLifecycle::Initialized || !m_Impl->m_RuntimeCore)
    {
        LOGGER_WARN("Application") << "Run requires an initialized application";
        return;
    }
    auto serviceActivation = m_Impl->m_Services.Activate();
    m_Impl->m_Lifecycle = ApplicationLifecycle::Running;
    m_Impl->m_RuntimeCore->Run();
    Shutdown();
}

Scene& Application::GetScene()
{
    return *m_Impl->m_Scene;
}
RuntimeCore& Application::GetRuntimeCore()
{
    return *m_Impl->m_RuntimeCore;
}
StateMachine& Application::GetStateMachine()
{
    return m_Impl->m_RuntimeCore->GetStateMachine();
}

IScriptRegistry* Application::GetScriptRegistry()
{
    return m_Impl->m_ScriptRegistry.get();
}

AppConfig Application::GetConfig() const
{
    return m_Impl && m_Impl->m_ConfigManager ? m_Impl->m_ConfigManager->GetConfig() : AppConfig{};
}

ApplicationLifecycle Application::GetLifecycle() const
{
    return m_Impl ? m_Impl->m_Lifecycle : ApplicationLifecycle::Stopped;
}

ApplicationProviderCapabilities Application::GetProviderCapabilities() const
{
    return m_Impl ? m_Impl->m_Providers.GetCapabilities() : ApplicationProviderCapabilities{};
}
