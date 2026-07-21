#pragma once

#include <core/logic/event_manager.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/interface/i_render_runtime_control.h>
#include <render/logic/frustum_culler.h>
#include <render/logic/occlusion_culler.h>
#include <render/type/graphics_types.h>
#include <render/type/render_service_state.h>
#include <render/unit/command_queue.h>
#include <render/unit/render_command.h>
#include <render/unit/render_queue.h>
#include <resource/unit/shader.h>
#include <resource/unit/mesh.h>
#include <memory>
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class RenderCore;
class IGraphicsContext;
class ConfigManager;

class RenderServiceImpl : public IRenderService, public IRenderRuntimeControl
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
    RenderQueue& GetRenderQueueObj() override
    {
        return m_RenderQueueObj;
    }
    void PushRenderViewContext() override;
    bool PopRenderViewContext() override;
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
    void SetFaceCulling(bool enabled, CullMode mode = CullMode::Back) override;
    void SetDepthTest(bool enabled, CompareFunc func = CompareFunc::Less) override;
    void SetInstanceBatching(bool enable) override
    {
        m_Flags.instanceBatchingEnabled = enable;
    }
    void SetFrustumCulling(bool enable) override
    {
        m_Flags.frustumCullingEnabled = enable;
    }
    void SetRenderOrderEnabled(bool enable) override
    {
        m_Flags.renderOrderEnabled = enable;
    }
    void SetFilterLayerMask(uint32_t mask) override
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
    void IncrementFrame() override
    {
        m_GlobalState.frameIndex++;
    }
    void ResetRenderedCount() override
    {
        m_RenderedCount = 0;
    }
    void ResetQueuesBuilt() override
    {
        m_QueuesBuilt = false;
    }
    void AddTime(float dt) override
    {
        m_GlobalData.time += dt;
        m_GlobalData.deltaTime = dt;
    }
    void BeginFrame(const RenderViewParams& params);

private:
    struct RenderViewContext
    {
        RenderQueue queue;
        RenderCameraState cameraState;
        RenderGlobalState globalState;
        GPUGlobalData globalData{};
        int renderedCount = 0;
        int lastWidth = -1;
        int lastHeight = -1;
        float lastAlpha = -1.0f;
        bool queuesBuilt = false;
        bool isCapturingProbe = false;
        std::array<unsigned int, 4> planarTextureIDs{};
        std::array<glm::vec3, 4> planarNormals{};
        int planarReflectionCount = 0;
    };

    RenderQueue m_RenderQueueObj;
    std::vector<std::unique_ptr<RenderViewContext>> m_RenderViewContexts;
    size_t m_RenderViewContextDepth = 0;

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

    std::vector<entt::entity> m_CameraEntitiesCache;
    std::vector<entt::entity> m_ShadowEntitiesCache;
    std::vector<uint32_t> m_CameraVisitedGenerations;
    std::vector<uint32_t> m_ShadowVisitedGenerations;
    uint32_t m_CurrentFrameGeneration = 1;

    struct ProbeData
    {
        ReflectionProbeComponent* component;
        glm::vec3 position;
        entt::entity entity;
    };

    std::vector<ProbeData> m_ProbeCache;
    std::unordered_map<std::string, const ProbeData*> m_ProbeTargetsCache;

    struct CachedPlanarReflection
    {
        unsigned int textureID = 0;
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
    };
    std::array<CachedPlanarReflection, 4> m_PlanarReflectionCache{};
    int m_PlanarReflectionCount = 0;

    void BuildRenderQueuesFromCandidates(Scene& scene, const RenderViewParams& params, float lodFactor,
                                         const std::vector<ProbeData>& probes,
                                         const std::unordered_map<std::string, const ProbeData*>& probesByTarget,
                                         bool candidatesFromOctree, uint32_t cameraGen);

    std::vector<entt::entity> m_CandidatesCache;

    struct OctreeEntryState
    {
        uint32_t worldVersion = 0;
        const Model* model = nullptr;
        AABB worldAABB;
    };
    std::unordered_map<entt::entity, OctreeEntryState> m_OctreeEntryStates;
    std::unordered_set<entt::entity> m_OctreeSeenEntities;
    std::vector<entt::entity> m_DirtyOctreeEntities;
    std::vector<MeshInstanceData> m_InstanceDataScratch;

    std::unique_ptr<RenderCore> m_RenderCore;
    RenderCommandBuffer m_RenderCommandBuffer;
    uint32_t m_MainFBO = 0;

    IGraphicsContext* m_Context = nullptr;
    ConfigManager* m_ConfigManager = nullptr;
    EventSubscriptionList m_EventSubscriptions;
    bool m_IsShutdown = true;

    void UploadCurrentViewState();
};
