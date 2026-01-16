#include <core/application.h>

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

// void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
// {
//     auto app = reinterpret_cast<Application *>(glfwGetWindowUserPointer(window));
//     if (app)
//         app->OnKey(key, scancode, action, mods);
// }

Application::Application(const AppConfig& config) 
    : m_Config(config), m_StateMachine(this) 
{
}

Application::~Application()
{
    
    // Explicitly clean up systems holding GL resources
    // renderSystem doesn't have a Shutdown(), but holds ShadowMap (FBO).
    // postProcess holds FBO.
    // resourceManager holds Shaders/Meshes.

    // If classes don't have Shutdown(), we rely on their destructors.
    // Since they are members, we can't force-call destructor here easily for direct members.
    // For smart pointers we can .reset().

    // postProcess is direct member. We might need a Shutdown() or just Move glfwTerminate out?
    // User said "don't move logic to main.cpp".
    // So we must clean up here.
    
    // Call Shutdown methods if they exist, or if we need to add them.
    // ResourceManager is unique_ptr, so .reset() works.
    m_StateMachine.Clear(); 

    // 2. Unload Scene and Resources
    // CRITICAL: Clear registry first to destroy components (Meshes/Shaders) while Context is ALIVE
    scene.registry.clear(); 
    
    sceneManager.reset();
    resourceManager.reset();
    soundManager.reset();
    inputManager.reset();
    mouseManager.reset();
    keyboardManager.reset();
    physicsWorld.reset(); // CRITICAL: Contains DebugDrawer which has GL resources (VBO/VAO)
    
    // 3. Clean up direct system members causing GL issues
    renderSystem.Shutdown();
    postProcess.Shutdown();

    glfwTerminate();
    // PostProcess and RenderSystem are direct members.
    // They will be destroyed AFTER ~Application body returns.
    // THIS IS THE PROBLEM.
    // If glfwTerminate is here, they die after context is dead.
    
    // Solution 1: Move glfwTerminate to a separate Shutdown() method called from main?
    // User said "ko đưa code logic lên main.cpp" (don't put logic in main).
    // This implies main should stay simple.
    
    // Solution 2: Make PostProcess and RenderSystem smart pointers too?
    // Solution 3: Add Shutdown() to them and call here.
    
    // Let's go with Solution 3 (Shutdown/Clean) or Solution 2 (pointers).
    // Pointers is easier to effect immediately without changing their class headers much.
    // But PostProcess::Init was called. 
    
    // Actually, I can just rely on the fact that I can't control direct member destruction order vs body code easily.
    // EXCEPT if I rely on `glfwTerminate` being called by a member that is destroyed LAST?
    // No.
    
    // To respect "no logic in main", I can just not call glfwTerminate() in ~Application() 
    // but relying on OS cleanup is bad.
    
    // The previous crash fix (Step 4442) moved glfwTerminate to main. 
    // User explicitly reverted/rejected that ("tôi out ra bằng esc thì bị, sửa cho tôi (ko đưa code logic lên main.cpp)").
    // This means I must fix it INSIDE Application.

    // To fix INSIDE Application:
    // I need to ensure context outlives members.
    // But members die AFTER ~Application().
    // So I cannot call glfwTerminate() in ~Application().
    
    // Is there a "OnDestroy" or similar?
    // No.
    
    // Wait, if I make `window` management a RAII wrapper member declared BEFORE everything else?
    // Then it would be destroyed LAST (reverse declaration order).
    // If I wrap `glfwTerminate` in a `WindowContext` struct and put it as the FIRST member of Application.
    // No, destruction is reverse of declaration.
    // First member constructed -> Last member destroyed.
    // So if WindowContext is First, it dies Last.
    // So if `glfwTerminate` is in `~WindowContext`, it runs Last.
    // That solves it!

    // Let's try to release what we can (pointers).
    // For direct members, I should verify if they hold GL resources (PostProcess does).
    // I can stick to the wrapper solution or finding a way to allow them to cleanup.

    // Let's try resetting the smart pointers we have, which covers ResourceManager (Heavy GL user).
    // PostProcess has a custom Framebuffer.
    // RenderSystem has custom Shadow buffers.
    // I will add Shutdown() to PostProcess and RenderSystem and call them here.
}

bool Application::Init()
{
    if (!glfwInit()) {
        std::cerr << "[Application] Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(m_Config.width, m_Config.height, m_Config.title.c_str(), NULL, NULL);
    if (window == NULL)
    {
        std::cout << "[Application] Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // glfwSetKeyCallback(window, key_callback);

    if (m_Config.vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "[Application] Failed to initialize GLAD" << std::endl;
        return false;
    }
    glEnable(GL_DEPTH_TEST);
    
    physicsWorld = std::make_unique<PhysicsWorld>();
    keyboardManager = std::make_unique<KeyboardManager>(window);
    mouseManager = std::make_unique<MouseManager>(window);
    inputManager = std::make_unique<InputManager>(*keyboardManager, *mouseManager);
    resourceManager = std::make_unique<ResourceManager>();
    soundManager = std::make_unique<SoundManager>();
    sceneManager = std::make_unique<SceneManager>(scene, *resourceManager, *physicsWorld, *soundManager, this);
    
    mouseManager->SetLastPosition(m_Config.width / 2.0, m_Config.height / 2.0);
    soundManager->Init();
    resourceManager->CreateUIModel("default_rect", UIType::Color);

    postProcess.Init(m_Config.width, m_Config.height);

    renderSystem.InitShadows(*resourceManager);
    resourceManager->LoadShader("debugLine", "resources/shaders/debug_line.vs", "resources/shaders/debug_line.fs");
    
    return true;
}

void Application::Run()
{
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        if (resourceManager) resourceManager->Update();

        ProcessInput();

        m_Accumulator += deltaTime;
        while (m_Accumulator >= m_FixedDeltaTime)
        {
            m_StateMachine.FixedUpdate(m_FixedDeltaTime);
            m_Accumulator -= m_FixedDeltaTime;
        }

        m_StateMachine.Update(deltaTime);
        mouseManager->EndFrame();
        
        renderSystem.RenderShadows(scene);
        
        glViewport(0, 0, m_Config.width, m_Config.height);
        
        postProcess.BeginCapture();

        m_StateMachine.Render();

        if (m_ShowPhysicsDebug)
        {
            Shader* debugShader = resourceManager->GetShader("debugLine");
            if (debugShader)
                physicsSystem.RenderDebug(scene, *physicsWorld, *debugShader, m_Config.width, m_Config.height);
        }

        postProcess.EndCapture();

        glfwSwapBuffers(window);

        if (m_Config.frameRateLimit > 0)
        {
            double targetFrameTime = 1.0 / (double)m_Config.frameRateLimit;
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

void Application::ProcessInput()
{
    if (keyboardManager->GetKey(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);
    
    if (keyboardManager->IsKeyDown(GLFW_KEY_F1))
    {
        m_ShowPhysicsDebug = !m_ShowPhysicsDebug;
    }
}

void Application::SetWindowConfiguration(int width, int height, WindowMode mode, int monitorIndex, int refreshRate)
{
    m_Config.width = width;
    m_Config.height = height;
    m_Config.mode = mode;
    m_Config.monitorIndex = monitorIndex;
    m_Config.refreshRate = refreshRate;

    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    GLFWmonitor* targetMonitor = nullptr;

    if (monitorIndex >= 0 && monitorIndex < count)
        targetMonitor = monitors[monitorIndex];
    else if (count > 0)
        targetMonitor = monitors[0];

    if (!targetMonitor) targetMonitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* videoMode = glfwGetVideoMode(targetMonitor);
    if (!videoMode) return;

    int targetRefreshRate = (refreshRate > 0) ? refreshRate : videoMode->refreshRate;

    if (mode == WindowMode::FULLSCREEN)
    {
        glfwSetWindowMonitor(window, targetMonitor, 0, 0, width, height, targetRefreshRate);
    }
    else if (mode == WindowMode::BORDERLESS)
    {
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_FALSE);
        
        int xpos = 0, ypos = 0;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);
        
        m_Config.width = videoMode->width;
        m_Config.height = videoMode->height;
        
        glfwSetWindowMonitor(window, nullptr, xpos, ypos, videoMode->width, videoMode->height, targetRefreshRate);
    }
    else 
    {
        glfwSetWindowAttrib(window, GLFW_DECORATED, GLFW_TRUE);
        
        int xpos = 0, ypos = 0;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);
        int cx = xpos + (videoMode->width - width) / 2;
        int cy = ypos + (videoMode->height - height) / 2;

        glfwSetWindowMonitor(window, nullptr, cx, cy, width, height, targetRefreshRate);
    }

    glViewport(0, 0, m_Config.width, m_Config.height);
    postProcess.Resize(m_Config.width, m_Config.height);
}

void Application::SetVsync(bool enable)
{
    m_Config.vsync = enable;
    glfwSwapInterval(enable ? 1 : 0);
}

void Application::SetFrameRateLimit(int limit)
{
    m_Config.frameRateLimit = limit;
}

void Application::SetPhysicsStep(float step)
{
    if (step > 0.0f)
        m_FixedDeltaTime = step;
}

void Application::OnResize(int width, int height)
{
    if (width == 0 || height == 0) return;

    m_Config.width = width;
    m_Config.height = height;
    glViewport(0, 0, width, height);
    postProcess.Resize(width, height);
}

void Application::OnMouseMove(double xpos, double ypos)
{
    mouseManager->UpdatePosition(xpos, ypos);
}

void Application::OnMouseButton(int button, int action, int mods)
{
    mouseManager->UpdateButton(button, action, mods);
}

void Application::OnScroll(double xoffset, double yoffset)
{
    mouseManager->UpdateScroll(xoffset, yoffset);
}