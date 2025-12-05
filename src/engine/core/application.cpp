#include <engine/core/application.h>

#include <engine/utils/filesystem.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <engine/graphic/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->OnResize(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->OnMouseMove(xpos, ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    if (app) app->OnScroll(xoffset, yoffset);
}

Application::Application() 
    : camera(glm::vec3(0.0f, 0.0f, 3.0f)) 
{
    lastX = SCR_WIDTH / 2.0f;
    lastY = SCR_HEIGHT / 2.0f;
}

Application::~Application() {
    glfwTerminate();
}

bool Application::Init() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Game Engine", NULL, NULL);
    if (window == NULL) {
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

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    glEnable(GL_DEPTH_TEST);

    physicsWorld = std::make_unique<PhysicsWorld>();

    mainShader = std::make_unique<Shader>(
        FileSystem::getPath("resources/shaders/anim_model.vs").c_str(),
        FileSystem::getPath("resources/shaders/anim_model.fs").c_str()
    );

    static Model playerModel(FileSystem::getPath("resources/objects/player/Dying.fbx"));
    static Animation danceAnim(FileSystem::getPath("resources/objects/player/Dying.fbx"), &playerModel);

    EntityID playerID = scene.createEntity();
    scene.renderers[playerID].model = &playerModel;
    scene.animators[playerID].animator = new Animator(&danceAnim);
    
    scene.transforms[playerID].position = glm::vec3(0.0f, 5.0f, 0.0f);
    scene.transforms[playerID].scale = glm::vec3(0.01f);

    btCollisionShape* colShape = new btCapsuleShape(0.5f, 2.0f);
    btTransform startTransform;
    startTransform.setIdentity();
    startTransform.setOrigin(BulletGLMHelpers::convert(scene.transforms[playerID].position));
    
    float mass = 10.0f;
    scene.rigidbodies[playerID].body = physicsWorld->CreateRigidBody(mass, startTransform, colShape);
    scene.rigidbodies[playerID].body->setAngularFactor(btVector3(0, 1, 0)); // Khóa xoay trục X, Z

    EntityID floorID = scene.createEntity();
    scene.transforms[floorID].position = glm::vec3(0.0f, -2.0f, 0.0f);
    // Physics cho Floor (Box tĩnh)
    btCollisionShape* groundShape = new btBoxShape(btVector3(50, 1, 50));
    btTransform groundTrans;
    groundTrans.setIdentity();
    groundTrans.setOrigin(btVector3(0, -2, 0));
    physicsWorld->CreateRigidBody(0.0f, groundTrans, groundShape);

    return true;
}

void Application::Run() {
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        ProcessInput();

        // --- UPDATE ---
        // 1. Physics Step
        physicsWorld->Update(deltaTime);
        
        // 2. Sync Physics to Transform
        physicsSystem.Update(scene);

        // 3. Animation Step
        animationSystem.Update(scene, deltaTime);

        // --- RENDER ---
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        mainShader->use();

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        mainShader->setMat4("projection", projection);
        mainShader->setMat4("view", view);

        renderSystem.Render(scene, *mainShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Application::ProcessInput() {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void Application::OnResize(int width, int height) {
    glViewport(0, 0, width, height);
}

void Application::OnMouseMove(double xpos, double ypos) {
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; 
    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void Application::OnScroll(double xoffset, double yoffset) {
    camera.ProcessMouseScroll((float)yoffset);
}