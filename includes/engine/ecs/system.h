#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <entt/entt.hpp>

#include <engine/ecs/component.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <engine/graphic/shader.h>
#include <engine/core/keyboard_manager.h>
#include <engine/core/mouse_manager.h>

struct Scene
{
    entt::registry registry;

    entt::entity createEntity();
    entt::entity GetActiveCamera();
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
    void Render(Scene &scene);

private:
    void UploadLightData(Scene &scene, Shader *shader);
};

class CameraSystem
{
public:
    void Update(Scene &scene, float screenWidth, float screenHeight);
};

class CameraControlSystem
{
public:
    void Update(Scene &scene, float dt, const KeyboardManager &keyboard, const MouseManager &mouse);
};

class UIInteractSystem
{
public:
    void Update(Scene &scene, float dt, const MouseManager &mouse);
};

class UIRenderSystem {
public:
    void Render(Scene& scene, float screenWidth, float screenHeight);
};