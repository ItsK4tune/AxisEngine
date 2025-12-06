#include <engine/core/application.h>

#include <engine/utils/filesystem.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <engine/graphic/model.h>

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

Application::Application() {}

Application::~Application()
{
    glfwTerminate();
}

bool Application::Init()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Game Engine", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(window);

    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    glEnable(GL_DEPTH_TEST);

    keyboardManager.Init(window);
    mouseManager.SetLastPosition(SCR_WIDTH / 2.0, SCR_HEIGHT / 2.0);

    physicsWorld = std::make_unique<PhysicsWorld>();

    sceneManager = std::make_unique<SceneManager>(scene, resourceManager, *physicsWorld);
    resourceManager.CreateUIModel("default_rect", UIType::Color);
    sceneManager->LoadScene("scenes/placeholder.scene");

    auto view = scene.registry.view<UITransformComponent>();
    for (auto e : view)
    {
        // Gán đại event cho tất cả UI (Demo)
        auto &interact = scene.registry.emplace<UIInteractiveComponent>(e);
        interact.onClick = [](entt::entity)
        { std::cout << "Clicked from Scene File!\n"; };
    }

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

        cameraControlSystem.Update(scene, deltaTime, keyboardManager, mouseManager);
        uiInteractSystem.Update(scene, deltaTime, mouseManager);
        mouseManager.EndFrame();

        physicsWorld->Update(deltaTime);
        physicsSystem.Update(scene);
        animationSystem.Update(scene, deltaTime);

        cameraSystem.Update(scene, (float)SCR_WIDTH, (float)SCR_HEIGHT);

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderSystem.Render(scene);
        uiRenderSystem.Render(scene, (float)SCR_WIDTH, (float)SCR_HEIGHT);

        glfwSwapBuffers(window);
    }
}

void Application::ProcessInput()
{
    if (keyboardManager.GetKey(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, true);
}

void Application::OnResize(int width, int height)
{
    glViewport(0, 0, width, height);
}

void Application::OnMouseMove(double xpos, double ypos)
{
    mouseManager.UpdatePosition(xpos, ypos);
}

void Application::OnMouseButton(int button, int action, int mods)
{
    mouseManager.UpdateButton(button, action, mods);
}

void Application::OnScroll(double xoffset, double yoffset)
{
    mouseManager.UpdateScroll(xoffset, yoffset);
}