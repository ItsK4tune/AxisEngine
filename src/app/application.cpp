#include <app/application.h>
#include <app/app_builder.h>
#include <graphic/renderer_initializer.h>

#include <interface/graphic/i_graphics_context.h>
#include <audio/audio_manager.h>
#include <utils/logger.h>
#include <utils/filesystem.h>

Application::Application()
{
}

Application::~Application()
{
    if (m_RuntimeCore)
        m_RuntimeCore->GetStateMachine().Clear();

    if (m_SystemManager)
        m_SystemManager->ShutdownSystems();
    m_SystemManager.reset();

    m_Scene.registry.clear();

    m_ContentService.reset();
    m_SceneManager.reset();
    m_RuntimeCore.reset();

    m_SoundPlayer.reset();
    m_ResourceManager.reset();
    m_PhysicsWorld.reset();

    m_IOHandler.reset();

    LOGGER_INFO("Application") << "Application shutdown completed.";
}

bool Application::Init(const AppConfig &config)
{
    m_Config = config;

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
        LOGGER_DEBUG("Application") << "Window resized to " << width << "x" << height;
        OnResize(width, height);
    });
    appWindow->SetCursorPosCallback([this](double x, double y) { OnMouseMove(x, y); });
    appWindow->SetMouseButtonCallback([this](int button, int action, int mods) { OnMouseButton(button, action, mods); });
    appWindow->SetScrollCallback([this](double x, double y) { OnScroll(x, y); });

    if (!m_Config.audioDevice.empty() && m_Config.audioDevice != "default")
        m_IOHandler->GetAudioManager().SetActiveDevice(m_Config.audioDevice);

    m_PhysicsWorld = AppBuilder::CreatePhysicsWorld(m_Config);
    m_ResourceManager = std::make_unique<ResourceManager>();
    m_SoundPlayer = std::make_unique<SoundPlayer>(m_IOHandler->GetAudioManager().GetEngine());
    m_SceneManager = std::make_unique<SceneManager>(m_Scene, *m_ResourceManager, *m_PhysicsWorld, *m_SoundPlayer, shared_from_this());

    m_ContentService = std::make_unique<ContentService>(*m_ResourceManager, *m_SceneManager, *m_SoundPlayer);
    m_RuntimeCore = std::make_unique<RuntimeCore>(shared_from_this());

    m_ResourceManager->CreateUIModel("default_rect", UIType::Color);

    m_SystemManager = std::make_unique<SystemManager>();
    m_SystemManager->InitializeSystems(*m_ResourceManager, config.width, config.height, shared_from_this());
    m_SystemManager->ApplyConfig(m_Config);

    m_ResourceManager->LoadShader("debugLine", "src/asset/shaders/debug_line.vs", "src/asset/shaders/debug_line.fs");

    LOGGER_INFO("Application") << "Loading default assets from src/asset/load.scene...";
    m_SceneManager->LoadScene("src/asset/load.scene");

    if (!m_Config.iconPath.empty())
    {
        LOGGER_DEBUG("Application") << "Setting window icon from: " << m_Config.iconPath;
        m_IOHandler->GetMonitorManager().SetWindowIcon(FileSystem::getPath(m_Config.iconPath));
    }

    LOGGER_INFO("Application") << "Application initialized successfully.";
    return true;
}

void Application::Run()
{
    m_RuntimeCore->Run();
}

RenderSystem &Application::GetRenderSystem() { return m_SystemManager->GetRenderSystem(); }
PhysicsSystem &Application::GetPhysicsSystem() { return m_SystemManager->GetPhysicsSystem(); }
AudioSystem &Application::GetAudioSystem() { return m_SystemManager->GetAudioSystem(); }
UIRenderSystem &Application::GetUIRenderSystem() { return m_SystemManager->GetUIRenderSystem(); }
ScriptableSystem &Application::GetScriptSystem() { return m_SystemManager->GetScriptSystem(); }
ParticleSystem &Application::GetParticleSystem() { return m_SystemManager->GetParticleSystem(); }
SkyboxRenderSystem &Application::GetSkyboxRenderSystem() { return m_SystemManager->GetSkyboxRenderSystem(); }
AnimationSystem &Application::GetAnimationSystem() { return m_SystemManager->GetAnimationSystem(); }
VideoSystem &Application::GetVideoSystem() { return m_SystemManager->GetVideoSystem(); }
PostProcessPipeline &Application::GetPostProcess() { return m_SystemManager->GetPostProcess(); }

float Application::GetTimeScale() const { return m_RuntimeCore->GetTimeScale(); }
void Application::SetTimeScale(float timeScale) { m_RuntimeCore->SetTimeScale(timeScale); }
float Application::GetRealDeltaTime() const { return m_RuntimeCore->GetRealDeltaTime(); }
bool Application::IsPaused() const { return m_RuntimeCore->IsPaused(); }
void Application::SetPaused(bool paused) { m_RuntimeCore->SetPaused(paused); }

void Application::OnResize(int width, int height)
{
    m_IOHandler->OnResize(width, height);
    m_SystemManager->GetPostProcess().Resize(width, height);
}

void Application::OnMouseMove(double xpos, double ypos)
{
    m_IOHandler->OnMouseMove(xpos, ypos);
}

void Application::OnMouseButton(int button, int action, int mods)
{
    m_IOHandler->OnMouseButton(button, action, mods);
}

void Application::OnScroll(double xoffset, double yoffset)
{
    m_IOHandler->OnScroll(xoffset, yoffset);
}
