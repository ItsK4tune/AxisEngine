#pragma once

#include <ecs/interface/i_render_service.h>
#include <render/logic/frustum_culler.h>
#include <render/logic/occlusion_culler.h>
#include <render/logic/static_batch_manager.h>
#include <render/type/graphics_types.h>
#include <render/type/render_service_state.h>
#include <render/unit/command_queue.h>
#include <render/unit/render_command.h>
#include <render/unit/render_queue.h>
#include <resource/unit/shader.h>
#include <memory>
#include <string>
#include <vector>

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
    bool IsEnabled() const override
    {
        return m_Flags.enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Flags.enabled = enable;
    }
    std::string GetName() const override
    {
        return "RenderService";
    }
    int GetPriority() const override
    {
        return 0;
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderMain;
    }

    // ICameraService
    glm::vec3 GetCameraPosition() const override
    {
        return m_CameraState.position;
    }
    glm::mat4 GetViewMatrix() const override
    {
        return m_CameraState.viewMatrix;
    }
    glm::mat4 GetProjectionMatrix() const override
    {
        return m_CameraState.projectionMatrix;
    }
    float GetNearPlane() const override
    {
        return m_CameraState.nearPlane;
    }
    float GetFarPlane() const override
    {
        return m_CameraState.farPlane;
    }
    void UploadCameraUBO(const GPUCameraData& camData) override;
    void RestoreCameraState(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float nearPlane,
                            float farPlane) override;

    // IRenderStateService
    AntiAliasingMode GetAntiAliasingMode() const override
    {
        return m_Flags.aaMode;
    }
    void SetAntiAliasingMode(AntiAliasingMode mode) override
    {
        m_Flags.aaMode = mode;
    }
    glm::mat4 GetPrevViewProj() const override
    {
        return m_GlobalState.prevViewProj;
    }
    glm::mat4 GetCurrViewProj() const override
    {
        return m_GlobalState.currViewProj;
    }
    glm::vec2 GetJitterOffset() const override
    {
        return m_GlobalState.jitterOffset;
    }
    bool IsDebugNoTexture() const override
    {
        return m_Flags.debugNoTexture;
    }
    void SetDebugNoTexture(bool enable) override
    {
        m_Flags.debugNoTexture = enable;
    }
    bool IsWireframe() const override
    {
        return m_Flags.wireframe;
    }
    void SetWireframe(bool enable) override
    {
        m_Flags.wireframe = enable;
    }
    bool IsOcclusionCullingEnabled() const override
    {
        return m_Flags.occlusionCullingEnabled;
    }
    void SetOcclusionCulling(bool enable) override
    {
        m_Flags.occlusionCullingEnabled = enable;
    }
    unsigned int GetWhiteTexture() const override;
    unsigned int GetBlackTexture() const override;
    unsigned int GetFlatNormalTexture() const override;
    void UpdateGlobalLightData(const GPUGlobalLightData& data) override;

    // IRenderQueueService
    int GetRenderedCount() const override
    {
        return m_RenderedCount;
    }
    void AddRenderedCount(int count) override
    {
        m_RenderedCount += count;
    }
    uint32_t GetMainFBO() const override
    {
        return m_MainFBO;
    }
    void SetMainFBO(uint32_t fbo) override
    {
        m_MainFBO = fbo;
    }
    int GetLastWidth() const override
    {
        return m_LastWidth;
    }
    int GetLastHeight() const override
    {
        return m_LastHeight;
    }
    StaticBatchManager& GetBatchManager() override
    {
        return m_BatchManager;
    }
    RenderQueue& GetRenderQueueObj() override
    {
        return m_RenderQueueObj;
    }
    void BuildRenderQueues(Scene& scene, float alpha, int width = 0, int height = 0) override;
    void BuildRenderQueuesWithCamera(Scene& scene, const RenderViewParams& params) override;
    void RenderOcclusionQueries(Scene& scene, float alpha) override;
    void ExecuteQueue(const std::vector<RenderItem>& queue, RenderQueuePass pass, ShadowRenderer* shadowRenderer,
                      MaterialRenderer* materialRenderer, Shader* overrideShader = nullptr) override;
    void SubmitCommand(const RenderDrawCommand& cmd) override;
    void FlushCommands() override;

    // IIBLService
    unsigned int GetIrradianceMap() const override
    {
        return m_IBLState.irradianceMap;
    }
    unsigned int GetPrefilterMap() const override
    {
        return m_IBLState.prefilterMap;
    }
    unsigned int GetBrdfLUT() const override
    {
        return m_IBLState.brdfLUT;
    }

    // Service-specific methods not in interface
    void SetFaceCulling(bool enabled, CullMode mode = CullMode::Back);
    void SetDepthTest(bool enabled, CompareFunc func = CompareFunc::Less);
    void SetInstanceBatching(bool enable)
    {
        m_Flags.instanceBatchingEnabled = enable;
    }
    void SetFrustumCulling(bool enable)
    {
        m_Flags.frustumCullingEnabled = enable;
    }
    void SetRenderOrderEnabled(bool enable)
    {
        m_Flags.renderOrderEnabled = enable;
    }
    void SetFilterLayerMask(uint32_t mask)
    {
        m_Flags.filterLayerMask = mask;
    }
    void SetDistanceCulling(float distance)
    {
        m_Flags.distanceCullingSq = distance * distance;
    }
    void SetDistanceCullingSq(float distanceSq)
    {
        m_Flags.distanceCullingSq = distanceSq;
    }

    // For syncing with Update
    void IncrementFrame()
    {
        m_GlobalState.frameIndex++;
    }
    void ResetRenderedCount()
    {
        m_RenderedCount = 0;
    }
    void ResetQueuesBuilt()
    {
        m_QueuesBuilt = false;
    }
    void AddTime(float dt)
    {
        m_GlobalData.time += dt;
        m_GlobalData.deltaTime = dt;
    }
    void BeginFrame(const RenderViewParams& params);

private:
    StaticBatchManager m_BatchManager;
    RenderQueue m_RenderQueueObj;

    int m_RenderedCount = 0;

    RenderGlobalState m_GlobalState;
    RenderCameraState m_CameraState;
    RenderFlagsState m_Flags;
    RenderIBLState m_IBLState;

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
    std::shared_ptr<Shader> m_ForwardPBRLitShadowShader;
    std::shared_ptr<Shader> m_DeferredLitShadowShader;
    std::shared_ptr<Shader> m_ErrorForwardShader;
    std::shared_ptr<Shader> m_ErrorDeferredShader;

    FrustumCuller m_FrustumCuller;
    OcclusionCuller m_OcclusionCuller;

    std::unique_ptr<RenderCore> m_RenderCore;
    CommandQueue m_CommandQueue;
    RenderCommandBuffer m_RenderCommandBuffer;
    uint32_t m_MainFBO = 0;

    IGraphicsContext* m_Context = nullptr;
    ConfigManager* m_ConfigManager = nullptr;
};
