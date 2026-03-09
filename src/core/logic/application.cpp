#include <core/logic/app_framework.h>
#include <core/logic/content_service.h>
#include <platform/unit/io_context.h>
#include <engine/platform/logic/io_handler.h>
#include <platform/logic/monitor_manager.h>
#include <core/unit/system_context.h>
#include <core/manager/system_manager.h>
#include <audio/logic/audio_manager.h>
#include <audio/logic/sound_player.h>
#include <core/logic/job_system.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/logic/physics_system.h>
#include <ecs/logic/render_system.h>
#include <ecs/logic/audio_system.h>
#include <ecs/logic/particle_system.h>
#include <ecs/logic/animation_system.h>
#include <ecs/logic/script_system.h>
#include <ecs/logic/skybox_system.h>
#include <ecs/logic/ui_system.h>
#include <ecs/logic/video_system.h>
#include <core/logic/event_system.h>
#include <core/logic/event_types.h>
#include <render/logic/post_process_pipeline.h>
#include <render/logic/renderer_initializer.h>
#include <platform/logic/input_system.h>
#include <render/interface/i_graphics_context.h>
#include <physics/interface/i_physics_world.h>
#include <platform/interface/i_window.h>
#include <resource/manager/resource_manager.h>
#include <scene/logic/scene.h>
#include <scene/logic/scene_manager.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>

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
    JobSystem::Instance().Shutdown();

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
                LOGGER_ERROR("Application") << "Destructor: CRASH while destroying Entity " << (uint32_t)entity;
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
    m_ContentService.reset();
    m_SceneManager.reset();
    m_RuntimeCore.reset();
    m_SoundPlayer.reset();
    m_PhysicsWorld.reset();

    if (m_Scene)
    {
        m_Scene.reset();
    }

    if (m_ResourceManager)
        m_ResourceManager->Shutdown();
    m_ResourceManager.reset();
    m_IOHandler.reset();

    LOGGER_INFO("Application") << "Application shutdown completed.";
}

bool Application::Init(const AppConfig &config)
{
    m_Config = config;

    JobSystem::Instance().Initialize();

    auto graphicsContext = AppBuilder::CreateGraphicsContext(m_Config);
    auto audioEngine = AppBuilder::CreateAudioEngine(m_Config);
    auto window = AppBuilder::MakeWindow();

    m_IOHandler = std::make_unique<IOHandler>(std::move(graphicsContext), std::move(audioEngine));

    if (!m_IOHandler->Init(std::move(window), m_Config.title, m_Config.width, m_Config.height, m_Config.windowMode,
                           m_Config.monitorIndex, m_Config.refreshRate, m_Config.vsync, m_Config.frameRateLimit))
    {
        LOGGER_ERROR("Application") << "Failed to initialize IOHandler";
        return false;
    }

    auto &context = m_IOHandler->GetGraphicsContext();
    RendererInitializer::Initialize(context);

    if (!m_Config.depthTestEnabled)
        context.SetDepthTest(false);
    context.SetCullFace(m_Config.cullFaceEnabled);

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

    if (!m_Config.audioDevice.empty() && m_Config.audioDevice != "default")
        m_IOHandler->GetAudioManager().SetActiveDevice(m_Config.audioDevice);

    m_PhysicsWorld = AppBuilder::CreatePhysicsWorld(m_Config);
    m_PhysicsWorld->Init();
    m_ResourceManager = std::make_unique<ResourceManager>();
    m_ResourceManager->Init(context.GetShaderManager());
    m_SoundPlayer = std::make_unique<SoundPlayer>(m_IOHandler->GetAudioManager().GetEngine());
    m_Scene->InitializeManagers();
    
    auto applyConfigFn = [this](const AppConfig& cfg) { this->ApplyConfig(cfg); };
    
    m_SceneManager = std::make_unique<SceneManager>();
    m_ContentService = std::make_unique<ContentService>();
    m_RuntimeCore = std::make_unique<RuntimeCore>();
    m_SystemManager = std::make_unique<SystemManager>();

    m_SceneManager->Init(GetContext(), applyConfigFn);
    m_ContentService->Init(GetContext());
    m_RuntimeCore->Init(GetContext(), m_Config, applyConfigFn);
    m_SystemManager->InitializeSystems(*m_ResourceManager, config.width, config.height, GetContext());
    m_SystemManager->ApplyConfig(m_Config);

    m_ResourceManager->CreateUIModel("default_rect", UIType::Color);

    m_ResourceManager->LoadShader("debugLine", "includes/engine/asset/shaders/debug_line.vs", "includes/engine/asset/shaders/debug_line.fs");

    LOGGER_INFO("Application") << "Loading default assets from includes/engine/asset/load.axs...";
    m_SceneManager->LoadScene("includes/engine/asset/load.axs");

    if (!m_Config.iconPath.empty())
    {
        LOGGER_INFO("Application") << "Setting window icon from: " << m_Config.iconPath;
        m_IOHandler->GetMonitorManager().SetWindowIcon(FileSystem::getPath(m_Config.iconPath));
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
SoundPlayer&     Application::GetSoundPlayer()    { return *m_SoundPlayer; }

IOHandler&       Application::GetIOHandler()      { return *m_IOHandler; }
ContentService&  Application::GetContentService() { return *m_ContentService; }
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

EngineContext Application::GetContext()
{
    EngineContext ctx;
    ctx.scene        = m_Scene.get();
    ctx.physics      = m_PhysicsWorld.get();
    ctx.resources    = m_ResourceManager.get();
    ctx.sceneManager = m_SceneManager.get();
    ctx.soundPlayer  = m_SoundPlayer.get();
    ctx.io           = m_IOHandler.get();
    ctx.systems      = m_SystemManager.get();
    ctx.runtime      = m_RuntimeCore.get();
    return ctx;
}

RenderSystem&        Application::GetRenderSystem()       { return *m_SystemManager->GetSystem<RenderSystem>(); }
PhysicsSystem&       Application::GetPhysicsSystem()      { return *m_SystemManager->GetSystem<PhysicsSystem>(); }
AudioSystem&         Application::GetAudioSystem()        { return *m_SystemManager->GetSystem<AudioSystem>(); }
UIRenderSystem&      Application::GetUIRenderSystem()     { return *m_SystemManager->GetSystem<UIRenderSystem>(); }
ScriptableSystem&    Application::GetScriptSystem()       { return *m_SystemManager->GetSystem<ScriptableSystem>(); }
ParticleSystem&      Application::GetParticleSystem()     { return *m_SystemManager->GetSystem<ParticleSystem>(); }
SkyboxRenderSystem&  Application::GetSkyboxRenderSystem() { return *m_SystemManager->GetSystem<SkyboxRenderSystem>(); }
AnimationSystem&     Application::GetAnimationSystem()    { return *m_SystemManager->GetSystem<AnimationSystem>(); }
VideoSystem&         Application::GetVideoSystem()        { return *m_SystemManager->GetSystem<VideoSystem>(); }
PostProcessPipeline& Application::GetPostProcess()        { return m_SystemManager->GetPostProcess(); }

const AppConfig& Application::GetConfig() const { return m_Config; }

void Application::ApplyConfig(const AppConfig& config)
{
    m_Config = config;
    if (m_IOHandler)
    {
        m_IOHandler->GetMonitorManager().SetWindowConfiguration(
            config.width, config.height,
            (WindowMode)config.windowMode,
            config.monitorIndex, config.refreshRate);
        
        m_IOHandler->GetMonitorManager().SetVsync(config.vsync);
        m_IOHandler->GetMonitorManager().SetFrameRateLimit(config.frameRateLimit);
    }
}

float Application::GetTimeScale() const      { return m_RuntimeCore->GetTimeScale(); }
void  Application::SetTimeScale(float ts)    { m_RuntimeCore->SetTimeScale(ts); }
float Application::GetRealDeltaTime() const  { return m_RuntimeCore->GetRealDeltaTime(); }
bool  Application::IsPaused() const          { return m_RuntimeCore->IsPaused(); }
void  Application::SetPaused(bool paused)    { m_RuntimeCore->SetPaused(paused); }

void Application::OnResize(int width, int height)
{
    m_IOHandler->OnResize(width, height);
    m_SystemManager->GetPostProcess().Resize(width, height);
}
void Application::OnMouseMove(double xpos, double ypos)   { m_IOHandler->OnMouseMove(xpos, ypos); }
void Application::OnMouseButton(int button, int action, int mods) { m_IOHandler->OnMouseButton(button, action, mods); }
void Application::OnScroll(double xoffset, double yoffset) { m_IOHandler->OnScroll(xoffset, yoffset); }

IOContext Application::GetIOContext()
{
    return {
        *m_IOHandler->GetMonitorManager().GetWindow(),
        m_IOHandler->GetKeyboard(),
        m_IOHandler->GetMouse(),
        m_IOHandler->GetInputManager(),
        m_IOHandler->GetMonitorManager(),
        m_IOHandler->GetGraphicsContext(),
        *m_IOHandler
    };
}

SystemContext Application::GetSystemContext()
{
    return {
        *m_SystemManager->GetSystem<RenderSystem>(),
        *m_SystemManager->GetSystem<PhysicsSystem>(),
        *m_SystemManager->GetSystem<AudioSystem>(),
        *m_SystemManager->GetSystem<UIRenderSystem>(),
        *m_SystemManager->GetSystem<ScriptableSystem>(),
        *m_SystemManager->GetSystem<ParticleSystem>(),
        *m_SystemManager->GetSystem<SkyboxRenderSystem>(),
        *m_SystemManager->GetSystem<AnimationSystem>(),
        *m_SystemManager->GetSystem<VideoSystem>(),
        m_SystemManager->GetPostProcess()
    };
}
