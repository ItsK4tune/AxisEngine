#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <engine/ecs/component.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <engine/graphic/shader.h>

using EntityID = size_t;

struct Scene
{
    std::vector<TransformComponent> transforms;
    std::vector<MeshRendererComponent> renderers;
    std::vector<RigidBodyComponent> rigidbodies;
    std::vector<AnimationComponent> animators;
    std::vector<CameraComponent> cameras;

    size_t createEntity();
    EntityID GetActiveCameraID();
};

class PhysicsSystem
{
public:
    void Update(Scene &scene);
};

class AnimationSystem
{
public:
    void Update(Scene &scene, float dt);
};

class RenderSystem
{
public:
    void Render(Scene &scene, Shader &shader);
};

class CameraSystem
{
public:
    void Update(Scene &scene, float screenWidth, float screenHeight);
};

class CameraControlSystem 
{
public:
    void Update(Scene &scene, float dt, GLFWwindow* window, double xoffset, double yoffset);
};