#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

#include <engine/graphic/shader.h>
#include <engine/graphic/camera.h>
#include <engine/ecs/component.h>
#include <engine/ecs/system.h>
#include <engine/physic/physic_world.h> 

class Application {
public:
    Application();
    ~Application();

    bool Init();
    void Run();

    void ProcessInput();
    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

private:
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;
    GLFWwindow* window = nullptr;

    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    KeyboardManager keyboardManager;
    MouseManager mouseManager;

    std::unique_ptr<PhysicsWorld> physicsWorld;
    Scene scene;
    PhysicsSystem physicsSystem;
    RenderSystem renderSystem;
    AnimationSystem animationSystem;
    CameraSystem cameraSystem;
    CameraControlSystem cameraControlSystem;
    UIInteractSystem uiInteractSystem;
    UIRenderSystem uiRenderSystem;

    // Resources (giữ shader để dùng trong loop)
    std::unique_ptr<Shader> modelShader;
    std::unique_ptr<Shader> uiShader;
};