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
    
    renderSystem.Shutdown();
    postProcess.Shutdown();
}

bool Application::Init()
{
    AppConfig config = ConfigLoader::Load("configuration/settings.json");

    monitorManager.SetWindowTitle(config.title);
    monitorManager.SetWindowConfiguration(config.width, config.height, (WindowMode)config.windowMode, config.monitorIndex, config.refreshRate);
    monitorManager.SetVsync(config.vsync);
    monitorManager.SetFrameRateLimit(config.frameRateLimit);
    
    if (!monitorManager.Init())
        return false;

    if (!config.depthTestEnabled) glDisable(GL_DEPTH_TEST);
    if (config.cullFaceEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    
    if (!config.iconPath.empty())
    {
        monitorManager.SetWindowIcon(config.iconPath); 
    }

    GLFWwindow* window = monitorManager.GetWindow();
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);

    physicsWorld = std::make_unique<PhysicsWorld>();
    appHandler = std::make_unique<AppHandler>(window);

    // Initialize Managers first
    resourceManager = std::make_unique<ResourceManager>();
    soundManager = std::make_unique<SoundManager>();
    sceneManager = std::make_unique<SceneManager>(scene, *resourceManager, *physicsWorld, *soundManager, this);
    
    appHandler->GetMouse().SetLastPosition(monitorManager.GetWidth() / 2.0, monitorManager.GetHeight() / 2.0);
    
    soundManager->Init();

    if (!config.audioDevice.empty() && config.audioDevice != "default") {
        soundManager->SetActiveDevice(config.audioDevice);
    }

    resourceManager->CreateUIModel("default_rect", UIType::Color);

    postProcess.Init(monitorManager.GetWidth(), monitorManager.GetHeight());

    renderSystem.InitShadows(*resourceManager);
    renderSystem.SetEnableShadows(config.shadowsEnabled);

    // Load Shaders AFTER ResourceManager is valid
    resourceManager->LoadShader("debugLine", "resources/shaders/debug_line.vs", "resources/shaders/debug_line.fs");

#ifdef ENABLE_DEBUG_SYSTEM
    // Initialize Debug System LAST so all other systems are ready
    debugSystem = std::make_unique<DebugSystem>();
    debugSystem->Init(this);
#endif
    
    return true;
}

void Application::Run()
{
    while (!glfwWindowShouldClose(monitorManager.GetWindow()))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        if (resourceManager) resourceManager->Update();

        appHandler->ProcessInput(monitorManager.GetWindow());

#ifdef ENABLE_DEBUG_SYSTEM
        if (debugSystem) debugSystem->OnUpdate(deltaTime);
#endif

        m_Accumulator += deltaTime;
        while (m_Accumulator >= m_FixedDeltaTime)
        {
            physicsSystem.Update(scene, *physicsWorld, m_FixedDeltaTime); // Logic/Physics Step (Internal check enabled)
            
            // State Fixed Update
            m_StateMachine.FixedUpdate(m_FixedDeltaTime);

            m_Accumulator -= m_FixedDeltaTime;
        }

        // Logic Update (Internal check enabled)
        scriptSystem.Update(scene, deltaTime, this);
        animationSystem.Update(scene, deltaTime);
        videoSystem.Update(scene, *resourceManager, deltaTime);
        uiInteractSystem.Update(scene, deltaTime, appHandler->GetMouse());
        audioSystem.Update(scene, *soundManager);
        particleSystem.Update(scene, deltaTime);

        m_StateMachine.Update(deltaTime); // State specific logic
        appHandler->GetMouse().EndFrame();
        
        // Rendering (Internal check enabled)
        renderSystem.RenderShadows(scene);
        
        glViewport(0, 0, monitorManager.GetWidth(), monitorManager.GetHeight());
        
        postProcess.BeginCapture();

        skyboxRenderSystem.Render(scene); 
        renderSystem.Render(scene);       // Main Geometry
        
        // Render Particles (Before UI, After Geometry)
        particleSystem.Render(scene, *resourceManager);

        // State Render (deprecated but kept for legacy)
        m_StateMachine.Render(); 

        uiRenderSystem.Render(scene, (float)monitorManager.GetWidth(), (float)monitorManager.GetHeight());

#ifdef ENABLE_DEBUG_SYSTEM
        if (debugSystem) debugSystem->Render(scene);
#endif

        postProcess.EndCapture();

        glfwSwapBuffers(monitorManager.GetWindow());
        
        int frameRateLimit = monitorManager.GetFrameRateLimit();
        if (frameRateLimit > 0)
        {
            double targetFrameTime = 1.0 / (double)frameRateLimit;
            double frameEnd = glfwGetTime();
            double frameElapsed = frameEnd - currentFrame;
            
            while (frameElapsed < targetFrameTime)
            {
                frameEnd = glfwGetTime();
                frameElapsed = frameEnd - currentFrame;
            }
        }
    }
}

void Application::SetPhysicsStep(float step)
{
    if (step > 0.0f)
        m_FixedDeltaTime = step;
}

void Application::OnResize(int width, int height)
{
    monitorManager.OnResize(width, height);
    postProcess.Resize(width, height);
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