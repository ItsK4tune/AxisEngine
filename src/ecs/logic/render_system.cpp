#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/ui_components.h>
#include <core/logic/logger.h>
#include <core/type/event_types.h>
#include <render/unit/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <thread>
#include <core/logic/job_system.h>
#include <ecs/logic/render_system.h>
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
#include <core/logic/event_system.h>
#include <core/type/app_config.h>
#include <core/logic/config_manager.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <ecs/logic/debug/debug_system.h>
#endif

#include <platform/logic/io_handler.h>
#include <render/logic/shadow_renderer.h>
#include <render/logic/material_renderer.h>

void RenderSystem::Initialize()
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& configManager = sl.Require<ConfigManager>();
    const AppConfig& config = configManager.GetConfig();
    auto& shaderLib = sl.Require<ResourceManager>();
    
    // Set initial state
    this->SetInstanceBatching(config.instanceBatchingEnabled);
    this->SetFrustumCulling(config.frustumCullingEnabled);
    this->SetOcclusionCulling(config.occlusionCullingEnabled);
    this->SetDistanceCulling(config.distanceCulling);
    this->SetAntiAliasingMode((AntiAliasingMode)config.antialiasing);
    this->SetRenderOrderEnabled(config.renderOrderEnabled);
    this->SetFilterLayerMask(config.filterLayerMask);
    this->SetFaceCulling(config.cullFaceEnabled);
    this->SetDepthTest(config.depthTestEnabled);

    // Subscribe to config changes
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

    // Shadow and Light renderers are now handled by ShadowSystem and LightingSystem
    // m_LightRenderer is still here for now if needed, but should be handled by LightingSystem

    if (m_WhiteTextureID == 0)
    {
        auto &tm = context.GetTextureManager();
        
        // White
        m_WhiteTextureID = tm.GenTexture();
        tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
        unsigned char white[] = {255, 255, 255, 255};
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA, DataType::UnsignedByte, white);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);

        // Black
        m_BlackTextureID = tm.GenTexture();
        tm.BindTexture(TextureType::Texture2D, m_BlackTextureID);
        unsigned char black[] = {0, 0, 0, 255};
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA, DataType::UnsignedByte, black);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Nearest);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Nearest);

        // Flat Normal (0.5, 0.5, 1.0) => (128, 128, 255)
        m_FlatNormalTextureID = tm.GenTexture();
        tm.BindTexture(TextureType::Texture2D, m_FlatNormalTextureID);
        unsigned char flatNormal[] = {128, 128, 255, 255};
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA, DataType::UnsignedByte, flatNormal);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, (int)TextureFilter::Linear);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, (int)TextureFilter::Linear);
    }

    auto& resources = ServiceLocator::Instance().Require<ResourceManager>();
    resources.LoadShader("unlit", "include/engine/asset/shaders/unlit.vs", "include/engine/asset/shaders/unlit.fs");
    m_UnlitShader = resources.GetShader("unlit");
    m_BonesUniforms.reserve(200);
    for (int i = 0; i < 200; ++i)
        m_BonesUniforms.push_back("finalBonesMatrices[" + std::to_string(i) + "]");

    auto& bm = context.GetBufferManager();
    m_CameraUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUCameraData), nullptr, BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::UniformBuffer, 20, m_CameraUBO->Get());
    
    m_GlobalLightUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalLightData), nullptr, BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::UniformBuffer, 21, m_GlobalLightUBO->Get());
    
    m_GlobalDataUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalData), nullptr, BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::UniformBuffer, 22, m_GlobalDataUBO->Get());
    
    // PostProcess and GBuffer initials are now handled by specialized systems
    InitQuad();
}

void RenderSystem::Update(Scene& scene, float dt)
{
    // RenderSystem update logic (e.g. frame index increment)
    m_FrameIndex++;
    m_RenderedCount = 0;
}

void RenderSystem::Render(Scene& scene)
{
    // Primary render pass logic if any, otherwise handled in specialized passes
}

void RenderSystem::RenderUI(Scene &scene, float width, float height, IRenderStateManager &renderState)
{
    // RenderSystem doesn't handle UI directly; UIRenderSystem does.
}

void RenderSystem::Shutdown()
{
    LOGGER_INFO("RenderSystem") << "Shutting down RenderSystem";
    m_OcclusionCuller.Shutdown();

    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto& bm = context.GetBufferManager();

    if (m_QuadVAO.id != 0) bm.DeleteVertexArray(m_QuadVAO.id);
    if (m_QuadVBO.id != 0) bm.DeleteBuffer(m_QuadVBO.id);
}

void RenderSystem::SetFaceCulling(bool enabled, CullMode mode)
{
    auto& sl = ServiceLocator::Instance();
    auto& context = sl.Require<IGraphicsContext>();
    auto &rsm = context.GetRenderStateManager();

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

void RenderSystem::InitQuad()
{
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& bm = context.GetBufferManager();
    m_QuadVAO.id = bm.CreateVertexArray();
    m_QuadVBO.id = bm.CreateBuffer();

    bm.BindVertexArray(m_QuadVAO.id);
    bm.BindBuffer(BufferType::ArrayBuffer, m_QuadVBO.id);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(quadVertices), &quadVertices, BufferUsage::StaticDraw);
    
    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, 5 * sizeof(float), (void*)0);
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 2, DataType::Float, false, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void RenderSystem::SetDepthTest(bool enabled, CompareFunc func)
{
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto &rsm = context.GetRenderStateManager();

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

void RenderSystem::BuildRenderQueues(Scene &scene, float alpha, int width, int height)
{
    if (m_QueuesBuilt && m_LastAlpha == alpha && m_LastWidth == width && m_LastHeight == height) {
        return;
    }

    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    if ((m_LastWidth != width || m_LastHeight != height) && width > 0 && height > 0) {
        // GBuffer resize is now handled by GeometrySystem
    }
    m_LastWidth = width;
    m_LastHeight = height;

    if (alpha <= 0.0f || alpha > 1.0f) {
        LOGGER_WARN("RenderSystem") << "Invalid alpha value: " << alpha;
    }

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) {
        m_QueuesBuilt = true; m_LastAlpha = alpha;
        return;
    }

    CameraComponent* cam = &scene.registry.get<CameraComponent>(camEntity);
    PositionComponent* camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);

    if (!m_QueuesBuilt) { m_PrevViewProj = m_CurrViewProj; }

    m_JitteredProjection = cam->projectionMatrix;
    m_JitterOffset = glm::vec2(0.0f);

    if (m_AAMode == AntiAliasingMode::TAA && width > 0 && height > 0) {
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

    m_CurrViewProj = m_JitteredProjection * cam->viewMatrix;

    // Update Camera UBO
    GPUCameraData camData;
    std::memcpy(camData.projection, &m_JitteredProjection[0][0], 16 * sizeof(float));
    std::memcpy(camData.view, &cam->viewMatrix[0][0], 16 * sizeof(float));
    std::memcpy(camData.viewPos, &camPos[0], 3 * sizeof(float));
    camData.pad0 = 0.0f;

    auto& bm = context.GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUCameraData), &camData);

    // Update Global Data UBO
    m_GlobalData.resolution[0] = (float)width;
    m_GlobalData.resolution[1] = (float)height;
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalData), &m_GlobalData);

    glm::mat4 stableVP = cam->projectionMatrix * cam->viewMatrix;
    m_FrustumCuller.BuildFrustum(stableVP);

    if (m_OcclusionCullingEnabled) {
        m_OcclusionCuller.UpdateResults(scene);
    }

    m_RenderQueueObj.Build(scene, alpha, m_FrustumCuller, m_FrustumCullingEnabled, m_OcclusionCullingEnabled, 
                           m_DistanceCullingSq, m_FilterLayerMask, cam->cullingMask, camPos);

    m_LastAlpha = -1.0f;
    m_LastWidth = -1;
    m_LastHeight = -1;

    m_QueuesBuilt = true;
    m_LastAlpha = alpha;
}

void RenderSystem::ExecuteQueue(Scene& scene, const std::vector<RenderItem>& queue, bool isTransparentPass, ShadowRenderer* shadowRenderer, MaterialRenderer* materialRenderer, Shader* overrideShader)
{
    size_t totalItems = queue.size();
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 1;
    size_t chunkSize = (totalItems + numThreads - 1) / numThreads;

    std::vector<CommandQueue> threadQueues(numThreads);
    JobSystem::JobCounter counter(0);
    
    // Increment total rendered count for UI (only once per pass type)
    m_RenderedCount += (int)totalItems;

    for (size_t i = 0; i < numThreads; ++i)
    {
        size_t startIdx = i * chunkSize;
        if (startIdx >= totalItems) break;
        size_t endIdx = std::min(startIdx + chunkSize, totalItems);

        JobSystem::Instance().Execute([this, &scene, &queue, startIdx, endIdx, &threadQueue = threadQueues[i], isTransparentPass, shadowRenderer, materialRenderer, overrideShader]() {
            Shader *currentShader = nullptr;
            Model *currentModel = nullptr;
            MaterialComponent *currentMaterial = nullptr;
            std::vector<glm::mat4> instanceBatch;

            auto flushBatch = [&](Shader *shader, Model *model, const std::vector<glm::mat4>& instances)
            {
                if (!instances.empty() && shader && model)
                {
                    threadQueue.Submit([shader, model, instances]() {
                        model->DrawInstanced(*shader, instances, false);
                    });
                }
            };

            for (size_t k = startIdx; k < endIdx; ++k)
            {
                const auto& item = queue[k];
                entt::entity entity = item.entity;
                Model* actModel = item.activeModel;
                MaterialComponent *material = item.activeMaterial;
                Shader *itemShader = item.activeShader;
                if (overrideShader) {
                    // Deferred Pass (G-Buffer) or Shadow Pass (though Shadow doesn't call ExecuteQueue currently)
                    if (!itemShader) itemShader = overrideShader;
                } else if (!itemShader) {
                    // Forward Pass Opaque/Transparent
                    itemShader = m_UnlitShader.get();
                }

                if (!itemShader) continue;

                if (currentShader != itemShader)
                {
                    flushBatch(currentShader, currentModel, instanceBatch);
                    instanceBatch.clear();
                    currentShader = itemShader;
                    currentModel = nullptr;

                    Shader* s = currentShader;
                    bool enableShadows = shadowRenderer && shadowRenderer->IsShadowsEnabled() && shadowRenderer->GetShadowMode() > 0;
                    Shadow* shadowObj = shadowRenderer ? &shadowRenderer->GetShadow() : nullptr;
                    bool isDebugNoTexture = m_DebugNoTexture;

                    threadQueue.Submit([=]() {
                        s->use();
                        if (enableShadows && shadowObj) {
                            for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i) {
                                shadowObj->BindTexture_Dir(i, 10 + i);
                                s->setInt(("shadowMapDir[" + std::to_string(i) + "]").c_str(), 10 + i);
                            }
                        }
                        // Fog removed
                        if (shadowRenderer) {
                            s->setFloat("u_ShadowBias", shadowRenderer->GetShadowBias());
                            s->setInt("u_ShadowSoftness", shadowRenderer->GetShadowSoftness());
                        }
                    });
                }

                bool hasAnimComp = scene.registry.all_of<AnimationComponent>(entity);
                bool isAnimated = hasAnimComp && scene.registry.get<AnimationComponent>(entity).animator;
                bool isNonStatic = actModel && !actModel->IsStatic();

                if (isAnimated || isNonStatic || !m_InstanceBatchingEnabled)
                {
                    flushBatch(currentShader, currentModel, instanceBatch);
                    instanceBatch.clear();
                    currentModel = nullptr;

                    glm::mat4 mtx = item.worldMatrix;
                    if (actModel && !isAnimated) mtx *= actModel->GetRootTransform();

                    // Still need renderer for tint color
                    auto* rendererPtr = scene.registry.try_get<MeshRendererComponent>(entity);
                    glm::vec4 tc = rendererPtr ? rendererPtr->color : glm::vec4(1.0f);
                    Shader* actShader = currentShader;
                    
                    if (actModel && actShader) {
                        std::vector<glm::mat4> transforms;
                        if (isAnimated) transforms = scene.registry.get<AnimationComponent>(entity).animator->GetFinalBoneMatrices();

                        threadQueue.Submit([=, &scene]() {
                            actShader->setMat4("model", mtx);
                            actShader->setVec4("tintColor", tc);
                            if (!transforms.empty()) actShader->setMat4Array("finalBonesMatrices", transforms);
                            bool matBound = materialRenderer->SetupMaterialUniforms(actShader, material, scene, m_DebugNoTexture);
                            actShader->setBool("isInstanced", false);
                            actShader->setUInt("entityID", static_cast<unsigned int>(entity));
                            actModel->Draw(*actShader, !matBound);
                        });
                    }
                }
                else
                {
                    if (currentModel != actModel || currentMaterial != material)
                    {
                        flushBatch(currentShader, currentModel, instanceBatch);
                        instanceBatch.clear();
                        currentModel = actModel;
                        currentMaterial = material;
                        
                        auto* rendererPtr = scene.registry.try_get<MeshRendererComponent>(entity);
                        glm::vec4 tc = rendererPtr ? rendererPtr->color : glm::vec4(1.0f);
                        Shader* actShader = currentShader;
                        threadQueue.Submit([=, &scene]() {
                            actShader->setVec4("tintColor", tc);
                            bool matBound = materialRenderer->SetupMaterialUniforms(actShader, material, scene, m_DebugNoTexture);
                            actShader->setBool("isInstanced", false);
                            actShader->setUInt("entityID", static_cast<unsigned int>(entity));
                        });
                    }
                    instanceBatch.push_back(item.worldMatrix * (actModel ? actModel->GetRootTransform() : glm::mat4(1.0f)));
                }
            }
            flushBatch(currentShader, currentModel, instanceBatch);
        }, &counter);
    }
    JobSystem::Instance().Wait(&counter);

    m_CommandQueue.Clear();
    for (auto& tq : threadQueues) m_CommandQueue.Merge(tq);
    m_CommandQueue.Execute();
}

std::vector<entt::id_type> RenderSystem::GetReadComponents() const
{
    return {
        entt::type_id<MeshRendererComponent>().hash(),
        entt::type_id<CameraComponent>().hash(),
        entt::type_id<PositionComponent>().hash(),
        entt::type_id<RotationComponent>().hash(),
        entt::type_id<ScaleComponent>().hash(),
        entt::type_id<WorldTransformComponent>().hash(),
        entt::type_id<MaterialComponent>().hash(),
        entt::type_id<SkyboxRenderComponent>().hash(),
        entt::type_id<GPUDirLight>().hash(),
        entt::type_id<GPUPointLight>().hash(),
        entt::type_id<GPUSpotLight>().hash(),
        entt::type_id<AnimationComponent>().hash(),
        entt::type_id<StreamingComponent>().hash(),
        entt::type_id<UIRendererComponent>().hash()
    };
}

std::vector<entt::id_type> RenderSystem::GetWriteComponents() const
{
    return {
        entt::type_id<OcclusionComponent>().hash()
    };
}
void RenderSystem::RenderAlpha(Scene& scene, int width, int height, float alpha)
{
    // Rendering with alpha is usually handled by other systems like TransparentSystem or ParticleSystem
    // But we need to implement it as part of IRenderSystem if called directly
}

void RenderSystem::RenderTransparent(Scene& scene, int width, int height, float alpha)
{
    // Transparent rendering logic
}

void RenderSystem::UpdateGlobalLightData(const GPUGlobalLightData& data)
{
    auto& context = ServiceLocator::Instance().Require<IGraphicsContext>();
    auto& bm = context.GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalLightData), &data);
}

RenderPath RenderSystem::GetRenderPath() const
{
    auto& sl = ServiceLocator::Instance();
    auto& config = sl.Require<ConfigManager>().GetConfig();
    return config.renderPath;
}
