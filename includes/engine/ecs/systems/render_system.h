#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <scene/scene.h>
#include <graphic/geometry/static_batch_manager.h>
#include <vector>
#include <ecs/component.h>
#include <graphic/renderer/shadow_renderer.h>
#include <graphic/renderer/light_renderer.h>

class ResourceManager;
class Shader;

enum class AntiAliasingMode
{
    NONE = 0,
    FXAA = 1,
    TAA = 2
};

class RenderSystem
{
public:
    void Render(Scene &scene, int width, int height);

    void InitShadows(ResourceManager &res);
    void Shutdown();
    void RenderShadows(Scene &scene);
    void SetEnableShadows(bool enable) { m_ShadowRenderer.SetEnableShadows(enable); }
    void SetShadowMode(int mode) { m_ShadowRenderer.SetShadowMode(mode); }
    bool IsShadowsEnabled() const { return m_ShadowRenderer.IsShadowsEnabled(); }
    int GetShadowMode() const { return m_ShadowRenderer.GetShadowMode(); }

    void SetFaceCulling(bool enabled, int mode = GL_BACK);
    void SetDepthTest(bool enabled, int func = GL_LESS);

    Shadow &GetShadow() { return m_ShadowRenderer.GetShadow(); }
    int GetRenderedCount() const { return m_RenderedCount; }

    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }
    void SetDebugNoTexture(bool enable) { m_DebugNoTexture = enable; }
    void SetInstanceBatching(bool enable) { m_InstanceBatchingEnabled = enable; }
    void SetFrustumCulling(bool enable) { m_FrustumCullingEnabled = enable; }
    
    void SetShadowProjectionSize(float size) { m_ShadowRenderer.SetShadowProjectionSize(size); }
    void SetShadowFrustumCulling(bool enable) { m_ShadowRenderer.SetShadowFrustumCulling(enable); }
    void SetShadowDistanceCulling(float distance) { m_ShadowRenderer.SetShadowDistanceCulling(distance); }
    void SetDistanceCulling(float distance) { m_DistanceCullingSq = distance * distance; }

    void SetAntiAliasingMode(AntiAliasingMode mode) { m_AAMode = mode; }
    AntiAliasingMode GetAntiAliasingMode() const { return m_AAMode; }
    glm::vec2 GetJitterOffset() const { return m_JitterOffset; }
    
    const glm::mat4& GetPrevViewProj() const { return m_PrevViewProj; }
    const glm::mat4& GetCurrViewProj() const { return m_CurrViewProj; }

    StaticBatchManager &GetBatchManager() { return m_BatchManager; }
    
    void SetupMaterialUniforms(Shader *shader, entt::entity entity, Scene &scene);

private:
    ShadowRenderer m_ShadowRenderer;
    LightRenderer m_LightRenderer;
    StaticBatchManager m_BatchManager;
    int m_RenderedCount = 0;

    bool m_Enabled = true;
    bool m_InstanceBatchingEnabled = true;
    bool m_FrustumCullingEnabled = true;
    bool m_DebugNoTexture = false;
    unsigned int m_WhiteTextureID = 0;
    float m_DistanceCullingSq = 0.0f;

    AntiAliasingMode m_AAMode = AntiAliasingMode::NONE;
    glm::vec2 m_JitterOffset = glm::vec2(0.0f);
    int m_FrameIndex = 0;
    
    glm::mat4 m_PrevViewProj = glm::mat4(1.0f);
    glm::mat4 m_CurrViewProj = glm::mat4(1.0f);

    struct RenderItem
    {
        entt::entity entity;
        TransformComponent *transform;
        MeshRendererComponent *renderer;
    };
    std::vector<RenderItem> m_RenderQueue;
};
