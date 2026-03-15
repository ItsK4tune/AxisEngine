#pragma once

#include <ecs/interface/i_system.h>

#include <ecs/unit/render_components.h>
#include <glm/glm.hpp>
#include <render/interface/i_query_manager.h>
#include <render/logic/command_queue.h>
#include <render/logic/frustum_culler.h>
#include <render/logic/light_renderer.h>
#include <render/logic/material_renderer.h>
#include <render/logic/occlusion_culler.h>
#include <render/logic/render_queue.h>
#include <render/logic/shadow_renderer.h>
#include <render/logic/static_batch_manager.h>
#include <render/logic/gbuffer.h>
#include <render/logic/shader.h>
#include <render/type/graphics_types.h>
#include <resource/interface/i_resource_libraries.h>
#include <scene/logic/scene.h>
#include <vector>

class IGraphicsContext;
class ResourceManager;
class Shader;

#define GLM_ENABLE_EXPERIMENTAL


enum class AntiAliasingMode
{
    NONE = 0,
    FXAA = 1,
    TAA = 2
};


class RenderSystem : public ISystem
{
public:

    void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 80; }
    std::string GetName() const override { return "RenderSystem"; }
    void BuildRenderQueues(Scene &scene, float alpha, int width = 0, int height = 0);
    void RenderShadows(Scene &scene);
    void RenderAlpha(Scene &scene, int width, int height, float alpha);
    void RenderTransparent(Scene &scene, int width, int height, float alpha);
    void Render(Scene &scene) override;
    void RenderDeferredLighting(Scene &scene, int width, int height);
    
    void ExecuteQueue(Scene& scene, const std::vector<RenderItem>& queue, bool isTransparentPass);

    void BindForDecals();
    void UnbindForDecals();

    void BindGBufferForWriting();
    void UnbindGBuffer();
    void SetMainFBO(uint32_t fbo) { m_MainFBO = fbo; }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    void Initialize(IGraphicsContext& context, IShaderLibrary &shaderLib);
    void Shutdown();

    void SetEnableShadows(bool enable) { m_ShadowRenderer.SetEnableShadows(enable); }
    void SetShadowMode(int mode) { m_ShadowRenderer.SetShadowMode(mode); }
    bool IsShadowsEnabled() const { return m_ShadowRenderer.IsShadowsEnabled(); }
    int GetShadowMode() const { return m_ShadowRenderer.GetShadowMode(); }

    void SetFaceCulling(bool enabled, CullMode mode = CullMode::Back);
    void SetDepthTest(bool enabled, CompareFunc func = CompareFunc::Less);

    void SetDeferredRendering(bool enable) { m_DeferredRenderingEnabled = enable; }
    bool IsDeferredRenderingEnabled() const { return m_DeferredRenderingEnabled; }
    uint32_t GetGBufferDepth() const { return m_GBuffer.GetDepthTexture(); }
    uint32_t GetGBufferID() const { return m_GBuffer.GetIDTexture(); }
    uint32_t GetGBufferPosition() const { return m_GBuffer.GetPositionTexture(); }
    uint32_t GetGBufferNormal() const { return m_GBuffer.GetNormalTexture(); }
    int GetGBufferWidth() const { return m_GBuffer.GetWidth(); }
    int GetGBufferHeight() const { return m_GBuffer.GetHeight(); }

    Shadow &GetShadow() { return m_ShadowRenderer.GetShadow(); }
    int GetRenderedCount() const { return m_RenderedCount; }
    void AddRenderedCount(int count) { m_RenderedCount += count; }

    void SetDebugNoTexture(bool enable) { m_DebugNoTexture = enable; }
    bool IsDebugNoTexture() const { return m_DebugNoTexture; }
    void SetInstanceBatching(bool enable) { m_InstanceBatchingEnabled = enable; }
    void SetFrustumCulling(bool enable) { m_FrustumCullingEnabled = enable; }
    void SetOcclusionCulling(bool enable) { m_OcclusionCullingEnabled = enable; }
    bool IsOcclusionCullingEnabled() const { return m_OcclusionCullingEnabled; }
    
    void SetRenderOrderEnabled(bool enable) { m_RenderOrderEnabled = enable; }
    bool IsRenderOrderEnabled() const { return m_RenderOrderEnabled; }
    
    void SetFilterLayerMask(uint32_t mask) { m_FilterLayerMask = mask; }
    uint32_t GetFilterLayerMask() const { return m_FilterLayerMask; }

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

    
    IGraphicsContext* GetContext() const { return m_Context; }

private:
    EngineContext m_Ctx;
        IGraphicsContext* m_Context = nullptr;
    ShadowRenderer m_ShadowRenderer;
    LightRenderer m_LightRenderer;
    StaticBatchManager m_BatchManager;
    int m_RenderedCount = 0;

    bool m_Enabled = true;
    bool m_InstanceBatchingEnabled = true;
    bool m_FrustumCullingEnabled = true;
    bool m_OcclusionCullingEnabled = false;
    bool m_DebugNoTexture = false;
    bool m_RenderOrderEnabled = true;
    uint32_t m_FilterLayerMask = 0xFFFFFFFF;
    unsigned int m_WhiteTextureID = 0;
    unsigned int m_BlackTextureID = 0;
    unsigned int m_FlatNormalTextureID = 0;
    float m_DistanceCullingSq = 0.0f;

    AntiAliasingMode m_AAMode = AntiAliasingMode::NONE;
    glm::vec2 m_JitterOffset = glm::vec2(0.0f);
    int m_FrameIndex = 0;

    glm::mat4 m_PrevViewProj = glm::mat4(1.0f);
    glm::mat4 m_CurrViewProj = glm::mat4(1.0f);
    glm::mat4 m_JitteredProjection = glm::mat4(1.0f);

    bool m_QueuesBuilt = false;
    float m_LastAlpha = -1.0f;
    int m_LastWidth = -1;
    int m_LastHeight = -1;

    
    FrustumCuller m_FrustumCuller;
    OcclusionCuller m_OcclusionCuller;
    RenderQueue m_RenderQueueObj;
    MaterialRenderer m_MaterialRenderer;

    std::vector<std::string> m_BonesUniforms;
    
    std::unique_ptr<GPUUBO> m_CameraUBO;
    std::unique_ptr<GPUUBO> m_GlobalLightUBO;
    std::unique_ptr<GPUUBO> m_GlobalDataUBO;
    GPUCameraData m_CameraData;
    GPUGlobalLightData m_GlobalLightData;
    GPUGlobalData m_GlobalData;

    GBuffer m_GBuffer;
    std::shared_ptr<Shader> m_GBufferShader;
    std::shared_ptr<Shader> m_DeferredLightShader;
    bool m_DeferredRenderingEnabled = false;

    GpuHandle m_QuadVAO = 0;
    GpuHandle m_QuadVBO = 0;
    void InitQuad();

    CommandQueue m_CommandQueue;
    uint32_t m_MainFBO = 0;
};
