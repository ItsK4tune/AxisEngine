#include <core/app/application.h>
#include <audio/logic/audio_service.h>
#include <platform/unit/io_context.h>
#include <engine/platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <ecs/logic/system_manager.h>
#include <core/logic/job_system.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
#include <render/logic/render_service_impl.h>
#include <render/logic/post_process_pipeline.h>
#include <render/logic/renderer_initializer.h>
#include <platform/logic/input_manager.h>
#include <render/interface/i_graphics_context.h>
#include <physics/interface/i_physics_world.h>
#include <platform/interface/i_window.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <core/logic/log_manager.h>
#include <platform/logic/input_loader.h>
#include <core/logic/axis_assert.h>
#include <script/logic/script_registry.h>
#include <physics/logic/collision_matrix.h>
#include <glm/gtc/matrix_transform.hpp>

extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

struct Application::Impl
{
    std::unique_ptr<Scene> m_Scene;
    std::unique_ptr<IPhysicsWorld> m_PhysicsWorld;
    std::unique_ptr<ResourceManager> m_ResourceManager;
    std::unique_ptr<AudioService> m_AudioService;
    std::unique_ptr<RenderServiceImpl> m_RenderService;
    std::unique_ptr<SceneManager> m_SceneManager;

    std::unique_ptr<IOHandler> m_IOHandler;
    std::unique_ptr<TimeService> m_TimeService;
    std::unique_ptr<RuntimeCore> m_RuntimeCore;
    std::unique_ptr<SystemManager> m_SystemManager;
    std::unique_ptr<ConfigManager> m_ConfigManager;
    std::unique_ptr<ScriptRegistry> m_ScriptRegistry;
    std::unique_ptr<CollisionMatrix> m_CollisionMatrix;
    uint32_t m_ConfigSubId = 0;
};

Application::Application()
    : m_Impl(std::make_unique<Impl>())
{
    m_Impl->m_Scene = std::make_unique<Scene>();
}

Application::~Application() = default;

void Application::Shutdown()
{
    LOGGER_INFO("Application") << "Shutting down application...";
    
    if (m_Impl->m_RuntimeCore)
        m_Impl->m_RuntimeCore->Shutdown();

    if (m_Impl->m_Scene)
    {
        try {
            m_Impl->m_Scene->registry.clear();
        } catch (...) {
            LOGGER_ERROR("Application") << "Destructor CRASH during registry.clear()";
        }
    }

    if (m_Impl->m_SystemManager)
    {
        if (auto* phys = m_Impl->m_SystemManager->GetSystem("PhysicsSystem")) phys->Reset();
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
    m_Impl->m_ConfigManager.reset();
    
    if (m_Impl->m_ConfigSubId != 0)
        EventSystem::Instance().Unsubscribe<ConfigChangedEvent>(m_Impl->m_ConfigSubId);

    LOGGER_INFO("Application") << "Application shutdown completed.";
    LogManager::Instance().Shutdown();

    ServiceLocator::Instance().ClearAll();
}

bool Application::Initialize(const AppConfig &config)
{
    JobSystem::Instance().Initialize(config.numJobThreads);
    LogManager::Instance().Initialize(config.logLevel);
 

    m_Impl->m_ResourceManager = std::make_unique<ResourceManager>();
    m_Impl->m_SceneManager = std::make_unique<SceneManager>();
    m_Impl->m_RuntimeCore = std::make_unique<RuntimeCore>();
    m_Impl->m_SystemManager = std::make_unique<SystemManager>();
    m_Impl->m_ConfigManager = std::make_unique<ConfigManager>();
    m_Impl->m_ConfigManager->Initialize(config);
    m_Impl->m_TimeService = std::make_unique<DefaultTimeService>();

    m_Impl->m_ScriptRegistry = std::make_unique<ScriptRegistry>();
    m_Impl->m_CollisionMatrix = std::make_unique<CollisionMatrix>();

    auto graphicsContext = AppBuilder::CreateGraphicsContext(config);
    auto audioEngine = AppBuilder::CreateAudioEngine(config);
    auto window = AppBuilder::MakeWindow();

    m_Impl->m_IOHandler = std::make_unique<IOHandler>(std::move(graphicsContext));


    m_Impl->m_AudioService = std::make_unique<AudioService>();
    m_Impl->m_AudioService->Initialize(std::move(audioEngine));

    if (!m_Impl->m_IOHandler->Initialize(std::move(window), config.title, config.width, config.height, (int)config.windowMode,
                           config.monitorIndex, config.refreshRate, config.vsync, config.frameRateLimit))
    {
        AXIS_ASSERT(false, "Failed to initialize IOHandler - graphics/audio device error?");
        return false;
    }

    // Sync config with actual window size (in case it differs from requested)
    auto& mm = m_Impl->m_IOHandler->GetMonitorManager();
    m_Impl->m_ConfigManager->SetResolution(mm.GetWidth(), mm.GetHeight());

    auto &context = m_Impl->m_IOHandler->GetGraphicsContext();
    RendererInitializer::Initialize(context);

    if (!config.depthTestEnabled)
        context.SetDepthTest(false);
    context.SetCullFace(config.cullFaceEnabled);

    IWindow *appWindow = m_Impl->m_IOHandler->GetMonitorManager().GetWindow();
    appWindow->SetResizeCallback([this](int width, int height) {
        LOGGER_INFO("Application") << "Window resized to " << width << "x" << height;
        EventSystem::Instance().Publish(WindowResizedEvent{width, height});
        OnResize(width, height);
    });
    appWindow->SetCursorPosCallback([this](double x, double y) {
        OnMouseMove(x, y);
        EventSystem::Instance().Publish(MouseMovedEvent{x, y});
    });
    appWindow->SetMouseButtonCallback([this](int button, int action, int mods) {
        OnMouseButton(button, action, mods);
        if (action == 1) EventSystem::Instance().Publish(MouseButtonPressedEvent{button, mods});
        else if (action == 0) EventSystem::Instance().Publish(MouseButtonReleasedEvent{button, mods});
    });
    appWindow->SetScrollCallback([this](double x, double y) {
        OnScroll(x, y);
        EventSystem::Instance().Publish(MouseScrolledEvent{x, y});
    });
    appWindow->SetKeyCallback([this](int key, int scancode, int action, int mods) {
        if (action == 1) EventSystem::Instance().Publish(KeyPressedEvent{key, mods});
        else if (action == 0) EventSystem::Instance().Publish(KeyReleasedEvent{key, mods});
    });

    if (!config.audioDevice.empty() && config.audioDevice != "default")
    {
        LOGGER_INFO("Application") << "Audio device preference: " << config.audioDevice;
    }
 
    m_Impl->m_PhysicsWorld = AppBuilder::CreatePhysicsWorld(config);
    m_Impl->m_PhysicsWorld->Initialize();
    
    m_Impl->m_ResourceManager->Initialize(context.GetShaderManager(), context.GetTextureManager(), *m_Impl->m_AudioService->GetEngine());
    m_Impl->m_ResourceManager->GetTextureManager().SetAsyncEnabled(config.asyncResourceLoading);
    m_Impl->m_ResourceManager->GetTextureManager().SetMaxAnisotropy(config.maxAnisotropy);
    m_Impl->m_Scene->InitializeManagers();
    

    auto& sl = ServiceLocator::Instance();
    sl.Register<Scene>(m_Impl->m_Scene.get());
    sl.Register<IPhysicsWorld>(m_Impl->m_PhysicsWorld.get());
    sl.Register<ResourceManager>(m_Impl->m_ResourceManager.get());
    sl.Register<SceneManager>(m_Impl->m_SceneManager.get());
    sl.Register<IOHandler>(m_Impl->m_IOHandler.get());
    sl.Register<SystemManager>(m_Impl->m_SystemManager.get());
    sl.Register<RuntimeCore>(m_Impl->m_RuntimeCore.get());
    sl.Register<IGraphicsContext>(&m_Impl->m_IOHandler->GetGraphicsContext());
    sl.Register<ConfigManager>(m_Impl->m_ConfigManager.get());
    sl.Register<TimeService>(m_Impl->m_TimeService.get());
    sl.Register<ScriptRegistry>(m_Impl->m_ScriptRegistry.get());
    sl.Register<CollisionMatrix>(m_Impl->m_CollisionMatrix.get());

    m_Impl->m_RenderService = std::make_unique<RenderServiceImpl>();
    m_Impl->m_RenderService->Initialize();

    m_Impl->m_SystemManager->CreateSystems();

    m_Impl->m_SceneManager->Initialize();
    m_Impl->m_ScriptRegistry->Initialize();
    RegisterUserScripts();

    m_Impl->m_ConfigSubId = EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (m_Impl->m_IOHandler)
        {
            if (e.bitmask & ConfigChangedEvent::Window) {
                auto& mm = m_Impl->m_IOHandler->GetMonitorManager();
                if (mm.GetWidth() != e.config.width || mm.GetHeight() != e.config.height || mm.GetWindowMode() != e.config.windowMode) {
                    mm.SetWindowConfiguration(
                        e.config.width, e.config.height,
                        e.config.windowMode,
                        e.config.monitorIndex,
                        e.config.refreshRate
                    );
                    mm.SetVsync(e.config.vsync);
                    OnResize(mm.GetWidth(), mm.GetHeight());
                }
            }
            
            if (e.bitmask & (ConfigChangedEvent::Window | ConfigChangedEvent::Graphics))
            {
                m_Impl->m_IOHandler->GetMonitorManager().SetVsync(e.config.vsync);
                m_Impl->m_IOHandler->GetMonitorManager().SetFrameRateLimit(e.config.frameRateLimit);
            }
        }
    });

    m_Impl->m_RuntimeCore->Initialize();
    
    LOGGER_INFO("Application") << "Loading default assets from include/engine/asset/load.axs...";
    m_Impl->m_SceneManager->LoadScene("include/engine/asset/load.axs");
    m_Impl->m_ResourceManager->InitializePostLoad();

    m_Impl->m_SystemManager->InitializeSystems(*m_Impl->m_ResourceManager, config.width, config.height);

    if (!config.iconPath.empty())
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

Scene&           Application::GetScene()          { return *m_Impl->m_Scene; }
RuntimeCore&     Application::GetRuntimeCore()    { return *m_Impl->m_RuntimeCore; }
StateMachine&    Application::GetStateMachine()   { return m_Impl->m_RuntimeCore->GetStateMachine(); }

ScriptRegistry*  Application::GetScriptRegistry() { return m_Impl->m_ScriptRegistry.get(); }

const AppConfig& Application::GetConfig() const { return m_Impl->m_ConfigManager->GetConfig(); }

void Application::ApplyConfig(const AppConfig& config)
{
    m_Impl->m_ConfigManager->UpdateConfig(config);
}

float Application::GetTimeScale() const      { return m_Impl->m_RuntimeCore->GetTimeScale(); }
void  Application::SetTimeScale(float ts)    { m_Impl->m_RuntimeCore->SetTimeScale(ts); }
float Application::GetRealDeltaTime() const  { return m_Impl->m_RuntimeCore->GetRealDeltaTime(); }
bool  Application::IsPaused() const          { return m_Impl->m_RuntimeCore->IsPaused(); }
void  Application::SetPaused(bool paused)    { m_Impl->m_RuntimeCore->SetPaused(paused); }

void Application::OnResize(int width, int height)
{
    if (m_Impl->m_ConfigManager) m_Impl->m_ConfigManager->SetResolution(width, height);
    m_Impl->m_IOHandler->OnResize(width, height);
}
void Application::OnMouseMove(double xpos, double ypos)   { m_Impl->m_IOHandler->OnMouseMove(xpos, ypos); }
void Application::OnMouseButton(int button, int action, int mods) { m_Impl->m_IOHandler->OnMouseButton(button, action, mods); }
void Application::OnScroll(double xoffset, double yoffset) { m_Impl->m_IOHandler->OnScroll(xoffset, yoffset); }


