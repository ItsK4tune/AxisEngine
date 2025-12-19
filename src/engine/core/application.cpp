#include <engine/core/application.h>

#include <engine/utils/filesystem.h>
#include <engine/utils/bullet_glm_helpers.h>
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
    glfwTerminate();
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
    resourceManager = std::make_unique<ResourceManager>();
    soundManager = std::make_unique<SoundManager>();
    sceneManager = std::make_unique<SceneManager>(scene, *resourceManager, *physicsWorld, *soundManager);
    
    mouseManager->SetLastPosition(m_Config.width / 2.0, m_Config.height / 2.0);
    soundManager->Init();
    resourceManager->CreateUIModel("default_rect", UIType::Color);

    postProcess.Init(m_Config.width, m_Config.height);

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

        ProcessInput();
        m_StateMachine.Update(deltaTime);
        mouseManager->EndFrame();
        
        postProcess.BeginCapture();

        m_StateMachine.Render();

        postProcess.EndCapture();

        glfwSwapBuffers(window);
    }
}

void Application::ProcessInput()
{
    if (keyboardManager->GetKey(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);
}

void Application::OnResize(int width, int height)
{
    glViewport(0, 0, width, height);
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