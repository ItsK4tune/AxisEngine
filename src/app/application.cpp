#include <app/application.h>
#include <app/app_builder.h>
#include <app/io_handler.h>
#include <app/content_service.h>
#include <app/system_manager.h>
#include <app/monitor_manager.h>
#include <audio/audio_manager.h>
#include <scene/scene.h>
#include <scene/scene_manager.h>
#include <resource/resource_manager.h>
#include <audio/sound_player.h>
#include <interface/physics/i_physics_world.h>
#include <interface/graphic/i_graphics_context.h>
#include <interface/window/i_window.h>
#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <input/input_manager.h>
#include <ecs/systems/render_system.h>
#include <ecs/systems/physics_system.h>
#include <ecs/systems/audio_system.h>
#include <ecs/systems/ui_system.h>
#include <ecs/systems/script_system.h>
#include <ecs/systems/particle_system.h>
#include <ecs/systems/skybox_system.h>
#include <ecs/systems/animation_system.h>
#include <ecs/systems/video_system.h>
#include <graphic/core/post_process_pipeline.h>
#include <app/io_context.h>
#include <app/world_context.h>
#include <app/system_context.h>
#include <graphic/renderer_initializer.h>
#include <event/input_events.h>
#include <event/event_system.h>
#include <core/job_system.h>
#include <utils/logger.h>
#include <utils/filesystem.h>

Application::Application()
    : m_Scene(std::make_unique<Scene>())
{
}

Application::~Application()
{
    LOGGER_INFO("Application") << "Shutting down application...";

    // 1. Shutdown JobSystem first to stop any background tasks
    JobSystem::Instance().Shutdown();

    // 2. Halting Systems is CRITICAL before any data is destroyed.
    // They must disconnect from EnTT hooks while the registry is still stable.
    if (m_SystemManager)
    {
        m_SystemManager->GetPhysicsSystem().Reset();
        m_SystemManager->ShutdownSystems();
    }

    // 3. Clear states only after systems are halted.
    // This triggers GameState::OnExit, which might call SceneManager methods.
    if (m_RuntimeCore)
        m_RuntimeCore->GetStateMachine().Clear();

    // 4. Shutdown SceneManager (it will handle scene-specific entity destruction)
    if (m_SceneManager)
    {
        m_SceneManager->Shutdown();
    }

    // 5. Cleanup managers and services
    if (m_Scene)
    {
        m_Scene->ShutdownManagers();
    }

    m_SystemManager.reset();
    m_ContentService.reset();
    m_SceneManager.reset();
    m_RuntimeCore.reset();
    m_SoundPlayer.reset();
    m_ResourceManager.reset();

    // 6. Finally, clear the registry and destroy the scene object
    if (m_Scene)
    {
        m_Scene->registry.clear();
        m_Scene.reset();
    }
    m_PhysicsWorld.reset();
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
    m_SoundPlayer = std::make_unique<SoundPlayer>(m_IOHandler->GetAudioManager().GetEngine());
    m_SceneManager = std::make_unique<SceneManager>(*m_Scene, *m_ResourceManager, *m_PhysicsWorld, *m_SoundPlayer, this);

    m_ContentService = std::make_unique<ContentService>(*m_ResourceManager, *m_SceneManager, *m_SoundPlayer);
    m_RuntimeCore = std::make_unique<RuntimeCore>(this);

    m_ResourceManager->CreateUIModel("default_rect", UIType::Color);

    m_SystemManager = std::make_unique<SystemManager>();
    m_SystemManager->InitializeSystems(*m_ResourceManager, config.width, config.height, this);
    m_SystemManager->ApplyConfig(m_Config);

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
}

// --- Scene / World ---
Scene&           Application::GetScene()          { return *m_Scene; }
IPhysicsWorld&   Application::GetPhysicsWorld()   { return *m_PhysicsWorld; }
ResourceManager& Application::GetResourceManager(){ return *m_ResourceManager; }
SceneManager&    Application::GetSceneManager()   { return *m_SceneManager; }
SoundPlayer&     Application::GetSoundPlayer()    { return *m_SoundPlayer; }

// --- IO ---
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

// --- Runtime ---
RuntimeCore&     Application::GetRuntimeCore()    { return *m_RuntimeCore; }
StateMachine&    Application::GetStateMachine()   { return m_RuntimeCore->GetStateMachine(); }
SystemManager&   Application::GetSystemManager()  { return *m_SystemManager; }

// --- Systems ---
RenderSystem&        Application::GetRenderSystem()       { return m_SystemManager->GetRenderSystem(); }
PhysicsSystem&       Application::GetPhysicsSystem()      { return m_SystemManager->GetPhysicsSystem(); }
AudioSystem&         Application::GetAudioSystem()        { return m_SystemManager->GetAudioSystem(); }
UIRenderSystem&      Application::GetUIRenderSystem()     { return m_SystemManager->GetUIRenderSystem(); }
ScriptableSystem&    Application::GetScriptSystem()       { return m_SystemManager->GetScriptSystem(); }
ParticleSystem&      Application::GetParticleSystem()     { return m_SystemManager->GetParticleSystem(); }
SkyboxRenderSystem&  Application::GetSkyboxRenderSystem() { return m_SystemManager->GetSkyboxRenderSystem(); }
AnimationSystem&     Application::GetAnimationSystem()    { return m_SystemManager->GetAnimationSystem(); }
VideoSystem&         Application::GetVideoSystem()        { return m_SystemManager->GetVideoSystem(); }
PostProcessPipeline& Application::GetPostProcess()        { return m_SystemManager->GetPostProcess(); }

// --- Config ---
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

// --- Time ---
float Application::GetTimeScale() const      { return m_RuntimeCore->GetTimeScale(); }
void  Application::SetTimeScale(float ts)    { m_RuntimeCore->SetTimeScale(ts); }
float Application::GetRealDeltaTime() const  { return m_RuntimeCore->GetRealDeltaTime(); }
bool  Application::IsPaused() const          { return m_RuntimeCore->IsPaused(); }
void  Application::SetPaused(bool paused)    { m_RuntimeCore->SetPaused(paused); }

// --- Callbacks ---
void Application::OnResize(int width, int height)
{
    m_IOHandler->OnResize(width, height);
    m_SystemManager->GetPostProcess().Resize(width, height);
}
void Application::OnMouseMove(double xpos, double ypos)   { m_IOHandler->OnMouseMove(xpos, ypos); }
void Application::OnMouseButton(int button, int action, int mods) { m_IOHandler->OnMouseButton(button, action, mods); }
void Application::OnScroll(double xoffset, double yoffset) { m_IOHandler->OnScroll(xoffset, yoffset); }

// --- Context getters ---
WorldContext Application::GetWorldContext()
{
    return { *m_Scene, *m_PhysicsWorld, *m_SceneManager, *m_ResourceManager, *m_SoundPlayer };
}

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
        m_SystemManager->GetRenderSystem(),
        m_SystemManager->GetPhysicsSystem(),
        m_SystemManager->GetAudioSystem(),
        m_SystemManager->GetUIRenderSystem(),
        m_SystemManager->GetScriptSystem(),
        m_SystemManager->GetParticleSystem(),
        m_SystemManager->GetSkyboxRenderSystem(),
        m_SystemManager->GetAnimationSystem(),
        m_SystemManager->GetVideoSystem(),
        m_SystemManager->GetPostProcess()
    };
}
