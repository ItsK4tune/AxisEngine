#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <scene/scene.h>
#include <ecs/component.h>
#include <utils/bullet_glm_helpers.h>
#include <graphic/shader.h>
#include <graphic/shadow.h>
#include <graphic/static_batch_manager.h>
#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <audio/sound_manager.h>
#include <resource/resource_manager.h>
#include <set>
#include <utility>

class PhysicsWorld;

class PhysicsSystem
{
public:
    void Update(Scene &scene, PhysicsWorld &physicsWorld, float dt);
    void RenderDebug(Scene &scene, PhysicsWorld &physicsWorld, Shader &shader, int screenWidth, int screenHeight);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    using CollisionPair = std::pair<entt::entity, entt::entity>;
    std::set<CollisionPair> m_activeCollisions;
    bool m_Enabled = true;
};

class AnimationSystem
{
public:
    void Update(Scene &scene, float dt);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};

class RenderSystem
{
public:
    void Render(Scene &scene);

    void InitShadows(class ResourceManager &res);
    void Shutdown();
    void RenderShadows(Scene &scene);
    void SetEnableShadows(bool enable) { m_EnableShadows = enable; }
    void SetShadowMode(int mode) { m_ShadowMode = mode; }
    bool IsShadowsEnabled() const { return m_EnableShadows; }
    int GetShadowMode() const { return m_ShadowMode; }

    void SetFaceCulling(bool enabled, int mode = GL_BACK);
    void SetDepthTest(bool enabled, int func = GL_LESS);

    Shadow &GetShadow() { return m_Shadow; }
    int GetRenderedCount() const { return m_RenderedCount; }

    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }
    void SetDebugNoTexture(bool enable) { m_DebugNoTexture = enable; }
    void SetInstanceBatching(bool enable) { m_InstanceBatchingEnabled = enable; }
    void SetFrustumCulling(bool enable) { m_FrustumCullingEnabled = enable; }
    void SetShadowProjectionSize(float size) { m_ShadowProjectionSize = size; }
    void SetShadowFrustumCulling(bool enable) { m_ShadowFrustumCullingEnabled = enable; }
    void SetShadowDistanceCulling(float distance) { m_ShadowDistanceCullingSq = distance * distance; }
    void SetDistanceCulling(float distance) { m_DistanceCullingSq = distance * distance; }

    StaticBatchManager &GetBatchManager() { return m_BatchManager; }

private:
    void UploadLightData(Scene &scene, Shader *shader);

    Shadow m_Shadow;
    StaticBatchManager m_BatchManager;
    int m_RenderedCount = 0;

    glm::mat4 m_LightSpaceMatrixDir[4];
    float m_FarPlanePoint = 25.0f;
    bool m_EnableShadows = true;
    int m_ShadowMode = 1; // 0=None, 1=Once, 2=All
    bool m_Enabled = true;
    bool m_InstanceBatchingEnabled = true;
    bool m_FrustumCullingEnabled = true;
    float m_ShadowProjectionSize = 20.0f;
    bool m_DebugNoTexture = false;
    unsigned int m_WhiteTextureID = 0;

    bool m_ShadowFrustumCullingEnabled = true;
    float m_ShadowDistanceCullingSq = 10000.0f; // Default 100^2
    float m_DistanceCullingSq = 0.0f;           // 0 = disabled

    unsigned int m_DirLightSSBO = 0;
    unsigned int m_PointLightSSBO = 0;
    unsigned int m_SpotLightSSBO = 0;

    struct RenderItem
    {
        entt::entity entity;
        TransformComponent *transform;
        MeshRendererComponent *renderer;
    };
    std::vector<RenderItem> m_RenderQueue;
};

class UIInteractSystem
{
public:
    void Update(Scene &scene, float dt, const MouseManager &mouse);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};

class UIRenderSystem
{
public:
    void Render(Scene &scene, float screenWidth, float screenHeight);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};

class Application;

class ScriptableSystem
{
public:
    void Update(Scene &scene, float dt, float unscaledDt, Application *app);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};

class SkyboxRenderSystem
{
public:
    void Render(Scene &scene);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};

class AudioSystem
{
public:
    void Update(Scene &scene, SoundManager &soundManager);
    void StopAll(Scene &scene);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};

class ParticleSystem
{
public:
    void Update(Scene &scene, float dt);
    void Render(Scene &scene, ResourceManager &res);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};

class VideoSystem
{
public:
    void Update(Scene &scene, ResourceManager &res, float dt);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
