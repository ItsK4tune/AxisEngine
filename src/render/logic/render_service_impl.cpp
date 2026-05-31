#include <render/logic/render_service_impl.h>
#include <core/logic/job_system.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <ecs/logic/system_factory.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/reflection_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/ui_components.h>
#include <render/unit/frustum.h>
#include <algorithm>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <core/logic/config_manager.h>
#include <core/logic/event_manager.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <ecs/logic/entity_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_query_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/render_core.h>
#include <resource/logic/resource_manager.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <cstring>

namespace
{
uint64_t CombineHash(uint64_t seed, uint64_t value)
{
    return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

uint64_t HashMaterialForBatch(const AxisMaterialComponent* material)
{
    if (!material)
        return 0;

    const auto& d = material->desc;
    uint64_t h = 1469598103934665603ULL;
    auto hashFloat = [](float value) { return static_cast<uint64_t>(std::hash<float>{}(value)); };
    auto hashString = [](const std::string& value) { return static_cast<uint64_t>(std::hash<std::string>{}(value)); };

    h = CombineHash(h, hashFloat(d.pbr.metallic));
    h = CombineHash(h, hashFloat(d.pbr.roughness));
    h = CombineHash(h, hashFloat(d.pbr.ao));
    h = CombineHash(h, hashFloat(d.opacity));
    h = CombineHash(h, hashFloat(d.emission.x));
    h = CombineHash(h, hashFloat(d.emission.y));
    h = CombineHash(h, hashFloat(d.emission.z));
    h = CombineHash(h, hashFloat(d.uvScale.x));
    h = CombineHash(h, hashFloat(d.uvScale.y));
    h = CombineHash(h, hashFloat(d.uvOffset.x));
    h = CombineHash(h, hashFloat(d.uvOffset.y));
    h = CombineHash(h, hashString(d.albedoPath));
    h = CombineHash(h, hashString(d.normalPath));
    h = CombineHash(h, hashString(d.metallicPath));
    h = CombineHash(h, hashString(d.roughnessPath));
    h = CombineHash(h, hashString(d.aoPath));
    h = CombineHash(h, hashString(d.emissivePath));
    h = CombineHash(h, hashString(d.specularPath));
    h = CombineHash(h, static_cast<uint64_t>(d.blendSrc));
    h = CombineHash(h, static_cast<uint64_t>(d.blendDst));
    h = CombineHash(h, hashString(d.type));
    return h;
}
}  // namespace

#include <platform/logic/io_handler.h>
#include <render/logic/material_renderer.h>
#include <render/logic/render_core.h>
#include <render/logic/shadow_renderer.h>

RenderServiceImpl::RenderServiceImpl()
{
}
RenderServiceImpl::~RenderServiceImpl()
{
}

void RenderServiceImpl::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<IRenderService>(this);
    sl.Register<ICameraService>(this);
    sl.Register<IRenderStateService>(this);
    sl.Register<IRenderQueueService>(this);
    sl.Register<IIBLService>(this);

    auto* context_ptr = sl.Resolve<IGraphicsContext>();
    auto* configMgr_ptr = sl.Resolve<ConfigManager>();
    if (!configMgr_ptr)
        return;  // Critical service

    m_Context = context_ptr;
    m_ConfigManager = configMgr_ptr;

    auto& config = m_ConfigManager->GetConfig();
    auto* shaderLib = sl.Resolve<ResourceManager>();

    // Core settings
    this->SetInstanceBatching(config.instanceBatchingEnabled);
    this->SetFrustumCulling(config.frustumCullingEnabled);
    this->SetOcclusionCulling(config.occlusionCullingEnabled);
    this->SetDistanceCulling(config.distanceCulling);
    this->SetAntiAliasingMode((AntiAliasingMode)config.antialiasing);
    this->SetRenderOrderEnabled(config.renderOrderEnabled);
    this->SetFilterLayerMask(config.filterLayerMask);
    this->SetFaceCulling(config.cullFaceEnabled);
    this->SetDepthTest(config.depthTestEnabled);

    // Skip GPU-dependent initialization if no context
    if (!m_Context)
    {
        LOGGER_INFO("RenderSystem") << "Initialized in HEADLESS mode (Rendering Services dormant)";
        return;
    }

    auto& context = *m_Context;
    auto& shaderLibRef = *shaderLib;

    EventManager::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
        if (!(e.bitmask & (ConfigChangedEvent::Graphics | ConfigChangedEvent::Window | ConfigChangedEvent::All)))
            return;

        const AppConfig& cfg = e.config;
        this->SetInstanceBatching(cfg.instanceBatchingEnabled);
        this->SetFrustumCulling(cfg.frustumCullingEnabled);
        this->SetOcclusionCulling(cfg.occlusionCullingEnabled);
        this->SetDistanceCulling(cfg.distanceCulling);
        this->SetAntiAliasingMode((AntiAliasingMode)cfg.antialiasing);
        this->SetRenderOrderEnabled(cfg.renderOrderEnabled);
        this->SetFilterLayerMask(cfg.filterLayerMask);
        this->SetFaceCulling(cfg.cullFaceEnabled);
        this->SetDepthTest(cfg.depthTestEnabled);
    });

    m_RenderCore = std::make_unique<RenderCore>();
    m_RenderCore->Initialize(m_Context, shaderLib);
    sl.Register<RenderCore>(m_RenderCore.get());

    if (m_Context)
    {
        auto& core = *m_RenderCore;
        auto& bm = m_Context->GetBufferManager();
        m_CameraUBO = std::make_unique<GPUUBO>(*m_Context, bm.CreateBuffer());
        bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
        bm.BufferData(BufferType::UniformBuffer, sizeof(GPUCameraData), nullptr, BufferUsage::DynamicDraw);

        m_GlobalLightUBO = std::make_unique<GPUUBO>(*m_Context, bm.CreateBuffer());
        bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
        bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalLightData), nullptr, BufferUsage::DynamicDraw);

        m_GlobalDataUBO = std::make_unique<GPUUBO>(*m_Context, bm.CreateBuffer());
        bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
        bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalData), nullptr, BufferUsage::DynamicDraw);

        bm.BindBufferBase(BufferType::UniformBuffer, 20, m_CameraUBO->Get());
        bm.BindBufferBase(BufferType::UniformBuffer, 21, m_GlobalLightUBO->Get());
        bm.BindBufferBase(BufferType::UniformBuffer, 22, m_GlobalDataUBO->Get());

        if (shaderLib)
        {
            m_OcclusionCuller.Initialize(m_Context, shaderLib->GetShader("occlusion"));
            m_UnlitShader = shaderLib->GetShader("forward_unlit");
            m_DeferredLitShader = shaderLib->GetShader("deferred_lit");
            m_DeferredUnlitShader = shaderLib->GetShader("deferred_unlit");
            m_ForwardPBRLitShader = shaderLib->GetShader("forward_pbr_lit");
            m_ForwardPBRLitShadowShader = shaderLib->GetShader("forward_pbr_lit_shadow");
            m_DeferredLitShadowShader = shaderLib->GetShader("deferred_lit_shadow");
            m_ErrorForwardShader = shaderLib->GetShader("error_forward");
            m_ErrorDeferredShader = shaderLib->GetShader("error_deferred");
        }
    }
}

void RenderServiceImpl::Shutdown()
{
    LOGGER_INFO("RenderSystem") << "Shutting down RenderSystem";
    m_OcclusionCuller.Shutdown();
    if (m_RenderCore)
        m_RenderCore->Shutdown();
}

void RenderServiceImpl::SetFaceCulling(bool enabled, CullMode mode)
{
    if (!m_Context)
        return;
    auto& rsm = m_Context->GetRenderStateManager();

    if (enabled)
    {
        rsm.Enable(ServerCapability::CullFace);
        rsm.SetCullFace(mode);
    }
    else
    {
        rsm.Disable(ServerCapability::CullFace);
    }
}

void RenderServiceImpl::SetDepthTest(bool enabled, CompareFunc func)
{
    if (!m_Context)
        return;
    auto& rsm = m_Context->GetRenderStateManager();

    if (enabled)
    {
        rsm.Enable(ServerCapability::DepthTest);
        rsm.SetCullFace(CullMode::Back);
        rsm.SetDepthFunc(CompareFunc::Less);
        rsm.SetDepthMask(true);
        rsm.SetDepthFunc(func);
    }
    else
    {
        rsm.Disable(ServerCapability::DepthTest);
    }
}

void RenderServiceImpl::BuildRenderQueues(Scene& scene, float alpha, int width, int height)
{
    if (!m_Context)
        return;
    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null)
    {
        if (!m_QueuesBuilt)
        {
            LOGGER_WARN("RenderSystem") << "BuildRenderQueues: No active camera found!";
            m_QueuesBuilt = true;
            m_LastAlpha = alpha;
        }
        return;
    }

    CameraComponent& cam = scene.registry.get<CameraComponent>(camEntity);
    PositionComponent* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 pos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    RenderViewParams params;
    params.view = cam.viewMatrix;
    params.projection = cam.projectionMatrix;
    params.cameraPos = pos;
    params.nearPlane = cam.nearPlane;
    params.farPlane = cam.farPlane;
    params.lodFactor = alpha;
    params.width = width;
    params.height = height;
    params.cullingMask = cam.cullingMask;
    BuildRenderQueuesWithCamera(scene, params);
}

void RenderServiceImpl::BeginFrame(const RenderViewParams& params)
{
    m_CameraState.position = params.cameraPos;
    m_CameraState.viewMatrix = params.view;
    m_CameraState.projectionMatrix = params.projection;

    m_CameraState.nearPlane = params.nearPlane;
    m_CameraState.farPlane = params.farPlane;

    if (params.width <= 0 || params.height <= 0)
        return;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            if (!std::isfinite(params.projection[i][j]) || !std::isfinite(params.view[i][j]))
            {
                LOGGER_ERROR("RenderSystem") << "NaN detected in camera matrices. Skipping frame.";
                return;
            }
        }
    }

    if (!m_QueuesBuilt)
    {
        m_GlobalState.prevViewProj = m_GlobalState.currViewProj;
    }

    m_GlobalState.jitteredProjection = params.projection;
    m_GlobalState.jitterOffset = glm::vec2(0.0f);

    if (m_Flags.aaMode == AntiAliasingMode::TAA && params.width > 512)
    {
        auto HaltonSequence = [](int index, int base) -> float {
            float result = 0.0f;
            float f = 1.0f;
            int i = index;
            while (i > 0)
            {
                f = f / base;
                result = result + f * (i % base);
                i = i / base;
            }
            return result;
        };
        const int sampleCount = 8;
        int frameIdx = m_GlobalState.frameIndex % sampleCount;
        float jitterX = HaltonSequence(frameIdx + 1, 2) - 0.5f;
        float jitterY = HaltonSequence(frameIdx + 1, 3) - 0.5f;
        m_GlobalState.jitterOffset = glm::vec2(jitterX, jitterY);

        glm::mat4 jitterMatrix = glm::mat4(1.0f);
        jitterMatrix[3][0] = jitterX * 2.0f / (float)params.width;
        jitterMatrix[3][1] = jitterY * 2.0f / (float)params.height;
        m_GlobalState.jitteredProjection = jitterMatrix * m_GlobalState.jitteredProjection;

        if (!m_QueuesBuilt)
            m_GlobalState.frameIndex++;
    }

    m_GlobalState.currViewProj = m_GlobalState.jitteredProjection * params.view;

    bool stable = true;
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            if (!std::isfinite(m_GlobalState.currViewProj[i][j]))
                stable = false;
        }
    }
    if (!stable)
    {
        m_GlobalState.currViewProj = params.projection * params.view;
    }

    GPUCameraData camData;
    std::memcpy(camData.projection, &m_GlobalState.jitteredProjection[0][0], 16 * sizeof(float));
    std::memcpy(camData.view, &params.view[0][0], 16 * sizeof(float));
    std::memcpy(camData.viewPos, &params.cameraPos[0], 3 * sizeof(float));
    camData.viewPos[3] = 1.0f;

    glm::mat4 invProj = glm::inverse(m_GlobalState.jitteredProjection);
    glm::mat4 invView = glm::inverse(params.view);
    std::memcpy(camData.invProjection, &invProj[0][0], 16 * sizeof(float));
    std::memcpy(camData.invView, &invView[0][0], 16 * sizeof(float));

    glm::mat4 invStableProj = glm::inverse(params.projection);
    std::memcpy(camData.stableProjection, &params.projection[0][0], 16 * sizeof(float));
    std::memcpy(camData.invStableProjection, &invStableProj[0][0], 16 * sizeof(float));

    if (!m_Context)
        return;
    auto& bm = m_Context->GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUCameraData), &camData);

    m_GlobalData.resolution[0] = (float)params.width;
    m_GlobalData.resolution[1] = (float)params.height;
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalData), &m_GlobalData);
}

void RenderServiceImpl::BuildRenderQueuesWithCamera(Scene& scene, const RenderViewParams& params)
{
    m_IsCapturingProbe = params.isCapturingProbe;
    float lodFactor = params.lodFactor;
    if (lodFactor <= 0.0f || lodFactor > 1.0f)
    {
        // LOGGER_WARN("RenderSystem") << "Invalid alpha value: " << lodFactor;
    }

    IGraphicsContext& context = *m_Context;
    m_LastWidth = params.width;
    m_LastHeight = params.height;

    BeginFrame(params);

    glm::mat4 stableVP = params.projection * params.view;
    m_FrustumCuller.BuildFrustum(stableVP);

    if (m_Flags.occlusionCullingEnabled)
    {
        m_OcclusionCuller.UpdateResults(scene);
    }

    m_RenderQueueObj.Clear();
    m_RenderedCount = 0;

    // Pre-collect reflection probes for fast lookup
    struct ProbeData
    {
        ReflectionProbeComponent* component;
        glm::vec3 position;
    };
    std::vector<ProbeData> probes;
    auto probeView = scene.registry.view<PositionComponent, ReflectionProbeComponent>();
    for (auto entity : probeView)
    {
        auto [pos, probe] = probeView.get<PositionComponent, ReflectionProbeComponent>(entity);
        probes.push_back({&probe, pos.value});
    }

    auto renderView = scene.registry.view<WorldTransformComponent, MeshRendererComponent, InfoComponent>();

    for (auto entity : renderView)
    {
        if (entity == params.excludeEntity)
            continue;

        auto& info = renderView.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        uint32_t layer = info.layer;
        if ((m_Flags.filterLayerMask & layer) == 0 || (params.cullingMask & layer) == 0)
            continue;

        auto& world = renderView.get<WorldTransformComponent>(entity);
        glm::mat4 modelMatrix = (lodFactor >= 0.9999f) ? world.worldMatrix : world.GetInterpolated(lodFactor);

        auto& renderer = renderView.get<MeshRendererComponent>(entity);
        if (!renderer.model)
            continue;

        const bool depthOverlay = renderer.ignoreDepth;
        const bool castsSceneShadow = renderer.castShadow && !depthOverlay;

        float distSqResult = glm::length2(params.cameraPos - glm::vec3(modelMatrix[3]));
        bool visibleToCamera = true;
        if (!depthOverlay && m_Flags.distanceCullingSq > 0.0f && distSqResult > m_Flags.distanceCullingSq)
            visibleToCamera = false;

        AABB worldAABB = renderer.model->aabb.Transform(modelMatrix);

        // Skip objects that contain the probe (Self-occlusion fix using AABB)
        if (m_IsCapturingProbe && worldAABB.Contains(params.cameraPos))
            continue;

        if (m_Flags.frustumCullingEnabled && !m_FrustumCuller.IsVisible(worldAABB.minBound, worldAABB.maxBound))
        {
            visibleToCamera = false;
        }

        if (!visibleToCamera && !castsSceneShadow && !depthOverlay)
            continue;

        Model* activeModel = renderer.model.get();
        Shader* itemShader = renderer.shader.lock().get();

        if (auto* lod = scene.registry.try_get<LODComponent>(entity))
        {
            for (int j = 0; j < (int)lod->lodDistancesSq.size(); ++j)
            {
                if (distSqResult > lod->lodDistancesSq[j] && j < (int)lod->lodModels.size() && lod->lodModels[j])
                {
                    activeModel = lod->lodModels[j].get();
                }
                else
                    break;
            }
        }

        bool visibleByOcclusion = true;
        if (m_Flags.occlusionCullingEnabled)
        {
            if (auto* occ = scene.registry.try_get<OcclusionComponent>(entity))
            {
                if (!occ->isVisible)
                    visibleByOcclusion = false;
            }
        }

        if (!visibleByOcclusion && !castsSceneShadow && !depthOverlay)
            continue;

        bool isTransparent = false;
        auto* material = scene.registry.try_get<AxisMaterialComponent>(entity);
        if (material && material->desc.opacity < 1.0f)
            isTransparent = true;

        auto* reflection = scene.registry.try_get<ReflectiveComponent>(entity);

        uint64_t materialBatchKey = HashMaterialForBatch(material);
        uint64_t key = 0;
        uint64_t sId = itemShader ? itemShader->getID() : 0;
        if (renderer.ignoreDepth)
        {
            uint64_t l = (uint64_t)(layer & 0xFF) << 56;
            uint64_t invertedOrder = (uint64_t)(255 - glm::clamp(renderer.order, 0, 255));
            uint64_t o = invertedOrder << 48;
            uint64_t e = (uint64_t)((uint32_t)entity) & 0x0000FFFFFFFFFFFFULL;
            key = l | o | e;
        }
        else if (!isTransparent)
        {
            uint64_t l = (uint64_t)(layer & 0xFF) << 56;
            uint64_t o = (uint64_t)(renderer.order & 0xFF) << 48;
            uint64_t s = (uint64_t)(sId & 0xFFFF) << 32;
            uint64_t m = (materialBatchKey & 0xFFFF) << 16;
            uint64_t mod = (uint64_t)((uintptr_t)activeModel & 0xFF) << 8;
            uint64_t d = (uint64_t)(glm::clamp(distSqResult * 0.1f, 0.0f, 255.0f)) & 0xFF;
            key = l | o | s | m | mod | d;
        }
        else
        {
            uint64_t l = (uint64_t)(layer & 0xFF) << 56;
            uint64_t o = (uint64_t)(renderer.order & 0xFF) << 48;
            float invDepth = 1000000.0f - distSqResult;
            if (invDepth < 0)
                invDepth = 0;
            uint64_t d = (uint64_t)(invDepth) & 0x0000FFFFFFFFFFFFULL;
            key = l | o | d;
        }

        RenderItem item;
        item.entityId = (uint32_t)entity;
        item.model = activeModel;
        item.shader = itemShader;
        item.material = material;
        item.materialBatchKey = materialBatchKey;
        item.reflection = (reflection && reflection->enabled) ? reflection : nullptr;

        // Reflection Probe Selection
        if (item.reflection)
        {
            bool probeFound = false;

            // 1. Try Target Probe (Name or Name:Tag)
            if (!item.reflection->targetProbe.empty())
            {
                std::string target = item.reflection->targetProbe;
                size_t colon = target.find(':');
                std::string targetName = (colon == std::string::npos) ? target : target.substr(0, colon);
                std::string targetTag = (colon == std::string::npos) ? "" : target.substr(colon + 1);

                for (auto probeEntity : probeView)
                {
                    auto* info = scene.registry.try_get<InfoComponent>(probeEntity);
                    if (info && (info->name == targetName))
                    {
                        if (targetTag.empty() || info->tag == targetTag)
                        {
                            auto [pPos, pComp] =
                                probeView.get<PositionComponent, ReflectionProbeComponent>(probeEntity);
                            item.probe = &pComp;
                            item.probePos = pPos.value;
                            probeFound = true;
                            break;
                        }
                    }
                }
            }

            // 2. Fallback to Nearest Probe
            if (!probeFound)
            {
                float minProbeDistSq = 1e30f;
                for (auto& p : probes)
                {
                    float d = glm::distance2(glm::vec3(modelMatrix[3]), p.position);
                    if (d < minProbeDistSq)
                    {
                        minProbeDistSq = d;
                        item.probe = p.component;
                        item.probePos = p.position;
                        probeFound = true;
                    }
                }
            }

            // Standardize Intensity (since radius is removed)
            item.reflectionIntensity = 1.0f;
            if (item.probe)
                item.probeIndex = item.probe->lastGpuIndex;
        }

        item.worldMatrix = modelMatrix;
        item.worldAABB = worldAABB;
        item.layer = layer;
        item.renderOrder = renderer.order;
        item.distanceSq = distSqResult;
        item.isTransparent = isTransparent;
        item.sortKey = key;
        item.castShadow = renderer.castShadow;
        item.receiveShadow = renderer.receiveShadow;
        item.ignoreDepth = renderer.ignoreDepth;
        item.renderMode = renderer.renderMode;

        item.tintColor = renderer.color;
        bool hasAnimation = false;
        if (auto* anim = scene.registry.try_get<AnimationComponent>(entity))
        {
            if (anim->animator)
            {
                hasAnimation = true;
                item.hasAnimation = true;
                item.boneMatrices = &anim->animator->GetFinalBoneMatrices();
            }
        }

        bool hasPhysic = false;
        bool isStaticPhysic = true;
        if (auto* rb = scene.registry.try_get<RigidBodyComponent>(entity))
        {
            hasPhysic = true;
            if (rb->body)
            {
                isStaticPhysic = rb->body->IsStatic() && !rb->body->IsKinematic();
            }
        }

        // Bake mode criteria: No animation, and no physics types other than static.
        item.isStatic = (!hasAnimation) && (!hasPhysic || (hasPhysic && isStaticPhysic));
        const bool addToCameraQueues = visibleToCamera && (visibleByOcclusion || depthOverlay);
        if (addToCameraQueues)
        {
            if (isTransparent)
            {
                m_RenderQueueObj.AddTransparent(item);
            }
            else
            {
                if (renderer.ignoreDepth)
                {
                    m_RenderQueueObj.AddDepthOverlay(item);
                }
                else if (renderer.renderMode == RenderMode::ForceForward)
                {
                    m_RenderQueueObj.AddForwardOpaque(item);
                }
                else
                {
                    m_RenderQueueObj.AddDeferredOpaque(item);
                }
            }
        }
        if (castsSceneShadow)
            m_RenderQueueObj.AddShadow(item);
    }

    // Collect Lights
    auto dirView = scene.registry.view<DirectionalLightComponent, InfoComponent>();
    for (auto entity : dirView)
    {
        auto& info = dirView.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        auto& light = dirView.get<DirectionalLightComponent>(entity);
        if (!light.active)
            continue;
        RenderLight rl;
        rl.type = RenderLightType::Directional;
        rl.color = light.color;
        rl.intensity = light.intensity;
        rl.castShadows = light.isCastShadow;
        rl.ambient = glm::vec3(light.ambient);
        rl.diffuse = glm::vec3(light.diffuse);
        rl.specular = glm::vec3(light.specular);
        rl.direction = light.direction;
        if (auto* rot = scene.registry.try_get<RotationComponent>(entity))
            rl.direction = rot->value * glm::vec3(0, -1, 0);
        if (auto* w = scene.registry.try_get<WorldTransformComponent>(entity))
            rl.version = w->version;
        m_RenderQueueObj.AddLight(std::move(rl));
    }

    auto isLightVolumeVisible = [this](const glm::vec3& position, float radius) {
        if (!m_Flags.frustumCullingEnabled || radius <= 0.0f)
            return true;

        glm::vec3 extent(radius);
        return m_FrustumCuller.IsVisible(position - extent, position + extent);
    };

    auto pointView = scene.registry.view<PointLightComponent, PositionComponent, InfoComponent>();
    for (auto entity : pointView)
    {
        auto& info = pointView.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        auto [light, pos] = pointView.get<PointLightComponent, PositionComponent>(entity);
        if (!light.active)
            continue;
        if (!isLightVolumeVisible(pos.value, light.radius))
            continue;
        RenderLight rl;
        rl.type = RenderLightType::Point;
        rl.position = pos.value;
        rl.color = light.color;
        rl.intensity = light.intensity;
        rl.range = light.radius;
        rl.constant = light.constant;
        rl.linear = light.linear;
        rl.quadratic = light.quadratic;
        rl.ambient = glm::vec3(light.ambient);
        rl.diffuse = glm::vec3(light.diffuse);
        rl.specular = glm::vec3(light.specular);
        rl.castShadows = light.isCastShadow;
        if (auto* w = scene.registry.try_get<WorldTransformComponent>(entity))
            rl.version = w->version;
        m_RenderQueueObj.AddLight(std::move(rl));
    }

    auto spotView = scene.registry.view<SpotLightComponent, PositionComponent, RotationComponent, InfoComponent>();
    for (auto entity : spotView)
    {
        auto& info = spotView.get<InfoComponent>(entity);
        if (!info.isActive)
            continue;

        auto [light, pos, rot] = spotView.get<SpotLightComponent, PositionComponent, RotationComponent>(entity);
        if (!light.active)
            continue;
        if (!isLightVolumeVisible(pos.value, light.radius))
            continue;
        RenderLight rl;
        rl.type = RenderLightType::Spot;
        rl.position = pos.value;
        rl.direction = rot.value * glm::vec3(0, -1, 0);
        rl.color = light.color;
        rl.intensity = light.intensity;
        rl.constant = light.constant;
        rl.linear = light.linear;
        rl.quadratic = light.quadratic;
        rl.range = light.radius;
        rl.innerCutoff = light.cutOff;
        rl.outerCutoff = light.outerCutOff;
        rl.ambient = glm::vec3(light.ambient);
        rl.diffuse = glm::vec3(light.diffuse);
        rl.specular = glm::vec3(light.specular);
        rl.castShadows = light.isCastShadow;
        if (auto* w = scene.registry.try_get<WorldTransformComponent>(entity))
            rl.version = w->version;
        m_RenderQueueObj.AddLight(std::move(rl));
    }

    m_RenderQueueObj.Sort();

    // Collect Skybox
    m_IBLState.irradianceMap = 0;
    m_IBLState.prefilterMap = 0;
    m_IBLState.brdfLUT = 0;
    auto skyView = scene.registry.view<SkyboxRenderComponent>();
    for (auto skyEnt : skyView)
    {
        auto& skyComp = skyView.get<SkyboxRenderComponent>(skyEnt);
        if (skyComp.isPrimary && skyComp.skybox)
        {
            m_IBLState.irradianceMap = skyComp.irradianceMap;
            m_IBLState.prefilterMap = skyComp.prefilterMap;
            m_IBLState.brdfLUT = skyComp.brdfLUT;
            break;
        }
    }

    static bool firstFrame = true;
    if (firstFrame && !m_IsCapturingProbe)
    {
        LOGGER_INFO("RenderSystem") << "BuildRenderQueues: Opaque="
                                    << (int)(m_RenderQueueObj.GetDeferredOpaqueQueue().size() +
                                             m_RenderQueueObj.GetForwardOpaqueQueue().size())
                                    << ", DepthOverlay=" << (int)m_RenderQueueObj.GetDepthOverlayQueue().size()
                                    << ", Transparent=" << (int)m_RenderQueueObj.GetTransparentQueue().size();
        firstFrame = false;
    }

    m_QueuesBuilt = true;
    m_LastAlpha = lodFactor;
    m_LastWidth = params.width;
    m_LastHeight = params.height;
}

void RenderServiceImpl::ExecuteQueue(const std::vector<RenderItem>& queue, bool isTransparentPass,
                                     ShadowRenderer* shadowRenderer, MaterialRenderer* materialRenderer,
                                     Shader* overrideShader)
{
    if (queue.empty())
        return;

    if (m_RenderCore)
    {
        auto& core = *m_RenderCore;
        materialRenderer = &core.GetMaterialRenderer();
    }

    if (!m_Context)
        return;
    auto& tm = m_Context->GetTextureManager();
    auto& rtm = m_Context->GetRenderTargetManager();
    auto& rsm = m_Context->GetRenderStateManager();

    Shader* lastShader = nullptr;

    RenderSceneData sceneData;
    sceneData.cameraPosition = m_CameraState.position;
    sceneData.viewMatrix = m_CameraState.viewMatrix;
    sceneData.projMatrix = m_CameraState.projectionMatrix;
    sceneData.nearPlane = m_CameraState.nearPlane;
    sceneData.farPlane = m_CameraState.farPlane;
    sceneData.irradianceMap = m_IBLState.irradianceMap;
    sceneData.prefilterMap = m_IBLState.prefilterMap;
    sceneData.brdfLUT = m_IBLState.brdfLUT;

    m_RenderedCount += (int)queue.size();

    // --- Pre-collect planar reflection data ONCE before the draw loop ---
    struct PlanarEntry
    {
        unsigned int textureID;
        glm::vec3 normal;
    };
    PlanarEntry planarEntries[4];
    int planarCount = 0;
    char planarTexNames[4][48];
    char planarNormNames[4][48];

    if (!m_IsCapturingProbe)
    {
        auto& scene = ServiceLocator::Instance().Require<Scene>();
        auto planarView = scene.registry.view<PlanarReflectionComponent>();
        for (auto planarEntity : planarView)
        {
            auto& prc = planarView.get<PlanarReflectionComponent>(planarEntity);
            if (prc.reflectionTextureID && prc.isRendered)
            {
                planarEntries[planarCount] = {prc.reflectionTextureID, prc.normal};
                snprintf(planarTexNames[planarCount], sizeof(planarTexNames[0]), "u_PlanarReflections[%d]",
                         planarCount);
                snprintf(planarNormNames[planarCount], sizeof(planarNormNames[0]), "u_PlanarNormals[%d]", planarCount);
                planarCount++;
                if (planarCount >= 4)
                    break;
            }
        }
    }

    // --- Pre-compute per-frame constants ---
    glm::vec2 screenSize((float)m_LastWidth, (float)m_LastHeight);
    bool isCapturing = m_IsCapturingProbe;

    // --- Reflection state tracking to skip redundant uniform sets ---
    bool lastHadReflection = false;
    bool lastHadProbe = false;
    unsigned int lastProbeCubemap = 0;

    std::vector<glm::mat4> instanceMatrices;
    instanceMatrices.reserve(1024);

    for (size_t itemIndex = 0; itemIndex < queue.size(); ++itemIndex)
    {
        const auto& item = queue[itemIndex];
        Model* model = item.model;
        AxisMaterialComponent* material = item.material;
        Shader* shader = item.shader;
        bool ignoreDepthForDraw = item.ignoreDepth && !overrideShader;

        if (overrideShader)
        {
            shader = overrideShader;
        }
        else
        {
            if (!shader)
            {
                if (isTransparentPass || item.renderMode == RenderMode::ForceForward)
                {
                    shader = m_ForwardPBRLitShader.get();
                }
                else
                {
                    shader = isCapturing ? m_ForwardPBRLitShader.get() : m_DeferredLitShader.get();
                }
            }

            // Capture passes can route deferred albedo directly into Color0 below; keep deferred shaders intact here.
            bool targetIsForward = isTransparentPass || item.renderMode == RenderMode::ForceForward || item.ignoreDepth;
            if (shader)
            {
                if (targetIsForward)
                {
                    if (shader->IsDeferred())
                    {
                        if (shader == m_DeferredUnlitShader.get() || shader->GetName() == "deferred_unlit")
                        {
                            shader = m_UnlitShader.get();
                        }
                        else if (shader == m_DeferredLitShadowShader.get() ||
                                 shader->GetName() == "deferred_lit_shadow")
                        {
                            shader = m_ForwardPBRLitShadowShader.get();
                        }
                        else
                        {
                            shader = m_ForwardPBRLitShader.get();
                        }
                    }
                }
                else
                {
                    if (!shader->IsDeferred())
                    {
                        if (shader == m_UnlitShader.get() || shader->GetName() == "forward_unlit")
                        {
                            shader = m_DeferredUnlitShader.get();
                        }
                        else if (shader == m_ForwardPBRLitShader.get() || shader->GetName() == "forward_pbr_lit")
                        {
                            shader = m_DeferredLitShader.get();
                        }
                        else if (shader == m_ForwardPBRLitShadowShader.get() ||
                                 shader->GetName() == "forward_pbr_lit_shadow")
                        {
                            shader = m_DeferredLitShadowShader.get();
                        }
                    }
                }
            }
        }

        if (!shader || !model)
        {
            shader = isTransparentPass ? m_ErrorForwardShader.get() : m_ErrorDeferredShader.get();
            if (!shader || !model)
                continue;
        }

        if (shader != lastShader)
        {
            shader->use();
            lastShader = shader;

            if (shadowRenderer && shadowRenderer->IsShadowsEnabled())
            {
                auto& shadow = shadowRenderer->GetShadow();
                shadow.BindTexture_Dir(0, 10);
                shadow.BindTexture_Point(0, 11);
                shadow.BindTexture_Spot(0, 12);
                shader->setInt("u_ShadowMapDir", 10);
                shader->setInt("u_ShadowMapPoint", 11);
                shader->setInt("u_ShadowMapSpot", 12);
                shader->setFloat("u_ShadowBias", shadowRenderer->GetShadowBias());
                shader->setInt("u_ShadowSoftness", shadowRenderer->GetShadowSoftness());
            }

            // Per-frame uniforms: set once per shader bind, not per item
            shader->setBool("u_ProbeUnlit", isCapturing);
            shader->setVec2("u_ScreenSize", screenSize);

            // Bind pre-collected planar reflections once per shader bind
            if (!isCapturing)
            {
                for (int p = 0; p < planarCount; ++p)
                {
                    tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture19) + p));
                    tm.BindTexture(TextureType::Texture2D, planarEntries[p].textureID);
                    shader->setInt(planarTexNames[p], 19 + p);
                    shader->setVec3(planarNormNames[p], planarEntries[p].normal);
                }
            }
            shader->setInt("u_PlanarCount", planarCount);

            // Reset reflection tracking on shader change
            lastHadReflection = false;
            lastHadProbe = false;
            lastProbeCubemap = 0;
        }

        glm::mat4 mtx = item.worldMatrix;
        if (item.hasAnimation && item.boneMatrices)
        {
            shader->setMat4Array("u_FinalBonesMatrices", *item.boneMatrices);
        }
        else
        {
            mtx *= model->GetRootTransform();
        }

        shader->setMat4_Fast(shader->m_Loc_u_Model, "u_Model", mtx);
        shader->setVec4_Fast(shader->m_Loc_u_TintColor, "u_TintColor", item.tintColor);
        shader->setUInt_Fast(shader->m_Loc_u_EntityID, "u_EntityID", item.entityId);
        shader->setBool_Fast(shader->m_Loc_u_IsInstanced, "u_IsInstanced", false);
        shader->setFloat_Fast(shader->m_Loc_u_ReceiveShadowFlag, "u_ReceiveShadowFlag", item.receiveShadow ? 1.0f : 0.0f);

        if (item.probeIndex >= 0)
        {
            shader->setInt_Fast(shader->m_Loc_u_ProbeIndex, "u_ProbeIndex", item.probeIndex);
        }

        // Reflection Probe Binding — only set uniforms on state transitions
        bool curHasReflection = (item.reflection && item.probe && !isCapturing);
        unsigned int curProbeCubemap = curHasReflection ? item.probe->cubemapID : 0;

        if (curHasReflection)
        {
            if (curProbeCubemap != lastProbeCubemap)
            {
                tm.ActiveTexture(TextureUnit::Texture15);
                tm.BindTexture(TextureType::TextureCubeMap, curProbeCubemap);
            }
            if (!lastHadReflection)
            {
                shader->setInt_Fast(shader->m_Loc_u_ReflectionProbe, "u_ReflectionProbe", 15);
                shader->setBool_Fast(shader->m_Loc_u_HasReflection, "u_HasReflection", true);
                shader->setBool_Fast(shader->m_Loc_u_HasProbe, "u_HasProbe", true);
            }
            shader->setVec3_Fast(shader->m_Loc_u_ProbePos, "u_ProbePos", item.probePos);
            shader->setVec3_Fast(shader->m_Loc_u_ProbeBoxMin, "u_ProbeBoxMin", item.probe->boxMin);
            shader->setVec3_Fast(shader->m_Loc_u_ProbeBoxMax, "u_ProbeBoxMax", item.probe->boxMax);
        }
        else if (lastHadReflection && !isCapturing)
        {
            // State transition: had reflection -> no reflection. Cleanup once.
            tm.ActiveTexture(TextureUnit::Texture15);
            tm.BindTexture(TextureType::TextureCubeMap, 0);
            shader->setBool_Fast(shader->m_Loc_u_HasReflection, "u_HasReflection", false);
            shader->setBool_Fast(shader->m_Loc_u_HasProbe, "u_HasProbe", false);
            shader->setInt("u_ProbeCount", 0);
        }

        lastHadReflection = curHasReflection;
        lastProbeCubemap = curProbeCubemap;

        // Dynamic Mapping for Reflection Probes (Zero-Code Re-routing)
        if (isCapturing)
        {
            if (shader->IsDeferred())
            {
                FramebufferAttachment atts[] = {FramebufferAttachment::None, FramebufferAttachment::None,
                                                FramebufferAttachment::Color0};
                rtm.DrawBuffers(3, atts);
            }
            else
            {
                FramebufferAttachment atts[] = {FramebufferAttachment::Color0};
                rtm.DrawBuffers(1, atts);
            }
        }

        // Reflection data uniforms — only when item has reflection
        if (item.reflection && !isCapturing)
        {
            shader->setFloat_Fast(shader->m_Loc_u_Reflectivity, "u_Reflectivity", item.reflection->reflectivity);
            shader->setFloat_Fast(shader->m_Loc_u_FresnelPower, "u_FresnelPower", item.reflection->fresnelPower);
            shader->setFloat_Fast(shader->m_Loc_u_FresnelBias, "u_FresnelBias", item.reflection->fresnelBias);

            if (item.probe && item.probe->cubemapID != 0)
            {
                shader->setFloat_Fast(shader->m_Loc_u_ReflectionIntensity, "u_ReflectionIntensity", item.reflectionIntensity);
            }
        }
        else if (!isCapturing)
        {
            // Only set defaults if previous item had reflection (avoid redundant sets)
            if (lastHadProbe)
            {
                shader->setFloat_Fast(shader->m_Loc_u_Reflectivity, "u_Reflectivity", 0.0f);
                shader->setFloat_Fast(shader->m_Loc_u_FresnelPower, "u_FresnelPower", 5.0f);
                shader->setFloat_Fast(shader->m_Loc_u_FresnelBias, "u_FresnelBias", 0.04f);
                shader->setBool_Fast(shader->m_Loc_u_HasProbe, "u_HasProbe", false);
            }
        }
        lastHadProbe = (item.reflection != nullptr);

        bool matBound = materialRenderer->SetupMaterialUniforms(shader, material, sceneData, item.tintColor,
                                                                m_Flags.debugNoTexture, m_Flags.wireframe);

        auto canInstanceWith = [&](const RenderItem& other) {
            return m_Flags.instanceBatchingEnabled && !isTransparentPass && !overrideShader && !item.hasAnimation &&
                   !other.hasAnimation && !item.reflection && !other.reflection && item.probeIndex < 0 &&
                   other.probeIndex < 0 && other.model == item.model && other.shader == item.shader &&
                   other.materialBatchKey == item.materialBatchKey && other.renderMode == item.renderMode &&
                   other.tintColor == item.tintColor && other.castShadow == item.castShadow &&
                   other.receiveShadow == item.receiveShadow && other.ignoreDepth == item.ignoreDepth;
        };

        size_t batchCount = 1;
        if (canInstanceWith(item))
        {
            instanceMatrices.clear();
            instanceMatrices.push_back(mtx);
            for (size_t nextIndex = itemIndex + 1; nextIndex < queue.size(); ++nextIndex)
            {
                const auto& next = queue[nextIndex];
                if (!canInstanceWith(next))
                    break;

                glm::mat4 nextMtx = next.worldMatrix;
                nextMtx *= next.model->GetRootTransform();
                instanceMatrices.push_back(nextMtx);
                ++batchCount;
            }
        }

        if (batchCount > 1)
        {
            if (ignoreDepthForDraw)
            {
                rsm.Disable(ServerCapability::DepthTest);
                rsm.SetDepthMask(false);
            }

            shader->setBool_Fast(shader->m_Loc_u_IsInstanced, "u_IsInstanced", true);
            shader->setUInt_Fast(shader->m_Loc_u_EntityID, "u_EntityID", 0);
            model->DrawInstanced(*shader, instanceMatrices, !matBound);
            shader->setBool_Fast(shader->m_Loc_u_IsInstanced, "u_IsInstanced", false);
            itemIndex += batchCount - 1;
        }
        else
        {
            if (ignoreDepthForDraw)
            {
                rsm.Disable(ServerCapability::DepthTest);
                rsm.SetDepthMask(false);
            }

            model->Draw(*shader, !matBound);
        }

        if (ignoreDepthForDraw)
        {
            rsm.Enable(ServerCapability::DepthTest);
            rsm.SetDepthFunc(CompareFunc::Less);
            rsm.SetDepthMask(!isTransparentPass);
        }

        // Restore DrawBuffers mapping after draw
        if (isCapturing)
        {
            FramebufferAttachment atts[] = {FramebufferAttachment::Color0};
            rtm.DrawBuffers(1, atts);
        }
    }
}

unsigned int RenderServiceImpl::GetWhiteTexture() const
{
    return m_RenderCore ? m_RenderCore->GetWhiteTexture() : 0;
}
unsigned int RenderServiceImpl::GetBlackTexture() const
{
    return m_RenderCore ? m_RenderCore->GetBlackTexture() : 0;
}
unsigned int RenderServiceImpl::GetFlatNormalTexture() const
{
    return m_RenderCore ? m_RenderCore->GetFlatNormalTexture() : 0;
}

void RenderServiceImpl::UpdateGlobalLightData(const GPUGlobalLightData& data)
{
    if (!m_Context)
        return;
    auto& bm = m_Context->GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalLightData), &data);
}

void RenderServiceImpl::SubmitCommand(const RenderDrawCommand& cmd)
{
    m_RenderCommandBuffer.Submit(cmd);
}

void RenderServiceImpl::FlushCommands()
{
    if (!m_Context)
        return;

    m_RenderCommandBuffer.Sort();
    const auto& commands = m_RenderCommandBuffer.GetCommands();

    if (commands.empty())
        return;

    auto& context = *m_Context;
    auto& rsm = context.GetRenderStateManager();
    auto& bm = context.GetBufferManager();
    auto& dc = context.GetDrawContext();
    auto& tm = context.GetTextureManager();

    uint32_t lastShader = 0;
    uint32_t lastVAO = 0;

    for (const auto& cmd : commands)
    {
        if (cmd.shaderId != lastShader)
        {
            if (cmd.shader)
                cmd.shader->use();
            lastShader = cmd.shaderId;
        }

        if (cmd.vao != lastVAO)
        {
            bm.BindVertexArray(cmd.vao);
            lastVAO = cmd.vao;
        }

        if (cmd.shader)
        {
            cmd.shader->setMat4("u_Model", cmd.modelMatrix);
            if (cmd.texture0 != 0)
            {
                tm.ActiveTexture(TextureUnit::Texture0);
                tm.BindTexture(TextureType::Texture2D, cmd.texture0);
                cmd.shader->setInt("u_Texture0", 0);
            }
            cmd.shader->setVec4("u_TintColor", cmd.tintColor);

            for (int u = 0; u < cmd.uniformCount; ++u)
            {
                const auto& slot = cmd.uniforms[u];
                if (slot.type == RenderDrawCommand::UniformSlot::UINT)
                    cmd.shader->setUInt(slot.location, slot.uintVal);
                else if (slot.type == RenderDrawCommand::UniformSlot::FLOAT)
                    cmd.shader->setFloat(slot.location, slot.floatVal);
            }
        }

        if (cmd.ebo != 0)
        {
            bm.BindBuffer(BufferType::ElementArrayBuffer, cmd.ebo);
            dc.DrawElements(Primitive::Triangles, cmd.count, DataType::UnsignedInt, 0);
        }
        else
        {
            dc.DrawArrays(Primitive::Triangles, 0, cmd.count);
        }
    }

    m_RenderCommandBuffer.Clear();
    bm.BindVertexArray(0);
}

void RenderServiceImpl::UploadCameraUBO(const GPUCameraData& camData)
{
    if (!m_Context || !m_CameraUBO)
        return;
    auto& bm = m_Context->GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUCameraData), &camData);
}

void RenderServiceImpl::RestoreCameraState(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos,
                                           float nearPlane, float farPlane)
{
    m_CameraState.viewMatrix = view;
    m_CameraState.projectionMatrix = proj;
    m_CameraState.position = pos;
    m_CameraState.nearPlane = nearPlane;
    m_CameraState.farPlane = farPlane;

    // Also recalculate the current view-projection for things that rely on it
    m_GlobalState.currViewProj = m_GlobalState.jitteredProjection * view;
}
