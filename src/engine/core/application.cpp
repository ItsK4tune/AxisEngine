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

    modelShader = std::make_unique<Shader>(
        FileSystem::getPath("resources/shaders/anim_model.vs").c_str(),
        FileSystem::getPath("resources/shaders/anim_model.fs").c_str());

    uiShader = std::make_unique<Shader>(
        FileSystem::getPath("resources/shaders/ui.vs").c_str(),
        FileSystem::getPath("resources/shaders/ui.fs").c_str());

    uiRenderSystem.Init();

    static Model playerModel(FileSystem::getPath("resources/objects/player/Dying.fbx"));
    static Animation danceAnim(FileSystem::getPath("resources/objects/player/Dying.fbx"), &playerModel);

    auto playerEntity = scene.createEntity();

    auto &pTrans = scene.registry.emplace<TransformComponent>(playerEntity);
    pTrans.position = glm::vec3(0.0f, 5.0f, 0.0f);
    pTrans.scale = glm::vec3(0.01f);

    auto &pRender = scene.registry.emplace<MeshRendererComponent>(playerEntity);
    pRender.model = &playerModel;
    pRender.shader = modelShader.get();

    auto &pAnim = scene.registry.emplace<AnimationComponent>(playerEntity);
    pAnim.animator = new Animator(&danceAnim);

    auto &pRb = scene.registry.emplace<RigidBodyComponent>(playerEntity);
    btCollisionShape *colShape = new btCapsuleShape(0.5f, 2.0f);
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(BulletGLMHelpers::convert(pTrans.position));
    pRb.body = physicsWorld->CreateRigidBody(10.0f, startTransform, colShape);
    pRb.body->setAngularFactor(btVector3(0, 1, 0));

    auto camEntity = scene.createEntity();
    auto &cTrans = scene.registry.emplace<TransformComponent>(camEntity);
    cTrans.position = glm::vec3(0.0f, 2.0f, 10.0f);

    auto &cComp = scene.registry.emplace<CameraComponent>(camEntity);
    cComp.isPrimary = true;
    cComp.yaw = -90.0f;

    auto floorEntity = scene.createEntity();
    auto &fTrans = scene.registry.emplace<TransformComponent>(floorEntity);
    fTrans.position = glm::vec3(0.0f, -2.0f, 0.0f);

    auto &fRb = scene.registry.emplace<RigidBodyComponent>(floorEntity);
    btCollisionShape *groundShape = new btBoxShape(btVector3(50, 1, 50));
    btTransform groundTrans;
    groundTrans.setIdentity();
    groundTrans.setOrigin(btVector3(0, -2, 0));
    fRb.body = physicsWorld->CreateRigidBody(0.0f, groundTrans, groundShape);

    auto sunEntity = scene.createEntity();
    auto &dirLight = scene.registry.emplace<DirectionalLightComponent>(sunEntity);
    dirLight.direction = glm::vec3(-0.5f, -1.0f, -0.5f);
    dirLight.intensity = 0.8f;
    dirLight.color = glm::vec3(1.0f, 0.95f, 0.8f);

    auto bulbEntity = scene.createEntity();
    auto &bulbTrans = scene.registry.emplace<TransformComponent>(bulbEntity);
    bulbTrans.position = glm::vec3(2.0f, 2.0f, 2.0f);

    auto &pointLight = scene.registry.emplace<PointLightComponent>(bulbEntity);
    pointLight.color = glm::vec3(1.0f, 0.0f, 0.0f); // Đỏ
    pointLight.intensity = 2.0f;
    pointLight.radius = 5.0f;

    auto btnEntity = scene.createEntity();
    auto &uiTrans = scene.registry.emplace<UITransformComponent>(btnEntity);
    uiTrans.position = glm::vec2(100, 100);
    uiTrans.size = glm::vec2(200, 50);
    uiTrans.zIndex = 10;

    auto &uiRenderer = scene.registry.emplace<UIRendererComponent>(btnEntity);
    uiRenderer.color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
    uiRenderer.shader = uiShader.get();

    auto &uiInteract = scene.registry.emplace<UIInteractiveComponent>(btnEntity);
    uiInteract.onClick = [](entt::entity e)
    {
        std::cout << "Button Clicked! Entity ID: " << (uint32_t)e << std::endl;
    };
    uiInteract.onHoverEnter = [](entt::entity e)
    {
        std::cout << "Hover Enter!" << std::endl;
    };

    auto &uiAnim = scene.registry.emplace<UIAnimationComponent>(btnEntity);
    uiAnim.hoverColor = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    return true;
}

void Application::Run()
{
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

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
        glfwPollEvents();
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