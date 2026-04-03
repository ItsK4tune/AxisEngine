#pragma once

#include <ecs/interface/i_render_service.h>
#include <render/unit/command_queue.h>
#include <render/logic/frustum_culler.h>
#include <render/unit/render_queue.h>
#include <render/logic/static_batch_manager.h>
#include <render/logic/occlusion_culler.h>
#include <resource/unit/shader.h>
#include <render/type/graphics_types.h>
#include <vector>
#include <memory>
#include <string>

class RenderCore;
class IGraphicsContext;
class ConfigManager;

class RenderServiceImpl : public IRenderService
{
public:
    RenderServiceImpl();
    virtual ~RenderServiceImpl();

    void Initialize();
    void Shutdown();

    // From IBaseSystem (inherited by IRenderService)
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    std::string GetName() const override { return "RenderService"; }
    int GetPriority() const override { return 0; }
    SystemCategory GetCategory() const override { return SystemCategory::RenderMain; }

    // ICameraService
    glm::vec3 GetCameraPosition() const override { return m_CameraPos; }
    glm::mat4 GetViewMatrix() const override { return m_ViewMatrix; }
    glm::mat4 GetProjectionMatrix() const override { return m_ProjMatrix; }
    float GetNearPlane() const override { return m_NearPlane; }
    float GetFarPlane() const override { return m_FarPlane; }

    // IRenderStateService
    AntiAliasingMode GetAntiAliasingMode() const override { return m_AAMode; }
    void SetAntiAliasingMode(AntiAliasingMode mode) override { m_AAMode = mode; }
    glm::mat4 GetPrevViewProj() const override { return m_PrevViewProj; }
    glm::mat4 GetCurrViewProj() const override { return m_CurrViewProj; }
    glm::vec2 GetJitterOffset() const override { return m_JitterOffset; }
    bool IsDebugNoTexture() const override { return m_DebugNoTexture; }
    void SetDebugNoTexture(bool enable) override { m_DebugNoTexture = enable; }
    bool IsWireframe() const override { return m_Wireframe; }
    void SetWireframe(bool enable) override { m_Wireframe = enable; }
    bool IsOcclusionCullingEnabled() const override { return m_OcclusionCullingEnabled; }
    void SetOcclusionCulling(bool enable) override { m_OcclusionCullingEnabled = enable; }
    RenderPath GetRenderPath() const override { return m_CachedRenderPath; }
    unsigned int GetWhiteTexture() const override;
    unsigned int GetBlackTexture() const override;
    unsigned int GetFlatNormalTexture() const override;
    void UpdateGlobalLightData(const GPUGlobalLightData& data) override;

    // IRenderQueueService
    int GetRenderedCount() const override { return m_RenderedCount; }
    void AddRenderedCount(int count) override { m_RenderedCount += count; }
    uint32_t GetMainFBO() const override { return m_MainFBO; }
    void SetMainFBO(uint32_t fbo) override { m_MainFBO = fbo; }
    StaticBatchManager& GetBatchManager() override { return m_BatchManager; }
    RenderQueue& GetRenderQueueObj() override { return m_RenderQueueObj; }
    void BuildRenderQueues(Scene &scene, float alpha, int width = 0, int height = 0) override;
    void BuildRenderQueuesWithCamera(Scene& scene, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float lodFactor = 1.0f, int width = 800, int height = 600, uint32_t cullingMask = 0xFFFFFFFF, bool isCapturingProbe = false, entt::entity excludeEntity = (entt::entity)0xFFFFFFFF) override;
    void ExecuteQueue(const std::vector<RenderItem>& queue, bool isTransparentPass, ShadowRenderer* shadowRenderer, MaterialRenderer* materialRenderer, Shader* overrideShader = nullptr) override;
    void SubmitCommand(const RenderDrawCommand& cmd) override;
    void FlushCommands() override;

    // IIBLService
    unsigned int GetIrradianceMap() const override { return m_IrradianceMap; }
    unsigned int GetPrefilterMap() const override { return m_PrefilterMap; }
    unsigned int GetBrdfLUT() const override { return m_BrdfLUT; }

    // Service-specific methods not in interface
    void SetFaceCulling(bool enabled, CullMode mode = CullMode::Back);
    void SetDepthTest(bool enabled, CompareFunc func = CompareFunc::Less);
    void SetInstanceBatching(bool enable) { m_InstanceBatchingEnabled = enable; }
    void SetFrustumCulling(bool enable) { m_FrustumCullingEnabled = enable; }
    void SetRenderOrderEnabled(bool enable) { m_RenderOrderEnabled = enable; }
    void SetFilterLayerMask(uint32_t mask) { m_FilterLayerMask = mask; }
    void SetDistanceCullingSq(float distanceSq) { m_DistanceCullingSq = distanceSq; }
    
    // For syncing with Update
    void IncrementFrame() { m_FrameIndex++; }
    void ResetRenderedCount() { m_RenderedCount = 0; }
    void ResetQueuesBuilt() { m_QueuesBuilt = false; }
    void AddTime(float dt) { m_GlobalData.time += dt; m_GlobalData.deltaTime = dt; }
    void FetchRenderPath();

private:
    StaticBatchManager m_BatchManager;
    RenderQueue m_RenderQueueObj;

    int m_RenderedCount = 0;

    bool m_Enabled = true;
    bool m_InstanceBatchingEnabled = true;
    bool m_FrustumCullingEnabled = true;
    bool m_OcclusionCullingEnabled = false;
    bool m_DebugNoTexture = false;
    bool m_Wireframe = false;
    bool m_RenderOrderEnabled = true;
    uint32_t m_FilterLayerMask = 0xFFFFFFFF;

    float m_DistanceCullingSq = 0.0f;
    RenderPath m_CachedRenderPath = RenderPath::Deferred;

    AntiAliasingMode m_AAMode = AntiAliasingMode::NONE;
    glm::vec2 m_JitterOffset = glm::vec2(0.0f);
    int m_FrameIndex = 0;

    glm::mat4 m_PrevViewProj = glm::mat4(1.0f);
    glm::mat4 m_CurrViewProj = glm::mat4(1.0f);
    glm::mat4 m_JitteredProjection = glm::mat4(1.0f);
    
    glm::vec3 m_CameraPos = glm::vec3(0.0f);
    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
    glm::mat4 m_ProjMatrix = glm::mat4(1.0f);
    float m_NearPlane = 0.1f;
    float m_FarPlane = 1000.0f;

    unsigned int m_IrradianceMap = 0;
    unsigned int m_PrefilterMap = 0;
    unsigned int m_BrdfLUT = 0;

    bool m_QueuesBuilt = false;
    float m_LastAlpha = -1.0f;
    int m_LastWidth = -1;
    int m_LastHeight = -1;
    bool m_IsCapturingProbe = false;

    GPUGlobalData m_GlobalData;
    std::unique_ptr<struct GPUUBO> m_CameraUBO;
    std::unique_ptr<struct GPUUBO> m_GlobalLightUBO;
    std::unique_ptr<struct GPUUBO> m_GlobalDataUBO;
    
    std::shared_ptr<Shader> m_UnlitShader;
    std::shared_ptr<Shader> m_DeferredLitShader;
    std::shared_ptr<Shader> m_DeferredUnlitShader;
    std::shared_ptr<Shader> m_ForwardPBRLitShader;
    std::shared_ptr<Shader> m_ErrorForwardShader;
    std::shared_ptr<Shader> m_ErrorDeferredShader;

    FrustumCuller m_FrustumCuller;
    OcclusionCuller m_OcclusionCuller;
    
    std::unique_ptr<RenderCore> m_RenderCore;
    CommandQueue m_CommandQueue;
    RenderCommandBuffer m_RenderCommandBuffer;
    uint32_t m_MainFBO = 0;
};
