#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/ui_components.h>
#include <ecs/unit/light_components.h>
#include <ecs/unit/reflection_components.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <render/unit/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <thread>
#include <core/logic/job_system.h>
#include <render/logic/render_service_impl.h>
#include <ecs/logic/system_factory.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <cstring>
#include <resource/logic/resource_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_query_manager.h>
#include <ecs/logic/entity_manager.h>
#include <render/interface/i_render_target_manager.h>
#include <core/logic/service_locator.h>
#include <render/logic/render_core.h>
#include <core/logic/event_system.h>

#include <core/type/app_config.h>
#include <core/logic/config_manager.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <ecs/logic/debug/debug_system.h>
#endif

#include <platform/logic/io_handler.h>
#include <render/logic/shadow_renderer.h>
#include <render/logic/material_renderer.h>
#include <render/logic/render_core.h>

RenderServiceImpl::RenderServiceImpl() {}
RenderServiceImpl::~RenderServiceImpl() {}

void RenderServiceImpl::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    sl.Register<IRenderService>(this);
    sl.Register<ICameraService>(this);
    sl.Register<IRenderStateService>(this);
    sl.Register<IRenderQueueService>(this);
    sl.Register<IIBLService>(this);
    auto& context = sl.Require<IGraphicsContext>();
    auto& configManager = sl.Require<ConfigManager>();
    const AppConfig& config = configManager.GetConfig();
    auto& shaderLib = sl.Require<ResourceManager>();
    
    m_Context = &context;
    m_ConfigManager = &configManager;

    this->SetInstanceBatching(config.instanceBatchingEnabled);
    this->SetFrustumCulling(config.frustumCullingEnabled);
    this->SetOcclusionCulling(config.occlusionCullingEnabled);
    this->SetDistanceCulling(config.distanceCulling);
    this->SetAntiAliasingMode((AntiAliasingMode)config.antialiasing);
    this->SetRenderOrderEnabled(config.renderOrderEnabled);
    this->SetFilterLayerMask(config.filterLayerMask);
    this->SetFaceCulling(config.cullFaceEnabled);
    this->SetDepthTest(config.depthTestEnabled);


    EventSystem::Instance().Subscribe<ConfigChangedEvent>([this](const ConfigChangedEvent& e) {
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
    m_RenderCore->Initialize(context);
    sl.Register<RenderCore>(m_RenderCore.get());

    auto& core = *m_RenderCore;
    
    auto& bm = context.GetBufferManager();
    m_CameraUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUCameraData), nullptr, BufferUsage::DynamicDraw);

    m_GlobalLightUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalLightData), nullptr, BufferUsage::DynamicDraw);

    m_GlobalDataUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalData), nullptr, BufferUsage::DynamicDraw);

    bm.BindBufferBase(BufferType::UniformBuffer, 20, m_CameraUBO->Get());
    bm.BindBufferBase(BufferType::UniformBuffer, 21, m_GlobalLightUBO->Get());
    bm.BindBufferBase(BufferType::UniformBuffer, 22, m_GlobalDataUBO->Get());

    m_OcclusionCuller.Initialize(context, shaderLib.GetShader("occlusion"));
    m_UnlitShader = shaderLib.GetShader("forward_unlit");
    m_DeferredLitShader = shaderLib.GetShader("deferred_lit");
    m_DeferredUnlitShader = shaderLib.GetShader("deferred_unlit");
    m_ForwardPBRLitShader = shaderLib.GetShader("forward_pbr_lit");
    m_ErrorForwardShader = shaderLib.GetShader("error_forward");
    m_ErrorDeferredShader = shaderLib.GetShader("error_deferred");
}

void RenderServiceImpl::FetchRenderPath()
{
    if (m_ConfigManager) {
        m_CachedRenderPath = m_ConfigManager->GetConfig().renderPath;
    } else {
        m_CachedRenderPath = ServiceLocator::Instance().Require<ConfigManager>().GetConfig().renderPath;
    }
}

void RenderServiceImpl::Shutdown()
{
    LOGGER_INFO("RenderSystem") << "Shutting down RenderSystem";
    m_OcclusionCuller.Shutdown();
    if (m_RenderCore) m_RenderCore->Shutdown();
}

void RenderServiceImpl::SetFaceCulling(bool enabled, CullMode mode)
{
    if (!m_Context) return;
    auto &rsm = m_Context->GetRenderStateManager();

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
    if (!m_Context) return;
    auto &rsm = m_Context->GetRenderStateManager();

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

void RenderServiceImpl::BuildRenderQueues(Scene &scene, float alpha, int width, int height)
{
    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) {
        if (!m_QueuesBuilt) {
            LOGGER_WARN("RenderSystem") << "BuildRenderQueues: No active camera found!";
            m_QueuesBuilt = true; m_LastAlpha = alpha;
        }
        return;
    }

    CameraComponent& cam = scene.registry.get<CameraComponent>(camEntity);
    PositionComponent* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 pos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    BuildRenderQueuesWithCamera(scene, cam.viewMatrix, cam.projectionMatrix, pos, cam.nearPlane, cam.farPlane, alpha, width, height, cam.cullingMask);
}

void RenderServiceImpl::BuildRenderQueuesWithCamera(Scene& scene, const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float nearPlane, float farPlane, float lodFactor, int width, int height, uint32_t cullingMask, bool isCapturingProbe, entt::entity excludeEntity)
{
    m_IsCapturingProbe = isCapturingProbe;
    if (lodFactor <= 0.0f || lodFactor > 1.0f) {
        // LOGGER_WARN("RenderSystem") << "Invalid alpha value: " << lodFactor;
    }

    if (m_ConfigManager) {
        m_CachedRenderPath = m_ConfigManager->GetConfig().renderPath;
    }

    IGraphicsContext& context = *m_Context;
    m_LastWidth = width;
    m_LastHeight = height;

    m_CameraPos = pos;
    m_ViewMatrix = view;
    m_ProjMatrix = proj;
    
    m_NearPlane = nearPlane;
    m_FarPlane = farPlane;

    if (width <= 0 || height <= 0) return;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!std::isfinite(proj[i][j]) || !std::isfinite(view[i][j])) {
                LOGGER_ERROR("RenderSystem") << "NaN detected in camera matrices. Skipping frame.";
                return;
            }
        }
    }

    if (!m_QueuesBuilt) { m_PrevViewProj = m_CurrViewProj; }

    m_JitteredProjection = proj;
    m_JitterOffset = glm::vec2(0.0f);

    // Only apply TAA if rendering to main screen (width/height usually match window)
    if (m_AAMode == AntiAliasingMode::TAA && width > 512) { 
        auto HaltonSequence = [](int index, int base) -> float {
            float result = 0.0f; float f = 1.0f; int i = index;
            while (i > 0) { f = f / base; result = result + f * (i % base); i = i / base; }
            return result;
        };
        const int sampleCount = 8;
        int frameIdx = m_FrameIndex % sampleCount;
        float jitterX = HaltonSequence(frameIdx + 1, 2) - 0.5f;
        float jitterY = HaltonSequence(frameIdx + 1, 3) - 0.5f;
        m_JitterOffset = glm::vec2(jitterX, jitterY);
        
        glm::mat4 jitterMatrix = glm::mat4(1.0f);
        jitterMatrix[3][0] = jitterX * 2.0f / (float)width;
        jitterMatrix[3][1] = jitterY * 2.0f / (float)height;
        m_JitteredProjection = jitterMatrix * m_JitteredProjection;
        
        if (!m_QueuesBuilt) m_FrameIndex++;
    }

    m_CurrViewProj = m_JitteredProjection * view;
    

    bool stable = true;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            if (!std::isfinite(m_CurrViewProj[i][j])) stable = false;
        }
    }
    if (!stable) {
        m_CurrViewProj = proj * view;
    }


    GPUCameraData camData;
    std::memcpy(camData.projection, &m_JitteredProjection[0][0], 16 * sizeof(float));
    std::memcpy(camData.view, &view[0][0], 16 * sizeof(float));
    std::memcpy(camData.viewPos, &pos[0], 3 * sizeof(float));
    camData.viewPos[3] = 1.0f;

    glm::mat4 invProj = glm::inverse(m_JitteredProjection);
    glm::mat4 invView = glm::inverse(view);
    std::memcpy(camData.invProjection, &invProj[0][0], 16 * sizeof(float));
    std::memcpy(camData.invView, &invView[0][0], 16 * sizeof(float));

    glm::mat4 invStableProj = glm::inverse(proj);
    std::memcpy(camData.stableProjection, &proj[0][0], 16 * sizeof(float));
    std::memcpy(camData.invStableProjection, &invStableProj[0][0], 16 * sizeof(float));

    auto& bm = context.GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUCameraData), &camData);


    m_GlobalData.resolution[0] = (float)width;
    m_GlobalData.resolution[1] = (float)height;
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalData), &m_GlobalData);

    glm::mat4 stableVP = proj * view;
    m_FrustumCuller.BuildFrustum(stableVP);

    if (m_OcclusionCullingEnabled) {
        m_OcclusionCuller.UpdateResults(scene);
    }

    m_RenderQueueObj.Clear();
    m_RenderedCount = 0;

    // Pre-collect reflection probes for fast lookup
    struct ProbeData {
        ReflectionProbeComponent* component;
        glm::vec3 position;
    };
    std::vector<ProbeData> probes;
    auto probeView = scene.registry.view<PositionComponent, ReflectionProbeComponent>();
    for (auto entity : probeView) {
        auto [pos, probe] = probeView.get<PositionComponent, ReflectionProbeComponent>(entity);
        probes.push_back({&probe, pos.value});
    }

    auto renderView = scene.registry.view<WorldTransformComponent, MeshRendererComponent>();
    for (auto entity : renderView) {
        if (entity == excludeEntity) continue;

        auto& world = renderView.get<WorldTransformComponent>(entity);
        glm::mat4 modelMatrix = world.GetInterpolated(lodFactor);
        
        auto& renderer = renderView.get<MeshRendererComponent>(entity);
        if (!renderer.model) continue;

        float distSqResult = glm::length2(pos - glm::vec3(modelMatrix[3]));
        if (m_DistanceCullingSq > 0.0f && distSqResult > m_DistanceCullingSq) continue;

        AABB worldAABB = renderer.model->aabb.Transform(modelMatrix);

        // Skip objects that contain the probe (Self-occlusion fix using AABB)
        if (m_IsCapturingProbe && worldAABB.Contains(pos)) continue;

        if (m_FrustumCullingEnabled) {
            if (!m_FrustumCuller.IsVisible(worldAABB.minBound, worldAABB.maxBound)) continue;
        }

        uint32_t layer = 1;
        if (auto* info = scene.registry.try_get<InfoComponent>(entity)) layer = info->layer;
        
        if ((m_FilterLayerMask & layer) == 0 || (cullingMask & layer) == 0) continue;

        Model* activeModel = renderer.model.get();
        Shader* itemShader = renderer.shader.lock().get();

        if (auto* lod = scene.registry.try_get<LODComponent>(entity)) {
            for (int j = 0; j < (int)lod->lodDistancesSq.size(); ++j) {
                if (distSqResult > lod->lodDistancesSq[j] && j < (int)lod->lodModels.size() && lod->lodModels[j]) {
                    activeModel = lod->lodModels[j].get();
                } else break;
            }
        }

        if (m_OcclusionCullingEnabled) {
            if (auto* occ = scene.registry.try_get<OcclusionComponent>(entity)) {
                if (!occ->isVisible) continue;
            }
        }

        bool isTransparent = false;
        auto* material = scene.registry.try_get<AxisMaterialComponent>(entity);
        if (material && material->desc.opacity < 1.0f) isTransparent = true;
        
        auto* reflection = scene.registry.try_get<ReflectiveComponent>(entity);

        uint64_t key = 0;
        uint64_t sId = itemShader ? itemShader->getID() : 0;
        if (!isTransparent) {
            uint64_t l = (uint64_t)(layer & 0xFF) << 56;
            uint64_t o = (uint64_t)(renderer.order & 0xFF) << 48;
            uint64_t s = (uint64_t)(sId & 0xFFFF) << 32;
            uint64_t m = (uint64_t)((uintptr_t)material & 0xFFFF) << 16;
            uint64_t mod = (uint64_t)((uintptr_t)activeModel & 0xFF) << 8;
            uint64_t d = (uint64_t)(glm::clamp(distSqResult * 0.1f, 0.0f, 255.0f)) & 0xFF;
            key = l | o | s | m | mod | d;
        } else {
            uint64_t l = (uint64_t)(layer & 0xFF) << 56;
            float invDepth = 1000000.0f - distSqResult; 
            if (invDepth < 0) invDepth = 0;
            uint64_t d = (uint64_t)(invDepth) & 0x00FFFFFFFFFFFFFFULL;
            key = l | d;
        }

        RenderItem item;
        item.entityId = (uint32_t)entity;
        item.model = activeModel;
        item.shader = itemShader;
        item.material = material;
        item.reflection = (reflection && reflection->enabled) ? reflection : nullptr;
        
        // Reflection Probe Selection
        if (item.reflection) {
            bool probeFound = false;
            
            // 1. Try Target Probe (Name or Name:Tag)
            if (!item.reflection->targetProbe.empty()) {
                std::string target = item.reflection->targetProbe;
                size_t colon = target.find(':');
                std::string targetName = (colon == std::string::npos) ? target : target.substr(0, colon);
                std::string targetTag = (colon == std::string::npos) ? "" : target.substr(colon + 1);

                for (auto probeEntity : probeView) {
                    auto* info = scene.registry.try_get<InfoComponent>(probeEntity);
                    if (info && (info->name == targetName)) {
                        if (targetTag.empty() || info->tag == targetTag) {
                            auto [pPos, pComp] = probeView.get<PositionComponent, ReflectionProbeComponent>(probeEntity);
                            item.probe = &pComp;
                            item.probePos = pPos.value;
                            probeFound = true;
                            break;
                        }
                    }
                }
            }

            // 2. Fallback to Nearest Probe
            if (!probeFound) {
                float minProbeDistSq = 1e30f;
                for (auto& p : probes) {
                    float d = glm::distance2(glm::vec3(modelMatrix[3]), p.position);
                    if (d < minProbeDistSq) {
                        minProbeDistSq = d;
                        item.probe = p.component;
                        item.probePos = p.position;
                        probeFound = true;
                    }
                }
            }

            // Standardize Intensity (since radius is removed)
            item.reflectionIntensity = 1.0f;
            if (item.probe) item.probeIndex = item.probe->lastGpuIndex;
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

        item.tintColor = renderer.color;
        bool hasAnimation = false;
        if (auto* anim = scene.registry.try_get<AnimationComponent>(entity)) {
            if (anim->animator) {
                hasAnimation = true;
                item.hasAnimation = true;
                item.boneMatrices = anim->animator->GetFinalBoneMatrices();
            }
        }

        bool hasPhysic = false;
        bool isStaticPhysic = true;
        if (auto* rb = scene.registry.try_get<RigidBodyComponent>(entity)) {
            hasPhysic = true;
            if (rb->body) {
                isStaticPhysic = rb->body->IsStatic() && !rb->body->IsKinematic();
            }
        }

        // Bake mode criteria: No animation, and no physics types other than static.
        item.isStatic = (!hasAnimation) && (!hasPhysic || (hasPhysic && isStaticPhysic));

        if (isTransparent) m_RenderQueueObj.AddTransparent(item);
        else m_RenderQueueObj.AddOpaque(item);
        
        if (renderer.castShadow) m_RenderQueueObj.AddShadow(item);
    }

    // Collect Lights
    auto dirView = scene.registry.view<DirectionalLightComponent>();
    for (auto entity : dirView) {
        auto& light = dirView.get<DirectionalLightComponent>(entity);
        if (!light.active) continue;
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

    auto pointView = scene.registry.view<PointLightComponent, PositionComponent>();
    for (auto entity : pointView) {
        auto [light, pos] = pointView.get<PointLightComponent, PositionComponent>(entity);
        if (!light.active) continue;
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

    auto spotView = scene.registry.view<SpotLightComponent, PositionComponent, RotationComponent>();
    for (auto entity : spotView) {
        auto [light, pos, rot] = spotView.get<SpotLightComponent, PositionComponent, RotationComponent>(entity);
        if (!light.active) continue;
        RenderLight rl;
        rl.type = RenderLightType::Spot;
        rl.position = pos.value;
        rl.direction = rot.value * glm::vec3(0, -1, 0);
        rl.color = light.color;
        rl.intensity = light.intensity;
        rl.constant = light.constant;
        rl.linear = light.linear;
        rl.quadratic = light.quadratic;
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
    m_IrradianceMap = 0;
    m_PrefilterMap = 0;
    m_BrdfLUT = 0;
    auto skyView = scene.registry.view<SkyboxRenderComponent>();
    for (auto skyEnt : skyView) {
        auto& skyComp = skyView.get<SkyboxRenderComponent>(skyEnt);
        if (skyComp.isPrimary && skyComp.skybox) {
            m_IrradianceMap = skyComp.irradianceMap;
            m_PrefilterMap = skyComp.prefilterMap;
            m_BrdfLUT = skyComp.brdfLUT;
            break;
        }
    }

    static bool firstFrame = true;
    if (firstFrame && !m_IsCapturingProbe) {
        LOGGER_INFO("RenderSystem") << "BuildRenderQueues: Opaque=" << (int)m_RenderQueueObj.GetOpaqueQueue().size() 
                                    << ", Transparent=" << (int)m_RenderQueueObj.GetTransparentQueue().size();
        firstFrame = false;
    }

    m_QueuesBuilt = true;
    m_LastAlpha = lodFactor;
    m_LastWidth = width;
    m_LastHeight = height;
}

void RenderServiceImpl::ExecuteQueue(const std::vector<RenderItem>& queue, bool isTransparentPass, ShadowRenderer* shadowRenderer, MaterialRenderer* materialRenderer, Shader* overrideShader)
{
    if (!materialRenderer) {
        auto& core = ServiceLocator::Instance().Require<RenderCore>();
        materialRenderer = &core.GetMaterialRenderer();
    }

    Shader* lastShader = nullptr;

    RenderSceneData sceneData;
    sceneData.cameraPosition = m_CameraPos;
    sceneData.viewMatrix = m_ViewMatrix;
    sceneData.projMatrix = m_ProjMatrix;
    sceneData.nearPlane = m_NearPlane;
    sceneData.farPlane = m_FarPlane;
    sceneData.irradianceMap = m_IrradianceMap;
    sceneData.prefilterMap = m_PrefilterMap;
    sceneData.brdfLUT = m_BrdfLUT;

    m_RenderedCount += (int)queue.size();

    for (const auto& item : queue) {
        Model* model = item.model;
        AxisMaterialComponent* material = item.material;
        Shader* shader = item.shader;

        if (overrideShader) {
            shader = overrideShader;
        } else if (!shader) {
            if (m_CachedRenderPath == RenderPath::Deferred) {
                if (m_IsCapturingProbe) {
                    shader = m_ForwardPBRLitShader.get();
                } else {
                    shader = m_DeferredLitShader.get();
                }
            } else {
                shader = m_UnlitShader.get();
            }
        } else if (m_CachedRenderPath == RenderPath::Deferred && !m_IsCapturingProbe) {
            // Only translate core shaders if NOT capturing a probe
            if (shader == m_UnlitShader.get()) {
                shader = m_DeferredUnlitShader.get();
            }
        } else if (m_IsCapturingProbe) {
            // If capturing a probe, swap deferred shaders to their forward equivalents
            if (shader == m_DeferredLitShader.get()) {
                shader = m_ForwardPBRLitShader.get();
            } else if (shader == m_DeferredUnlitShader.get()) {
                shader = m_UnlitShader.get();
            }
        }

        if (!shader || !model) {
            if (m_CachedRenderPath == RenderPath::Deferred) shader = m_ErrorDeferredShader.get();
            else shader = m_ErrorForwardShader.get();
            if (!shader || !model) continue;
        }

        if (shader != lastShader) {
            shader->use();
            lastShader = shader;

            if (shadowRenderer && shadowRenderer->IsShadowsEnabled()) {
                auto& shadow = shadowRenderer->GetShadow();
                shadow.BindTexture_Dir(0, 10);
                shadow.BindTexture_Point(0, 11);
                shadow.BindTexture_Spot(0, 12);
                shader->setInt("shadowMapDir", 10);
                shader->setInt("shadowMapPoint", 11);
                shader->setInt("shadowMapSpot", 12);
                shader->setFloat("u_ShadowBias", shadowRenderer->GetShadowBias());
                shader->setInt("u_ShadowSoftness", shadowRenderer->GetShadowSoftness());
            }
        }

        glm::mat4 mtx = item.worldMatrix;
        if (item.hasAnimation) {
            shader->setMat4Array("finalBonesMatrices", item.boneMatrices);
        } else {
            mtx *= model->GetRootTransform();
        }

        shader->setMat4("model", mtx);
        shader->setVec4("tintColor", item.tintColor);
        shader->setUInt("entityID", item.entityId);
        shader->setBool("isInstanced", false);
        
        if (shader->GetName() == "deferred_reflect") {
            shader->setInt("u_ProbeIndex", item.probeIndex);
        }
        // Reflection and Textures
        auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
        auto& tm = context.GetTextureManager();

        shader->setBool("u_ProbeUnlit", m_IsCapturingProbe);
        shader->setVec2("u_ScreenSize", glm::vec2(m_LastWidth, m_LastHeight));
        
        // Planar Reflection Binding for Forward
        auto& scene = ServiceLocator::Instance().Require<Scene>();
        auto planarView = scene.registry.view<PlanarReflectionComponent>();
        int planarCount = 0;
        if (!m_IsCapturingProbe) {
            for (auto planarEntity : planarView) {
                auto& prc = planarView.get<PlanarReflectionComponent>(planarEntity);
                if (prc.reflectionTextureID && prc.isRendered) {
                    tm.ActiveTexture(static_cast<TextureUnit>(static_cast<int>(TextureUnit::Texture19) + planarCount));
                    tm.BindTexture(TextureType::Texture2D, prc.reflectionTextureID);
                    
                    std::string texBase = "u_PlanarReflections[" + std::to_string(planarCount) + "]";
                    std::string normBase = "u_PlanarNormals[" + std::to_string(planarCount) + "]";
                    shader->setInt(texBase, 19 + planarCount);
                    shader->setVec3(normBase, prc.normal);
                    
                    planarCount++;
                    if (planarCount >= 4) break;
                }
            }
        }
        shader->setInt("u_PlanarCount", planarCount);
        
        // Reflection Probe Binding & Cleanup
        if (item.reflection && item.probe && !m_IsCapturingProbe) {
            tm.ActiveTexture(TextureUnit::Texture15);
            tm.BindTexture(TextureType::TextureCubeMap, item.probe->cubemapID);
            shader->setInt("reflectionProbe", 15);
            shader->setBool("u_HasReflection", true);
            
            // For Forward Reflection Shaders
            shader->setBool("u_HasProbe", true);
            shader->setVec3("u_ProbePos", item.probePos);
            shader->setVec3("u_ProbeBoxMin", item.probe->boxMin);
            shader->setVec3("u_ProbeBoxMax", item.probe->boxMax);
        } else {
            // Force Cleanup: Ensure no stale cubemap is bound to Unit 15
            if (!m_IsCapturingProbe) {
                tm.ActiveTexture(TextureUnit::Texture15);
                tm.BindTexture(TextureType::TextureCubeMap, 0);
                shader->setBool("u_HasReflection", false);
                shader->setBool("u_HasProbe", false);
                shader->setInt("u_ProbeCount", 0); // Explicitly zero out probe count
            }
        }
        
        // Dynamic Mapping for Reflection Probes (Zero-Code Re-routing)
        if (m_IsCapturingProbe) {
            auto& sl = ServiceLocator::Instance();
            auto& rtm = sl.Require<IGraphicsContext>().GetRenderTargetManager();
            if (shader->IsDeferred()) {
                // Reroute Shader Location 2 (Albedo) to Attachment 0 (Cube face)
                // location 0 (Pos) -> None, 1 (Norm) -> None, 2 (Albedo) -> Color0
                FramebufferAttachment atts[] = { FramebufferAttachment::None, FramebufferAttachment::None, FramebufferAttachment::Color0 };
                rtm.DrawBuffers(3, atts);
            } else {
                // Forward Shader: location 0 -> Color0
                FramebufferAttachment atts[] = { FramebufferAttachment::Color0 };
                rtm.DrawBuffers(1, atts);
            }
        }
        
        // Pass reflection data if available
        if (item.reflection && !m_IsCapturingProbe) {
            shader->setFloat("u_Reflectivity", item.reflection->reflectivity);
            shader->setFloat("u_FresnelPower", item.reflection->fresnelPower);
            shader->setFloat("u_FresnelBias", item.reflection->fresnelBias);
            
            if (item.probe && item.probe->cubemapID != 0) {
                auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
                auto& texMgr = context.GetTextureManager();
                texMgr.ActiveTexture(TextureUnit::Texture15);
                texMgr.BindTexture(TextureType::TextureCubeMap, item.probe->cubemapID);
                shader->setBool("u_HasProbe", true);
                shader->setVec3("u_ProbePos", item.probePos);
                shader->setVec3("u_ProbeBoxMin", item.probe->boxMin);
                shader->setVec3("u_ProbeBoxMax", item.probe->boxMax);
                shader->setFloat("u_ReflectionIntensity", item.reflectionIntensity);
            } else {
                shader->setBool("u_HasProbe", false);
            }
        } else {
            shader->setFloat("u_Reflectivity", 0.0f);
            shader->setFloat("u_FresnelPower", 5.0f);
            shader->setFloat("u_FresnelBias", 0.04f);
            shader->setBool("u_HasProbe", false);
        }

        bool matBound = materialRenderer->SetupMaterialUniforms(shader, material, sceneData, item.tintColor, m_DebugNoTexture, m_Wireframe);
        model->Draw(*shader, !matBound);

        // Restore DrawBuffers mapping after draw
        if (m_IsCapturingProbe) {
             auto& sl = ServiceLocator::Instance();
             auto& rtm = sl.Require<IGraphicsContext>().GetRenderTargetManager();
             FramebufferAttachment atts[] = { FramebufferAttachment::Color0 };
             rtm.DrawBuffers(1, atts);
        }
    }
}

unsigned int RenderServiceImpl::GetWhiteTexture() const { return ServiceLocator::Instance().Require<RenderCore>().GetWhiteTexture(); }
unsigned int RenderServiceImpl::GetBlackTexture() const { return ServiceLocator::Instance().Require<RenderCore>().GetBlackTexture(); }
unsigned int RenderServiceImpl::GetFlatNormalTexture() const { return ServiceLocator::Instance().Require<RenderCore>().GetFlatNormalTexture(); }



void RenderServiceImpl::UpdateGlobalLightData(const GPUGlobalLightData& data)
{
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& bm = context.GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalLightData), &data);
}

void RenderServiceImpl::SubmitCommand(const RenderDrawCommand& cmd)
{
    m_RenderCommandBuffer.Submit(cmd);
}

void RenderServiceImpl::FlushCommands()
{
    m_RenderCommandBuffer.Sort();
    const auto& commands = m_RenderCommandBuffer.GetCommands();
    
    if (commands.empty()) return;

    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& rsm = context.GetRenderStateManager();
    auto& bm = context.GetBufferManager();
    auto& dc = context.GetDrawContext();
    auto& tm = context.GetTextureManager();

    uint32_t lastShader = 0;
    uint32_t lastVAO = 0;

    for (const auto& cmd : commands) {
        if (cmd.shaderId != lastShader) {
            if (cmd.shader) cmd.shader->use();
            lastShader = cmd.shaderId;
        }
        
        if (cmd.vao != lastVAO) {
            bm.BindVertexArray(cmd.vao);
            lastVAO = cmd.vao;
        }
        
        if (cmd.shader) {
            cmd.shader->setMat4("model", cmd.modelMatrix);
            if (cmd.texture0 != 0) {
                tm.ActiveTexture(TextureUnit::Texture0);
                tm.BindTexture(TextureType::Texture2D, cmd.texture0);
                cmd.shader->setInt("u_Texture0", 0);
            }
            cmd.shader->setVec4("u_TintColor", cmd.tintColor);
            
            for (const auto& kv : cmd.uintUniforms) {
                cmd.shader->setUInt(kv.first, kv.second);
            }
            for (const auto& kv : cmd.floatUniforms) {
                cmd.shader->setFloat(kv.first, kv.second);
            }
        }
        
        if (cmd.ebo != 0) {
            bm.BindBuffer(BufferType::ElementArrayBuffer, cmd.ebo);
            dc.DrawElements(Primitive::Triangles, cmd.count, DataType::UnsignedInt, 0);
        } else {
            dc.DrawArrays(Primitive::Triangles, 0, cmd.count);
        }
    }
    
    m_RenderCommandBuffer.Clear();
    bm.BindVertexArray(0);
}

void RenderServiceImpl::UploadCameraUBO(const GPUCameraData& camData)
{
    if (!m_Context || !m_CameraUBO) return;
    auto& bm = m_Context->GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUCameraData), &camData);
}

void RenderServiceImpl::RestoreCameraState(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& pos, float nearPlane, float farPlane)
{
    m_ViewMatrix = view;
    m_ProjMatrix = proj;
    m_CameraPos = pos;
    m_NearPlane = nearPlane;
    m_FarPlane = farPlane;
    
    // Also recalculate the current view-projection for things that rely on it
    m_CurrViewProj = m_JitteredProjection * view;
}


