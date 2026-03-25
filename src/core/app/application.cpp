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
#include <ecs/logic/physics_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/post_process_system.h>
#include <ecs/logic/geometry_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/scriptable_system.h>
#include <ecs/logic/skybox_render_system.h>
#include <ecs/logic/ui_render_system.h>
#include <ecs/logic/video_system.h>
#include <core/logic/event_system.h>
#include <core/type/event_types.h>
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

Application::Application()
    : m_Scene(std::make_unique<Scene>())
{
}

Application::~Application()
{
}

void Application::Shutdown()
{
    LOGGER_INFO("Application") << "Shutting down application...";
    
    if (m_RuntimeCore)
        m_RuntimeCore->Shutdown();

    if (m_Scene)
    {
        auto& reg = m_Scene->registry;
        
        std::vector<entt::entity> aliveEntities;
        for (auto entity : reg.storage<entt::entity>())
        {
            aliveEntities.push_back(entity);
        }

        for (auto entity : aliveEntities)
        {
            try {
                reg.destroy(entity);
            } catch (...) {
                AXIS_ASSERT(false, "Destructor: CRASH while destroying Entity " + std::to_string((uint32_t)entity));
            }
        }

        m_Scene->registry.clear();
    }

    if (m_SystemManager)
    {
        m_SystemManager->GetSystem<PhysicsSystem>()->Reset();
        m_SystemManager->Shutdown();
    }

    if (m_SceneManager)
    {
        m_SceneManager->Shutdown();
    }

    if (m_Scene)
    {
        m_Scene->ShutdownManagers();
    }

    m_SystemManager.reset();
    m_SceneManager.reset();
    m_RuntimeCore.reset();
    m_AudioService.reset();
    m_PhysicsWorld.reset();

    if (m_Scene)
    {
        m_Scene.reset();
    }

    if (m_ResourceManager)
        m_ResourceManager->Shutdown();
    m_ResourceManager.reset();
    m_IOHandler.reset();


    JobSystem::Instance().Shutdown();
    LOGGER_INFO("Application") << "Cleaning up ConfigManager and ServiceLocator...";
    m_ConfigManager.reset();
    
    if (m_ConfigSubId != 0)
        EventSystem::Instance().Unsubscribe<ConfigChangedEvent>(m_ConfigSubId);

    LOGGER_INFO("Application") << "Application shutdown completed.";
    LogManager::Instance().Shutdown();

    ServiceLocator::Instance().ClearAll();
}

bool Application::Initialize(const AppConfig &config)
{
    JobSystem::Instance().Initialize(config.numJobThreads);
    LogManager::Instance().Initialize(config.logLevel);
 

    m_ResourceManager = std::make_unique<ResourceManager>();
    m_SceneManager = std::make_unique<SceneManager>();
    m_RuntimeCore = std::make_unique<RuntimeCore>();
    m_SystemManager = std::make_unique<SystemManager>();
    m_ConfigManager = std::make_unique<ConfigManager>();
    m_ConfigManager->Initialize(config);
    m_TimeService = std::make_unique<DefaultTimeService>();

    m_ScriptRegistry = std::make_unique<ScriptRegistry>();
    m_CollisionMatrix = std::make_unique<CollisionMatrix>();

    auto graphicsContext = AppBuilder::CreateGraphicsContext(config);
    auto audioEngine = AppBuilder::CreateAudioEngine(config);
    auto window = AppBuilder::MakeWindow();

    m_IOHandler = std::make_unique<IOHandler>(std::move(graphicsContext));


    m_AudioService = std::make_unique<AudioService>();
    m_AudioService->Initialize(std::move(audioEngine));

    if (!m_IOHandler->Initialize(std::move(window), config.title, config.width, config.height, (int)config.windowMode,
                           config.monitorIndex, config.refreshRate, config.vsync, config.frameRateLimit))
    {
        AXIS_ASSERT(false, "Failed to initialize IOHandler - graphics/audio device error?");
        return false;
    }

    // Sync config with actual window size (in case it differs from requested)
    auto& mm = m_IOHandler->GetMonitorManager();
    m_ConfigManager->SetResolution(mm.GetWidth(), mm.GetHeight());

    auto &context = m_IOHandler->GetGraphicsContext();
    RendererInitializer::Initialize(context);

    if (!config.depthTestEnabled)
        context.SetDepthTest(false);
    context.SetCullFace(config.cullFaceEnabled);

    IWindow *appWindow = GetWindow();
    appWindow->SetResizeCallback([this](int width, int height) {
        LOGGER_INFO("Application") << "Window resized to " << width << "x" << height;
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
 
    m_PhysicsWorld = AppBuilder::CreatePhysicsWorld(config);
    m_PhysicsWorld->Initialize();
    
    m_ResourceManager->Initialize(context.GetShaderManager(), context.GetTextureManager(), *m_AudioService->GetEngine());
    m_ResourceManager->GetTextureManager().SetAsyncEnabled(config.asyncResourceLoading);
    m_ResourceManager->GetTextureManager().SetMaxAnisotropy(config.maxAnisotropy);
    m_Scene->InitializeManagers();
    

    auto& sl = ServiceLocator::Instance();
    sl.Register<Scene>(m_Scene.get());
    sl.Register<IPhysicsWorld>(m_PhysicsWorld.get());
    sl.Register<ResourceManager>(m_ResourceManager.get());
    sl.Register<SceneManager>(m_SceneManager.get());
    sl.Register<IOHandler>(m_IOHandler.get());
    sl.Register<SystemManager>(m_SystemManager.get());
    sl.Register<RuntimeCore>(m_RuntimeCore.get());
    sl.Register<IGraphicsContext>(&m_IOHandler->GetGraphicsContext());
    sl.Register<ConfigManager>(m_ConfigManager.get());
    sl.Register<TimeService>(m_TimeService.get());
    sl.Register<ScriptRegistry>(m_ScriptRegistry.get());
    sl.Register<CollisionMatrix>(m_CollisionMatrix.get());



    m_SystemManager->CreateSystems();

    m_SceneManager->Initialize();
    m_ScriptRegistry->Initialize();
    RegisterUserScripts();

    m_ConfigSubId = EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (m_IOHandler)
        {
            if (e.bitmask & ConfigChangedEvent::Window) {
                auto& mm = m_IOHandler->GetMonitorManager();
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
                m_IOHandler->GetMonitorManager().SetVsync(e.config.vsync);
                m_IOHandler->GetMonitorManager().SetFrameRateLimit(e.config.frameRateLimit);
            }
        }
    });

    m_RuntimeCore->Initialize();
    
    LOGGER_INFO("Application") << "Loading default assets from include/engine/asset/load.axs...";
    m_SceneManager->LoadScene("include/engine/asset/load.axs");
    m_ResourceManager->InitializePostLoad();

    m_SystemManager->InitializeSystems(*m_ResourceManager, config.width, config.height);

    if (!config.iconPath.empty())
    {
        LOGGER_INFO("Application") << "Setting window icon from: " << config.iconPath;
        m_IOHandler->GetMonitorManager().SetWindowIcon(FileSystem::getPath(config.iconPath));
    }


    LOGGER_INFO("Application") << "Application initialized successfully.";
    return true;
}

void Application::Run()
{
    m_RuntimeCore->Run();
    Shutdown();
}

Scene&           Application::GetScene()          { return *m_Scene; }
IPhysicsWorld&   Application::GetPhysicsWorld()   { return *m_PhysicsWorld; }
ResourceManager& Application::GetResourceManager(){ return *m_ResourceManager; }
SceneManager&    Application::GetSceneManager()   { return *m_SceneManager; }
AudioService&     Application::GetAudioService()    { return *m_AudioService; }

IOHandler&       Application::GetIOHandler()      { return *m_IOHandler; }

MonitorManager&  Application::GetMonitorManager() { return m_IOHandler->GetMonitorManager(); }
KeyboardManager& Application::GetKeyboard() const { return m_IOHandler->GetKeyboard(); }
MouseManager&    Application::GetMouse() const    { return m_IOHandler->GetMouse(); }
InputManager&    Application::GetInputManager() const { return m_IOHandler->GetInputManager(); }
IGraphicsContext& Application::GetGraphicsContext() const { return m_IOHandler->GetGraphicsContext(); }
IWindow*         Application::GetWindow() const   { return m_IOHandler->GetMonitorManager().GetWindow(); }
int              Application::GetWidth() const    { return m_IOHandler->GetMonitorManager().GetWidth(); }
int              Application::GetHeight() const   { return m_IOHandler->GetMonitorManager().GetHeight(); }

RuntimeCore&     Application::GetRuntimeCore()    { return *m_RuntimeCore; }
StateMachine&    Application::GetStateMachine()   { return m_RuntimeCore->GetStateMachine(); }
SystemManager&   Application::GetSystemManager()  { return *m_SystemManager; }



const AppConfig& Application::GetConfig() const { return m_ConfigManager->GetConfig(); }

void Application::ApplyConfig(const AppConfig& config)
{

    m_ConfigManager->UpdateConfig(config);
}

float Application::GetTimeScale() const      { return m_RuntimeCore->GetTimeScale(); }
void  Application::SetTimeScale(float ts)    { m_RuntimeCore->SetTimeScale(ts); }
float Application::GetRealDeltaTime() const  { return m_RuntimeCore->GetRealDeltaTime(); }
bool  Application::IsPaused() const          { return m_RuntimeCore->IsPaused(); }
void  Application::SetPaused(bool paused)    { m_RuntimeCore->SetPaused(paused); }

void Application::OnResize(int width, int height)
{
    if (m_ConfigManager) m_ConfigManager->SetResolution(width, height);
    m_IOHandler->OnResize(width, height);
    
    auto& sl = ServiceLocator::Instance();
    auto* sm = sl.Resolve<SystemManager>();
    if (!sm) return;

    if (auto* pps = sm->GetSystem<PostProcessSystem>()) {
        pps->GetPipeline().Resize(width, height);
    }

    if (auto* gs = sm->GetSystem<GeometrySystem>()) {
        gs->GetGBuffer().Resize(width, height);
    }
    

    if (m_Scene) {
        auto view = m_Scene->registry.view<CameraComponent>();
        float aspect = (float)width / (float)height;
        for (auto entity : view) {
            auto& camera = view.get<CameraComponent>(entity);
            camera.aspectRatio = aspect;
            camera.screenWidth = width;
            camera.screenHeight = height;
            if (!camera.isOrthographic) {
                camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), aspect, camera.nearPlane, camera.farPlane);
            } else {
                float h = camera.orthoSize;
                float w = h * aspect;
                camera.projectionMatrix = glm::ortho(-w, w, -h, h, camera.nearPlane, camera.farPlane);
            }
        }
    }
}
void Application::OnMouseMove(double xpos, double ypos)   { m_IOHandler->OnMouseMove(xpos, ypos); }
void Application::OnMouseButton(int button, int action, int mods) { m_IOHandler->OnMouseButton(button, action, mods); }
void Application::OnScroll(double xoffset, double yoffset) { m_IOHandler->OnScroll(xoffset, yoffset); }

PostProcessPipeline& Application::GetPostProcess() { 
    return ServiceLocator::Instance().Require<SystemManager>().GetSystem<PostProcessSystem>()->GetPipeline(); 
}
