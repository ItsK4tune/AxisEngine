#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <engine/core/scene.h>
#include <engine/ecs/component.h>
#include <engine/utils/bullet_glm_helpers.h>
#include <engine/graphic/shader.h>
#include <engine/core/keyboard_manager.h>
#include <engine/core/mouse_manager.h>
#include <engine/core/sound_manager.h>

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

    void SetFaceCulling(bool enabled, int mode = GL_BACK);
    void SetDepthTest(bool enabled, int func = GL_LESS);

private:
    void UploadLightData(Scene &scene, Shader *shader);
};

class UIInteractSystem
{
public:
    void Update(Scene &scene, float dt, const MouseManager &mouse);
};

class UIRenderSystem
{
public:
    void Render(Scene &scene, float screenWidth, float screenHeight);
};

class Application;

class ScriptableSystem
{
public:
    void Update(Scene &scene, float dt, Application *app);
};

class SkyboxRenderSystem
{
public:
    void Render(Scene &scene);
};

class AudioSystem
{
public:
    void Update(Scene &scene, SoundManager& soundManager);
    void StopAll(Scene &scene);
};