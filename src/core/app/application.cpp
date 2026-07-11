#include <core/app/application.h>
#include <core/logic/data_manager.h>
#include <audio/logic/audio_service.h>
#include <core/logic/axis_assert.h>
#include <core/logic/config_manager.h>
#include <core/logic/config_serializer.h>
#include <core/logic/event_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/job_system.h>
#include <core/logic/log_manager.h>
#include <core/logic/logger.h>
#include <core/logic/service_locator.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_manager.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/render_components.h>
#include <engine/platform/logic/io_handler.h>
#include <physics/interface/i_physics_world.h>
#include <physics/logic/collision_matrix.h>
#include <platform/interface/i_window.h>
#include <platform/logic/input_serializer.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/monitor_manager.h>
#include <platform/unit/io_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/logic/post_process_pipeline.h>
#include <render/logic/render_service_impl.h>
#include <render/logic/renderer_initializer.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <script/logic/script_registry.h>
#include <csignal>
#include <glm/gtc/matrix_transform.hpp>

static Application* s_Instance = nullptr;
static void HandleQuitSignal(int sig)
{
    if (s_Instance)
    {
        LOGGER_INFO("Application") << "Received termination signal (" << sig << "). Requesting graceful shutdown...";
        s_Instance->GetRuntimeCore().GetEngineLoop().Stop();
    }
}

#if defined(_WIN32)
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

struct Application::Impl
{
    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<IPhysicsWorld> m_PhysicsWorld;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<AudioService> m_AudioService;
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
    int m_ConfigSubId = -1;
};

Application::Application() : m_Impl(std::make_unique<Impl>())
{
    s_Instance = this;
    m_Impl->m_Scene = std::make_unique<Scene>();
}

Application::~Application()
{
    s_Instance = nullptr;
}

void Application::Shutdown()
{
    LOGGER_INFO("Application") << "Shutting down application...";

    if (m_Impl->m_RuntimeCore)
        m_Impl->m_RuntimeCore->Shutdown();

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

    if (m_Impl->m_SystemManager)
    {
        if (auto* phys = m_Impl->m_SystemManager->GetSystem("PhysicsSystem"))
            phys->Reset();
        m_Impl->m_SystemManager->Shutdown();
    }

    if (m_Impl->m_SceneManager)
    {
        m_Impl->m_SceneManager->Shutdown();
    }

    if (m_Impl->m_Scene)
    {
        m_Impl->m_Scene->ShutdownManagers();
    }

    m_Impl->m_SystemManager.reset();
    m_Impl->m_RenderService.reset();
    m_Impl->m_SceneManager.reset();
    m_Impl->m_RuntimeCore.reset();
    m_Impl->m_AudioService.reset();
    m_Impl->m_PhysicsWorld.reset();
    m_Impl->m_DataManager.reset();

    if (m_Impl->m_Scene)
    {
        m_Impl->m_Scene.reset();
    }

    if (m_Impl->m_ResourceManager)
        m_Impl->m_ResourceManager->Shutdown();
    m_Impl->m_ResourceManager.reset();
    m_Impl->m_IOHandler.reset();

    JobSystem::Instance().Shutdown();
    LOGGER_INFO("Application") << "Cleaning up ConfigManager and ServiceLocator...";

    if (m_Impl->m_ConfigSubId != -1)
    {
        EventManager::Instance().Unsubscribe<ConfigChangedEvent>(m_Impl->m_ConfigSubId);
        m_Impl->m_ConfigSubId = -1;
    }

    m_Impl->m_ConfigManager.reset();

    LOGGER_INFO("Application") << "Application shutdown completed.";
    LogManager::Instance().Shutdown();

    ServiceLocator::Instance().ClearAll();
}

bool Application::Initialize()
{
    AppConfig config;
    ConfigSerializer serializer(config.headlessMode);
    serializer.Deserialize("include/engine/asset/config.axs", config);

    JobSystem::Instance().Initialize(config.numJobThreads);
    LogManager::Instance().Initialize(config.logLevel);

    std::signal(SIGINT, HandleQuitSignal);
    std::signal(SIGTERM, HandleQuitSignal);

    m_Impl->m_ResourceManager = std::make_unique<ResourceManager>();
    m_Impl->m_SceneManager = std::make_unique<SceneManager>();
    m_Impl->m_RuntimeCore = std::make_unique<RuntimeCore>();
    m_Impl->m_SystemManager = std::make_unique<SystemManager>();
    m_Impl->m_ConfigManager = std::make_unique<ConfigManager>();
    m_Impl->m_ConfigManager->Initialize(config);
    m_Impl->m_TimeService = std::make_unique<DefaultTimeService>();

    m_Impl->m_ScriptRegistry = std::make_unique<ScriptRegistry>();
    m_Impl->m_CollisionMatrix = std::make_unique<CollisionMatrix>();
    m_Impl->m_DataManager = std::make_unique<DataManager>();

    bool effectiveHeadless = config.headlessMode;
#ifdef ENABLE_EDITOR
    if (effectiveHeadless)
    {
        LOGGER_INFO("Application") << "ENABLE_EDITOR active: overriding headless -> headfull (editor requires window).";
        effectiveHeadless = false;
    }
#endif

    if (!effectiveHeadless)
    {
        m_Impl->m_GraphicsContext = AppBuilder::CreateGraphicsContext(config);
        auto audioEngine = AppBuilder::CreateAudioEngine(config);
        auto window = AppBuilder::MakeWindow();

        m_Impl->m_IOHandler = std::make_unique<IOHandler>();
        m_Impl->m_AudioService = std::make_unique<AudioService>();
        m_Impl->m_AudioService->Initialize(std::move(audioEngine));

        if (!m_Impl->m_IOHandler->Initialize(std::move(window), config.title, config.width, config.height,
                                             (int)config.windowMode, config.monitorIndex, config.refreshRate,
                                             config.vsync, config.frameRateLimit))
        {
            AXIS_ASSERT(false, "Failed to initialize IOHandler - graphics/audio device error?");
            return false;
        }

        auto& mm = m_Impl->m_IOHandler->GetMonitorManager();
        m_Impl->m_ConfigManager->SetResolution(mm.GetWidth(), mm.GetHeight());

        if (!m_Impl->m_GraphicsContext->Initialize())
        {
            LOGGER_ERROR("Application") << "Failed to initialize graphics context";
            return false;
        }
        m_Impl->m_GraphicsContext->SetDepthTest(true);

        auto& context = *m_Impl->m_GraphicsContext;
        RendererInitializer::Initialize(context);

        if (!config.depthTestEnabled)
            context.SetDepthTest(false);
        context.SetCullFace(config.cullFaceEnabled);

        auto* configMgr = m_Impl->m_ConfigManager.get();
        auto* ioHandler = m_Impl->m_IOHandler.get();
        auto* graphicsCtx = m_Impl->m_GraphicsContext.get();

        IWindow* appWindow = m_Impl->m_IOHandler->GetMonitorManager().GetWindow();
        appWindow->SetResizeCallback([configMgr, ioHandler, graphicsCtx](int width, int height) {
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

        appWindow->SetKeyCallback([](int key, int scancode, int action, int mods) {
            if (action == 1)
                EventManager::Instance().Publish(KeyPressedEvent{key, mods});
            else if (action == 0)
                EventManager::Instance().Publish(KeyReleasedEvent{key, mods});
        });

        if (!config.audioDevice.empty() && config.audioDevice != "default")
        {
            LOGGER_INFO("Application") << "Audio device preference: " << config.audioDevice;
        }
    }
    else
    {
        LOGGER_INFO("Application") << "Running in HEADLESS MODE. Graphics, Window, and Audio engine skipped.";
        m_Impl->m_AudioService = nullptr;
    }

    m_Impl->m_PhysicsWorld = AppBuilder::CreatePhysicsWorld(config);
    m_Impl->m_PhysicsWorld->Initialize();

    if (m_Impl->m_GraphicsContext && m_Impl->m_AudioService)
    {
        m_Impl->m_ResourceManager->Initialize(&m_Impl->m_GraphicsContext->GetShaderManager(),
                                              &m_Impl->m_GraphicsContext->GetTextureManager(),
                                              *m_Impl->m_AudioService->GetEngine());
    }
    else
    {
        m_Impl->m_ResourceManager->InitializeHeadless();
    }

    m_Impl->m_ResourceManager->SetTextureAsyncEnabled(config.asyncResourceLoading);
    m_Impl->m_ResourceManager->SetTextureMaxAnisotropy(config.maxAnisotropy);
    m_Impl->m_ResourceManager->SetStrictAssetLoading(config.strictAssetLoading);
    m_Impl->m_Scene->InitializeManagers();

    auto& sl = ServiceLocator::Instance();
    sl.Register<Scene>(m_Impl->m_Scene.get());
    sl.Register<IPhysicsWorld>(m_Impl->m_PhysicsWorld.get());
    sl.Register<ResourceManager>(m_Impl->m_ResourceManager.get());
    sl.Register<SceneManager>(m_Impl->m_SceneManager.get());
    if (m_Impl->m_IOHandler)
        sl.Register<IOHandler>(m_Impl->m_IOHandler.get());
    if (m_Impl->m_AudioService)
        sl.Register<AudioService>(m_Impl->m_AudioService.get());
    sl.Register<SystemManager>(m_Impl->m_SystemManager.get());
    sl.Register<RuntimeCore>(m_Impl->m_RuntimeCore.get());
    if (m_Impl->m_GraphicsContext)
        sl.Register<IGraphicsContext>(m_Impl->m_GraphicsContext.get());
    sl.Register<ConfigManager>(m_Impl->m_ConfigManager.get());
    sl.Register<TimeService>(m_Impl->m_TimeService.get());
    sl.Register<IScriptRegistry>(m_Impl->m_ScriptRegistry.get());
    sl.Register<ScriptRegistry>(m_Impl->m_ScriptRegistry.get());
    sl.Register<CollisionMatrix>(m_Impl->m_CollisionMatrix.get());
    sl.Register<DataManager>(m_Impl->m_DataManager.get());

    m_Impl->m_RenderService = std::make_unique<RenderServiceImpl>();
    m_Impl->m_RenderService->Initialize();

    m_Impl->m_SystemManager->CreateSystems();

    m_Impl->m_SceneManager->Initialize();
    m_Impl->m_ScriptRegistry->Initialize();
    RegisterUserStates();
    RegisterUserScripts();

    m_Impl->m_ConfigSubId = EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (m_Impl->m_IOHandler)
        {
            if (e.bitmask & ConfigChangedEvent::Window)
            {
                auto& mm = m_Impl->m_IOHandler->GetMonitorManager();
                if (mm.GetWidth() != e.config.width || mm.GetHeight() != e.config.height ||
                    mm.GetWindowMode() != e.config.windowMode)
                {
                    mm.SetWindowConfiguration(e.config.width, e.config.height, e.config.windowMode,
                                              e.config.monitorIndex, e.config.refreshRate);
                    mm.SetVsync(e.config.vsync);
                    if (m_Impl->m_IOHandler)
                        m_Impl->m_IOHandler->OnResize(mm.GetWidth(), mm.GetHeight());
                    if (m_Impl->m_GraphicsContext)
                        m_Impl->m_GraphicsContext->SetViewport(0, 0, mm.GetWidth(), mm.GetHeight());
                }
            }

            if (e.bitmask & (ConfigChangedEvent::Window | ConfigChangedEvent::Graphics))
            {
                m_Impl->m_IOHandler->GetMonitorManager().SetVsync(e.config.vsync);
                m_Impl->m_IOHandler->GetMonitorManager().SetFrameRateLimit(e.config.frameRateLimit);
            }
        }
        if (m_Impl->m_ResourceManager && (e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::All)))
        {
            m_Impl->m_ResourceManager->SetTextureAsyncEnabled(e.config.asyncResourceLoading);
            m_Impl->m_ResourceManager->SetTextureMaxAnisotropy(e.config.maxAnisotropy);
            m_Impl->m_ResourceManager->SetStrictAssetLoading(e.config.strictAssetLoading);
        }
        if (e.bitmask & (ConfigChangedEvent::Input | ConfigChangedEvent::All))
        {
            if (m_Impl->m_IOHandler)
            {
                auto mode = m_Impl->m_IOHandler->GetMouse().GetCursorMode();
                m_Impl->m_IOHandler->GetMouse().SetCursorMode(mode);
            }
        }
    });

    m_Impl->m_RuntimeCore->Initialize();

    LOGGER_INFO("Application") << "Loading default assets from include/engine/asset/load.axs...";
    m_Impl->m_SceneManager->LoadScene("include/engine/asset/load.axs", true);
    m_Impl->m_ResourceManager->InitializePostLoad();

    m_Impl->m_SystemManager->Initialize(*m_Impl->m_ResourceManager, config.width, config.height);

    if (!config.iconPath.empty() && m_Impl->m_IOHandler)
    {
        LOGGER_INFO("Application") << "Setting window icon from: " << config.iconPath;
        m_Impl->m_IOHandler->GetMonitorManager().SetWindowIcon(FileSystem::getPath(config.iconPath));
    }

    LOGGER_INFO("Application") << "Application initialized successfully.";
    return true;
}

bool Application::Initialize(const AppConfig& incomingConfig)
{
    AppConfig config = incomingConfig;

    JobSystem::Instance().Initialize(config.numJobThreads);
    LogManager::Instance().Initialize(config.logLevel);

    std::signal(SIGINT, HandleQuitSignal);
    std::signal(SIGTERM, HandleQuitSignal);

    m_Impl->m_ResourceManager = std::make_unique<ResourceManager>();
    m_Impl->m_SceneManager = std::make_unique<SceneManager>();
    m_Impl->m_RuntimeCore = std::make_unique<RuntimeCore>();
    m_Impl->m_SystemManager = std::make_unique<SystemManager>();
    m_Impl->m_ConfigManager = std::make_unique<ConfigManager>();
    m_Impl->m_ConfigManager->Initialize(config);
    m_Impl->m_TimeService = std::make_unique<DefaultTimeService>();

    m_Impl->m_ScriptRegistry = std::make_unique<ScriptRegistry>();
    m_Impl->m_CollisionMatrix = std::make_unique<CollisionMatrix>();
    m_Impl->m_DataManager = std::make_unique<DataManager>();

    bool effectiveHeadless = config.headlessMode;
#ifdef ENABLE_EDITOR
    if (effectiveHeadless)
    {
        LOGGER_INFO("Application") << "ENABLE_EDITOR active: overriding headless -> headfull (editor requires window).";
        effectiveHeadless = false;
    }
#endif

    if (!effectiveHeadless)
    {
        m_Impl->m_GraphicsContext = AppBuilder::CreateGraphicsContext(config);
        auto audioEngine = AppBuilder::CreateAudioEngine(config);
        auto window = AppBuilder::MakeWindow();

        m_Impl->m_IOHandler = std::make_unique<IOHandler>();
        m_Impl->m_AudioService = std::make_unique<AudioService>();
        m_Impl->m_AudioService->Initialize(std::move(audioEngine));

        if (!m_Impl->m_IOHandler->Initialize(std::move(window), config.title, config.width, config.height,
                                             (int)config.windowMode, config.monitorIndex, config.refreshRate,
                                             config.vsync, config.frameRateLimit))
        {
            AXIS_ASSERT(false, "Failed to initialize IOHandler - graphics/audio device error?");
            return false;
        }

        auto& mm = m_Impl->m_IOHandler->GetMonitorManager();
        m_Impl->m_ConfigManager->SetResolution(mm.GetWidth(), mm.GetHeight());

        if (!m_Impl->m_GraphicsContext->Initialize())
        {
            LOGGER_ERROR("Application") << "Failed to initialize graphics context";
            return false;
        }
        m_Impl->m_GraphicsContext->SetDepthTest(true);

        auto& context = *m_Impl->m_GraphicsContext;
        RendererInitializer::Initialize(context);

        if (!config.depthTestEnabled)
            context.SetDepthTest(false);
        context.SetCullFace(config.cullFaceEnabled);

        auto* configMgr = m_Impl->m_ConfigManager.get();
        auto* ioHandler = m_Impl->m_IOHandler.get();
        auto* graphicsCtx = m_Impl->m_GraphicsContext.get();

        IWindow* appWindow = m_Impl->m_IOHandler->GetMonitorManager().GetWindow();
        appWindow->SetResizeCallback([configMgr, ioHandler, graphicsCtx](int width, int height) {
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

        appWindow->SetKeyCallback([](int key, int scancode, int action, int mods) {
            if (action == 1)
                EventManager::Instance().Publish(KeyPressedEvent{key, mods});
            else if (action == 0)
                EventManager::Instance().Publish(KeyReleasedEvent{key, mods});
        });

        if (!config.audioDevice.empty() && config.audioDevice != "default")
        {
            LOGGER_INFO("Application") << "Audio device preference: " << config.audioDevice;
        }
    }
    else
    {
        LOGGER_INFO("Application") << "Running in HEADLESS MODE. Graphics, Window, and Audio engine skipped.";
        m_Impl->m_AudioService = nullptr;
    }

    m_Impl->m_PhysicsWorld = AppBuilder::CreatePhysicsWorld(config);
    m_Impl->m_PhysicsWorld->Initialize();

    if (m_Impl->m_GraphicsContext && m_Impl->m_AudioService)
    {
        m_Impl->m_ResourceManager->Initialize(&m_Impl->m_GraphicsContext->GetShaderManager(),
                                              &m_Impl->m_GraphicsContext->GetTextureManager(),
                                              *m_Impl->m_AudioService->GetEngine());
    }
    else
    {
        m_Impl->m_ResourceManager->InitializeHeadless();
    }

    m_Impl->m_ResourceManager->SetTextureAsyncEnabled(config.asyncResourceLoading);
    m_Impl->m_ResourceManager->SetTextureMaxAnisotropy(config.maxAnisotropy);
    m_Impl->m_ResourceManager->SetStrictAssetLoading(config.strictAssetLoading);
    m_Impl->m_Scene->InitializeManagers();

    auto& sl = ServiceLocator::Instance();
    sl.Register<Scene>(m_Impl->m_Scene.get());
    sl.Register<IPhysicsWorld>(m_Impl->m_PhysicsWorld.get());
    sl.Register<ResourceManager>(m_Impl->m_ResourceManager.get());
    sl.Register<SceneManager>(m_Impl->m_SceneManager.get());
    if (m_Impl->m_IOHandler)
        sl.Register<IOHandler>(m_Impl->m_IOHandler.get());
    if (m_Impl->m_AudioService)
        sl.Register<AudioService>(m_Impl->m_AudioService.get());
    sl.Register<SystemManager>(m_Impl->m_SystemManager.get());
    sl.Register<RuntimeCore>(m_Impl->m_RuntimeCore.get());
    if (m_Impl->m_GraphicsContext)
        sl.Register<IGraphicsContext>(m_Impl->m_GraphicsContext.get());
    sl.Register<ConfigManager>(m_Impl->m_ConfigManager.get());
    sl.Register<TimeService>(m_Impl->m_TimeService.get());
    sl.Register<IScriptRegistry>(m_Impl->m_ScriptRegistry.get());
    sl.Register<ScriptRegistry>(m_Impl->m_ScriptRegistry.get());
    sl.Register<CollisionMatrix>(m_Impl->m_CollisionMatrix.get());
    sl.Register<DataManager>(m_Impl->m_DataManager.get());

    m_Impl->m_RenderService = std::make_unique<RenderServiceImpl>();
    m_Impl->m_RenderService->Initialize();

    m_Impl->m_SystemManager->CreateSystems();

    m_Impl->m_SceneManager->Initialize();
    m_Impl->m_ScriptRegistry->Initialize();
    RegisterUserStates();
    RegisterUserScripts();

    m_Impl->m_ConfigSubId = EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (m_Impl->m_IOHandler)
        {
            if (e.bitmask & ConfigChangedEvent::Window)
            {
                auto& mm = m_Impl->m_IOHandler->GetMonitorManager();
                if (mm.GetWidth() != e.config.width || mm.GetHeight() != e.config.height ||
                    mm.GetWindowMode() != e.config.windowMode)
                {
                    mm.SetWindowConfiguration(e.config.width, e.config.height, e.config.windowMode,
                                              e.config.monitorIndex, e.config.refreshRate);
                    mm.SetVsync(e.config.vsync);
                    if (m_Impl->m_IOHandler)
                        m_Impl->m_IOHandler->OnResize(mm.GetWidth(), mm.GetHeight());
                    if (m_Impl->m_GraphicsContext)
                        m_Impl->m_GraphicsContext->SetViewport(0, 0, mm.GetWidth(), mm.GetHeight());
                }
            }

            if (e.bitmask & (ConfigChangedEvent::Window | ConfigChangedEvent::Graphics))
            {
                m_Impl->m_IOHandler->GetMonitorManager().SetVsync(e.config.vsync);
                m_Impl->m_IOHandler->GetMonitorManager().SetFrameRateLimit(e.config.frameRateLimit);
            }
        }
        if (m_Impl->m_ResourceManager && (e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::All)))
        {
            m_Impl->m_ResourceManager->SetTextureAsyncEnabled(e.config.asyncResourceLoading);
            m_Impl->m_ResourceManager->SetTextureMaxAnisotropy(e.config.maxAnisotropy);
            m_Impl->m_ResourceManager->SetStrictAssetLoading(e.config.strictAssetLoading);
        }
        if (e.bitmask & (ConfigChangedEvent::Input | ConfigChangedEvent::All))
        {
            if (m_Impl->m_IOHandler)
            {
                auto mode = m_Impl->m_IOHandler->GetMouse().GetCursorMode();
                m_Impl->m_IOHandler->GetMouse().SetCursorMode(mode);
            }
        }
    });

    m_Impl->m_RuntimeCore->Initialize();

    LOGGER_INFO("Application") << "Loading default assets from include/engine/asset/load.axs...";
    m_Impl->m_SceneManager->LoadScene("include/engine/asset/load.axs", true);
    m_Impl->m_ResourceManager->InitializePostLoad();

    m_Impl->m_SystemManager->Initialize(*m_Impl->m_ResourceManager, config.width, config.height);

    if (!config.iconPath.empty() && m_Impl->m_IOHandler)
    {
        LOGGER_INFO("Application") << "Setting window icon from: " << config.iconPath;
        m_Impl->m_IOHandler->GetMonitorManager().SetWindowIcon(FileSystem::getPath(config.iconPath));
    }

    LOGGER_INFO("Application") << "Application initialized successfully.";
    return true;
}

void Application::Run()
{
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

ScriptRegistry* Application::GetScriptRegistry()
{
    return m_Impl->m_ScriptRegistry.get();
}

AppConfig Application::GetConfig() const
{
    return m_Impl->m_ConfigManager->GetConfig();
}
