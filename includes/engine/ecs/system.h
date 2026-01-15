#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <core/scene.h>
#include <ecs/component.h>
#include <utils/bullet_glm_helpers.h>
#include <graphic/shader.h>
#include <graphic/shadow.h>
#include <core/keyboard_manager.h>
#include <core/mouse_manager.h>
#include <core/sound_manager.h>
#include <core/resource_manager.h>
#include <set>
#include <utility>

class PhysicsWorld;

class PhysicsSystem
{
public:
    void Update(Scene &scene, PhysicsWorld &physicsWorld, float dt);
    void RenderDebug(Scene &scene, PhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight);

private:
    using CollisionPair = std::pair<entt::entity, entt::entity>;
    std::set<CollisionPair> m_activeCollisions;
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

    void InitShadows(class ResourceManager& res);
    void RenderShadows(Scene &scene);
    void SetEnableShadows(bool enable) { m_EnableShadows = enable; }

    void SetFaceCulling(bool enabled, int mode = GL_BACK);
    void SetDepthTest(bool enabled, int func = GL_LESS);
    
    Shadow& GetShadow() { return m_Shadow; }

private:
    void UploadLightData(Scene &scene, Shader *shader);
    
    Shadow m_Shadow;

    glm::mat4 m_LightSpaceMatrixDir;
    float m_FarPlanePoint = 25.0f;
    bool m_EnableShadows = true;
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

class ParticleSystem
{
public:
    void Update(Scene &scene, float dt);
    void Render(Scene &scene, ResourceManager &res);
};
