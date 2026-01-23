#include <app/application.h>
#include <app/config_loader.h>

#include <utils/filesystem.h>
#include <utils/bullet_glm_helpers.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
        app->OnResize(width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
        app->OnMouseMove(xpos, ypos);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
        app->OnMouseButton(button, action, mods);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app)
        app->OnScroll(xoffset, yoffset);
}

Application::Application()
    : m_StateMachine(this)
{
}

Application::~Application()
{
    m_StateMachine.Clear();

    scene.registry.clear();

    sceneManager.reset();
    resourceManager.reset();
    soundManager.reset();
    appHandler.reset();
    physicsWorld.reset();

    if (systemManager)
        systemManager->ShutdownSystems();

    systemManager.reset();
    engineLoop.reset();
}

bool Application::Init()
{
    AppConfig config = ConfigLoader::Load(FileSystem::getPath("configuration/settings.json"));

    monitorManager.SetWindowTitle(config.title);
    monitorManager.SetWindowConfiguration(config.width, config.height, (WindowMode)config.windowMode, config.monitorIndex, config.refreshRate);
    monitorManager.SetVsync(config.vsync);
    monitorManager.SetFrameRateLimit(config.frameRateLimit);

    if (!monitorManager.Init())
        return false;

    if (!config.depthTestEnabled)
        glDisable(GL_DEPTH_TEST);
    if (config.cullFaceEnabled)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    GLFWwindow *window = monitorManager.GetWindow();
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    physicsWorld = std::make_unique<PhysicsWorld>();
    appHandler = std::make_unique<AppHandler>(window);
    appHandler->OnResize(monitorManager.GetWidth(), monitorManager.GetHeight());

    resourceManager = std::make_unique<ResourceManager>();
    soundManager = std::make_unique<SoundManager>();
    sceneManager = std::make_unique<SceneManager>(scene, *resourceManager, *physicsWorld, *soundManager, this);

    appHandler->GetMouse().SetLastPosition(monitorManager.GetWidth() / 2.0, monitorManager.GetHeight() / 2.0);

    soundManager->Init();

    if (!config.audioDevice.empty() && config.audioDevice != "default")
    {
        soundManager->SetActiveDevice(config.audioDevice);
    }

    resourceManager->CreateUIModel("default_rect", UIType::Color);

    systemManager = std::make_unique<SystemManager>();
    systemManager->InitializeSystems(*resourceManager, monitorManager.GetWidth(), monitorManager.GetHeight(), this);
    systemManager->GetRenderSystem().SetShadowMode(config.shadowMode);
    systemManager->GetRenderSystem().SetShadowProjectionSize(config.shadowProjectionSize);
    systemManager->GetRenderSystem().SetInstanceBatching(config.instanceBatchingEnabled);
    systemManager->GetRenderSystem().SetFrustumCulling(config.frustumCullingEnabled);
    systemManager->GetRenderSystem().SetShadowFrustumCulling(config.shadowFrustumCullingEnabled);
    systemManager->GetRenderSystem().SetShadowDistanceCulling(config.shadowDistanceCulling);
    systemManager->GetRenderSystem().SetDistanceCulling(config.distanceCulling);
    
    systemManager->GetRenderSystem().SetAntiAliasingMode((AntiAliasingMode)config.antialiasing);

    resourceManager->LoadShader("debugLine", "src/asset/shaders/debug_line.vs", "src/asset/shaders/debug_line.fs");

    std::cout << "[Application] Loading default assets from src/asset/load.scene..." << std::endl;
    sceneManager->LoadScene("src/asset/load.scene");

    engineLoop = std::make_unique<EngineLoop>(this);

    if (!config.iconPath.empty())
    {
        monitorManager.SetWindowIcon(FileSystem::getPath(config.iconPath));
    }

    return true;
}

void Application::Run()
{
    engineLoop->Run();
}

void Application::SetPhysicsStep(float step)
{
    engineLoop->SetPhysicsStep(step);
}

void Application::SetTimeScale(float scale)
{
    engineLoop->SetTimeScale(scale);
}

void Application::SetPaused(bool paused)
{
    engineLoop->SetPaused(paused);
}

float Application::GetTimeScale() const
{
    return engineLoop->GetTimeScale();
}

float Application::GetRealDeltaTime() const
{
    return engineLoop->GetRealDeltaTime();
}

bool Application::IsPaused() const
{
    return engineLoop->IsPaused();
}

void Application::OnResize(int width, int height)
{
    monitorManager.OnResize(width, height);
    systemManager->GetPostProcess().Resize(width, height);
    if (appHandler)
        appHandler->OnResize(width, height);
}

void Application::OnMouseMove(double xpos, double ypos)
{
    appHandler->OnMouseMove(xpos, ypos);
}

void Application::OnMouseButton(int button, int action, int mods)
{
    appHandler->OnMouseButton(button, action, mods);
}

void Application::OnScroll(double xoffset, double yoffset)
{
    appHandler->OnScroll(xoffset, yoffset);
}