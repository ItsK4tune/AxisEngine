#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/media_components.h>
#include <ecs/unit/ui_components.h>
#include <core/logic/job_system.h>
#include <ecs/logic/render_system.h>
#include <render/logic/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <core/logic/logger.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <resource/manager/resource_manager.h>
#include <render/interface/i_graphics_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_query_manager.h>
#include <ecs/manager/entity_manager.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <core/logic/debug_core.h>
#endif

void RenderSystem::Init(IGraphicsContext &context, IShaderLibrary &shaderLib)
{
    m_Context = &context;

    LOGGER_INFO("RenderSystem") << "Initializing shadow and light renderers";
    m_ShadowRenderer.Init(context, shaderLib);
    m_LightRenderer.Init(*m_Context);

    if (m_WhiteTextureID == 0)
    {
        auto &tm = m_Context->GetTextureManager();
        m_WhiteTextureID = tm.GenTexture();
        tm.BindTexture(TextureType::Texture2D, m_WhiteTextureID);
        unsigned char white[] = {255, 255, 255, 255};

        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0,
                      TextureFormat::RGBA, DataType::UnsignedByte, white);

        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter, static_cast<int>(TextureFilter::Nearest));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Nearest));
    }

    m_BonesUniforms.reserve(200);
    for (int i = 0; i < 200; ++i)
        m_BonesUniforms.push_back("finalBonesMatrices[" + std::to_string(i) + "]");

    auto& bm = m_Context->GetBufferManager();
    m_CameraUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUCameraData), nullptr, BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::UniformBuffer, 0, m_CameraUBO->Get());

    m_GlobalLightUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalLightData), nullptr, BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::UniformBuffer, 1, m_GlobalLightUBO->Get());

    m_GlobalDataUBO = std::make_unique<GPUUBO>(context, bm.CreateBuffer());
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferData(BufferType::UniformBuffer, sizeof(GPUGlobalData), nullptr, BufferUsage::DynamicDraw);
    bm.BindBufferBase(BufferType::UniformBuffer, 2, m_GlobalDataUBO->Get());

    shaderLib.LoadShader("occlusion_query", "includes/engine/asset/shaders/occlusion_query.vs", "includes/engine/asset/shaders/occlusion_query.fs");
    m_OcclusionCuller.Init(*m_Context, shaderLib.GetShader("occlusion_query"));
    m_MaterialRenderer.Init(*m_Context, m_WhiteTextureID);
}

void RenderSystem::Shutdown()
{
    LOGGER_INFO("RenderSystem") << "Shutting down RenderSystem";
    m_ShadowRenderer.Shutdown();

    m_OcclusionCuller.Shutdown();
}

void RenderSystem::SetFaceCulling(bool enabled, CullMode mode)
{
    if (!m_Context)
        return;
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

void RenderSystem::SetDepthTest(bool enabled, CompareFunc func)
{
    if (!m_Context)
        return;
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

void RenderSystem::BuildRenderQueues(Scene &scene, float alpha, int width, int height)
{
    if (m_QueuesBuilt && m_LastAlpha == alpha && m_LastWidth == width && m_LastHeight == height) {
        return;
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

    glm::mat4 stableVP = cam->projectionMatrix * cam->viewMatrix;
    m_FrustumCuller.BuildFrustum(stableVP);

    if (m_OcclusionCullingEnabled) {
        m_OcclusionCuller.UpdateResults(scene);
    }

    m_RenderQueueObj.Build(scene, alpha, m_FrustumCuller, m_FrustumCullingEnabled, m_OcclusionCullingEnabled, 
                           m_DistanceCullingSq, m_FilterLayerMask, cam->cullingMask, camPos);

    m_QueuesBuilt = true;
    m_LastAlpha = alpha;
}

void RenderSystem::RenderShadows(Scene &scene)
{
    m_ShadowRenderer.RenderShadows(scene, m_RenderQueueObj.GetShadowQueue());
}

void RenderSystem::RenderAlpha(Scene &scene, int width, int height, float alpha)
{
    if (!m_Enabled || !m_Context)
    {
        return;
    }

    BuildRenderQueues(scene, alpha, width, height);

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null)
        return;

    CameraComponent *cam = &scene.registry.get<CameraComponent>(camEntity);
    PositionComponent *camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);
    glm::mat4 projectionMatrix = m_JitteredProjection;

    Shader *currentShader = nullptr;
    Model *currentModel = nullptr;
    MaterialComponent *currentMaterial = nullptr;
    std::vector<glm::mat4> instanceBatch;
    m_RenderedCount = 0;

    m_LightRenderer.UploadLightData(scene, nullptr);

    // Update Camera UBO
    std::memcpy(m_CameraData.projection, &projectionMatrix[0][0], 16 * sizeof(float));
    std::memcpy(m_CameraData.view, &cam->viewMatrix[0][0], 16 * sizeof(float));
    std::memcpy(m_CameraData.viewPos, &camPos[0], 3 * sizeof(float));
    auto& bm = m_Context->GetBufferManager();
    bm.BindBuffer(BufferType::UniformBuffer, m_CameraUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUCameraData), &m_CameraData);

    // Update Global Light UBO
    m_GlobalLightData.numDirLights = m_LightRenderer.GetDirLightCount();
    m_GlobalLightData.nrPointLights = m_LightRenderer.GetPointLightCount();
    m_GlobalLightData.nrSpotLights = m_LightRenderer.GetSpotLightCount();
    m_GlobalLightData.farPlanePoint = m_ShadowRenderer.GetFarPlanePoint();
    m_GlobalLightData.farPlaneSpot = m_ShadowRenderer.GetFarPlaneSpot();
    m_GlobalLightData.u_ReceiveShadow = (m_ShadowRenderer.IsShadowsEnabled() && m_ShadowRenderer.GetShadowMode() > 0) ? 1 : 0;
    
    const glm::mat4* lsmDir = m_ShadowRenderer.GetLightSpaceMatrices();
    const glm::mat4* lsmSpot = m_ShadowRenderer.GetLightSpaceMatricesSpot();
    if (lsmDir) std::memcpy(m_GlobalLightData.lightSpaceMatricesDir, lsmDir, Shadow::MAX_DIR_LIGHTS_SHADOW * 16 * sizeof(float));
    if (lsmSpot) std::memcpy(m_GlobalLightData.lightSpaceMatricesSpot, lsmSpot, Shadow::MAX_SPOT_LIGHTS_SHADOW * 16 * sizeof(float));

    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalLightUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalLightData), &m_GlobalLightData);

    // Update Global Data UBO (Gate)
    static auto startTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    m_GlobalData.time = std::chrono::duration<float>(currentTime - startTime).count();
    static auto lastTime = startTime;
    m_GlobalData.deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
    m_GlobalData.resolution[0] = static_cast<float>(width);
    m_GlobalData.resolution[1] = static_cast<float>(height);

    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalData), &m_GlobalData);

    auto &rsm = m_Context->GetRenderStateManager();
    PolygonMode prevMode = rsm.GetPolygonMode();

#ifdef ENABLE_DEBUG_SYSTEM
    if (DebugConfig::ShowWireframe)
    {
        rsm.SetPolygonMode(CullMode::FrontAndBack, PolygonMode::Line);
    }
#endif

    m_CommandQueue.Clear();

    const auto& fullQueue = m_RenderQueueObj.GetOpaqueQueue();
    if (fullQueue.empty()) return;

    size_t totalItems = fullQueue.size();
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 1;
    size_t chunkSize = (totalItems + numThreads - 1) / numThreads;

    std::vector<CommandQueue> threadQueues(numThreads);
    JobSystem::JobCounter counter(0);

    for (size_t i = 0; i < numThreads; ++i)
    {
        size_t startIdx = i * chunkSize;
        if (startIdx >= totalItems) break;
        size_t endIdx = std::min(startIdx + chunkSize, totalItems);

        JobSystem::Instance().Execute([this, &scene, &fullQueue, startIdx, endIdx, &threadQueue = threadQueues[i], projectionMatrix, alpha]() {
            Shader *currentShader = nullptr;
            Model *currentModel = nullptr;
            MaterialComponent *currentMaterial = nullptr;
            std::vector<glm::mat4> instanceBatch;

            auto* context = m_Context;
            auto* materialRenderer = &m_MaterialRenderer;
            auto* shadowRenderer = &m_ShadowRenderer;
            auto* lightRenderer = &m_LightRenderer;

            auto flushBatch = [&](Shader *shader, Model *model, const std::vector<glm::mat4>& instances)
            {
                if (!instances.empty() && shader && model)
                {
                    threadQueue.Submit([shader, model, instances]() {
                        model->DrawInstanced(*shader, instances);
                    });
                }
            };

            entt::entity camEntity = EntityManager::GetActiveCamera(scene);
            CameraComponent *cam = &scene.registry.get<CameraComponent>(camEntity);
            PositionComponent *camPosComp = &scene.registry.get<PositionComponent>(camEntity);

            bool transparencyState = false;

            for (size_t k = startIdx; k < endIdx; ++k)
            {
                const auto& item = fullQueue[k];
                if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity))
                    continue;

                if (item.isTransparent && !transparencyState)
                {
                    flushBatch(currentShader, currentModel, instanceBatch);
                    instanceBatch.clear();
                    transparencyState = true;
                    threadQueue.Submit([context]() {
                        auto& rsm = context->GetRenderStateManager();
                        rsm.Enable(ServerCapability::Blend);
                        rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
                    });
                }

                auto &renderer = scene.registry.get<MeshRendererComponent>(item.entity);
                auto *material = scene.registry.try_get<MaterialComponent>(item.entity);
                entt::entity entity = item.entity;

                auto lockedShader = renderer.shader.lock();
                if (!lockedShader) continue;
                Shader *itemShader = lockedShader.get();

                if (currentShader != itemShader)
                {
                    flushBatch(currentShader, currentModel, instanceBatch);
                    instanceBatch.clear();
                    currentShader = itemShader;
                    currentModel = nullptr;
                    currentMaterial = nullptr;

                    Shader* s = currentShader;
                    bool enableShadows = shadowRenderer->IsShadowsEnabled() && shadowRenderer->GetShadowMode() > 0;
                    Shadow* shadowObj = &shadowRenderer->GetShadow();
                    
                    bool isDebugNoTexture = m_DebugNoTexture;

                    threadQueue.Submit([=]() {
                        s->use();
                        if (enableShadows) {
                            for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i) {
                                shadowObj->BindTexture_Dir(i, 10 + i);
                                s->setInt("shadowMapDir[" + std::to_string(i) + "]", 10 + i);
                            }
                            for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i) {
                                shadowObj->BindTexture_Point(i, 12 + i);
                                s->setInt("shadowMapPoint[" + std::to_string(i) + "]", 12 + i);
                            }
                            for (int i = 0; i < Shadow::MAX_SPOT_LIGHTS_SHADOW; ++i) {
                                shadowObj->BindTexture_Spot(i, 14 + i);
                                s->setInt("shadowMapSpot[" + std::to_string(i) + "]", 14 + i);
                            }
                        }
                        s->setBool("debug_noTexture", isDebugNoTexture);
                    });
                }

                bool hasAnimComp = scene.registry.all_of<AnimationComponent>(entity);
                bool isAnimated = hasAnimComp && scene.registry.get<AnimationComponent>(entity).animator;
                bool isNonStatic = item.activeModel && !item.activeModel->IsStatic();

                if (isAnimated || isNonStatic)
                {
                    flushBatch(currentShader, currentModel, instanceBatch);
                    instanceBatch.clear();
                    currentModel = nullptr;
                    currentMaterial = nullptr;

                    glm::mat4 mtx = item.worldMatrix;
                    glm::vec4 tc = renderer.color;
                    bool noTex = m_DebugNoTexture;
                    Model* actModel = item.activeModel;
                    Shader* actShader = currentShader;
                    bool enableBlend = transparencyState || item.isTransparent;
                    BlendFactor bSrc = material ? material->desc.blendSrc : BlendFactor::SrcAlpha;
                    BlendFactor bDst = material ? material->desc.blendDst : BlendFactor::OneMinusSrcAlpha;
                    bool matHasTextures = (material && (material->gpu.albedoMap != 0 || material->gpu.normalMap != 0 || material->gpu.metallicMap != 0 || material->gpu.roughnessMap != 0 || material->gpu.aoMap != 0 || material->gpu.emissiveMap != 0));

                    std::vector<glm::mat4> transforms;
                    if (isAnimated) {
                        auto &animComp = scene.registry.get<AnimationComponent>(entity);
                        if (animComp.animator) transforms = animComp.animator->GetFinalBoneMatrices();
                    } else if (isNonStatic && actModel) {
                        transforms.assign(200, actModel->GetRootTransform());
                    }

                    threadQueue.Submit([=, &scene]() {
                        actShader->setMat4("model", mtx);
                        actShader->setVec4("tintColor", tc);
                        if (!transforms.empty()) actShader->setMat4Array("finalBonesMatrices", transforms);
                        if (enableBlend && matHasTextures) context->GetRenderStateManager().SetBlendFunc(bSrc, bDst);
                        materialRenderer->SetupMaterialUniforms(actShader, entity, scene, noTex);
                        if (actModel) actModel->Draw(*actShader, !matHasTextures);
                    });
                }
                else
                {
                    bool matHasTextures = (material && (material->gpu.albedoMap != 0 || material->gpu.normalMap != 0 || material->gpu.metallicMap != 0 || material->gpu.roughnessMap != 0 || material->gpu.aoMap != 0 || material->gpu.emissiveMap != 0));
                    if (!m_InstanceBatchingEnabled || matHasTextures)
                    {
                        flushBatch(currentShader, currentModel, instanceBatch);
                        instanceBatch.clear();
                        currentModel = nullptr;
                        currentMaterial = nullptr;
                        glm::mat4 mtx = item.worldMatrix * item.activeModel->GetRootTransform();
                        glm::vec4 tc = renderer.color;
                        bool noTex = m_DebugNoTexture;
                        Model* actModel = item.activeModel;
                        Shader* actShader = currentShader;
                        bool enableBlend = transparencyState || item.isTransparent;
                        BlendFactor bSrc = material ? material->desc.blendSrc : BlendFactor::SrcAlpha;
                        BlendFactor bDst = material ? material->desc.blendDst : BlendFactor::OneMinusSrcAlpha;

                        threadQueue.Submit([=, &scene]() {
                            if (enableBlend && matHasTextures) context->GetRenderStateManager().SetBlendFunc(bSrc, bDst);
                            actShader->setMat4("model", mtx);
                            actShader->setVec4("tintColor", tc);
                            materialRenderer->SetupMaterialUniforms(actShader, entity, scene, noTex);
                            if (actModel) actModel->Draw(*actShader, !matHasTextures);
                        });
                    }
                    else
                    {
                        if (currentModel != item.activeModel || currentMaterial != material)
                        {
                            flushBatch(currentShader, currentModel, instanceBatch);
                            instanceBatch.clear();
                            currentModel = item.activeModel;
                            currentMaterial = material;
                            glm::vec4 tc = renderer.color;
                            bool noTex = m_DebugNoTexture;
                            Shader* actShader = currentShader;
                            threadQueue.Submit([=, &scene]() {
                                actShader->setVec4("tintColor", tc);
                                materialRenderer->SetupMaterialUniforms(actShader, entity, scene, noTex);
                            });
                        }
                        instanceBatch.push_back(item.worldMatrix * item.activeModel->GetRootTransform());
                    }
                }
            }
            flushBatch(currentShader, currentModel, instanceBatch);
            if (transparencyState) {
                threadQueue.Submit([context]() {
                    context->GetRenderStateManager().Disable(ServerCapability::Blend);
                });
            }
        }, &counter);
    }

    JobSystem::Instance().Wait(&counter);

    for (auto& tq : threadQueues)
    {
        m_CommandQueue.Merge(tq);
    }

    if (m_OcclusionCullingEnabled)
    {
        m_CommandQueue.Submit([=, &scene]() {
            m_OcclusionCuller.RenderQueries(scene, projectionMatrix, cam->viewMatrix, alpha);
        });
    }

#ifdef ENABLE_DEBUG_SYSTEM
    if (DebugConfig::ShowWireframe)
    {
        PolygonMode capMode = prevMode;
        m_CommandQueue.Submit([context = m_Context, capMode]() {
            context->GetRenderStateManager().SetPolygonMode(CullMode::FrontAndBack, capMode);
        });
    }
#endif

    m_CommandQueue.Execute();
    m_QueuesBuilt = false;
}





void RenderSystem::Render(Scene &scene)
{
}
