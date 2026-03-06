#include <core/job_system.h>
#include <ecs/systems/render_system.h>
#include <rendering/renderer/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <core/utils/logger.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <resource/resource_manager.h>
#include <rendering/interfaces/i_graphics_context.h>
#include <rendering/interfaces/i_texture_manager.h>
#include <rendering/interfaces/i_render_state_manager.h>
#include <rendering/interfaces/i_draw_context.h>
#include <rendering/interfaces/i_buffer_manager.h>
#include <rendering/interfaces/i_query_manager.h>
#include <ecs/entity_manager.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <core/debug/debug_config.h>
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
        tm.BindTexture(Graphics::TextureType::Texture2D, m_WhiteTextureID);
        unsigned char white[] = {255, 255, 255, 255};

        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA8, 1, 1, 0,
                      Graphics::TextureFormat::RGBA, Graphics::DataType::UnsignedByte, white);

        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
    }

    m_BonesUniforms.reserve(200);
    for (int i = 0; i < 200; ++i)
        m_BonesUniforms.push_back("finalBonesMatrices[" + std::to_string(i) + "]");

    m_ShadowPointUniforms.reserve(Shadow::MAX_POINT_LIGHTS_SHADOW);
    for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i)
        m_ShadowPointUniforms.push_back("shadowMapPoint[" + std::to_string(i) + "]");

    m_ShadowDirUniforms.reserve(Shadow::MAX_DIR_LIGHTS_SHADOW);
    for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i)
        m_ShadowDirUniforms.push_back("shadowMapDir[" + std::to_string(i) + "]");

    m_ShadowSpotUniforms.reserve(Shadow::MAX_SPOT_LIGHTS_SHADOW);
    for (int i = 0; i < Shadow::MAX_SPOT_LIGHTS_SHADOW; ++i)
        m_ShadowSpotUniforms.push_back("shadowMapSpot[" + std::to_string(i) + "]");

    m_LightSpaceMatrixUniforms.reserve(Shadow::MAX_DIR_LIGHTS_SHADOW);
    for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i)
        m_LightSpaceMatrixUniforms.push_back("lightSpaceMatrix[" + std::to_string(i) + "]");

    m_LightSpaceMatrixSpotUniforms.reserve(Shadow::MAX_SPOT_LIGHTS_SHADOW);
    for (int i = 0; i < Shadow::MAX_SPOT_LIGHTS_SHADOW; ++i)
        m_LightSpaceMatrixSpotUniforms.push_back("lightSpaceMatrixSpot[" + std::to_string(i) + "]");

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

void RenderSystem::SetFaceCulling(bool enabled, Graphics::CullMode mode)
{
    if (!m_Context)
        return;
    auto &rsm = m_Context->GetRenderStateManager();

    if (enabled)
    {
        rsm.Enable(Graphics::ServerCapability::CullFace);
        rsm.CullFace(mode);
    }
    else
    {
        rsm.Disable(Graphics::ServerCapability::CullFace);
    }
}

void RenderSystem::SetDepthTest(bool enabled, Graphics::CompareFunc func)
{
    if (!m_Context)
        return;
    auto &rsm = m_Context->GetRenderStateManager();

    if (enabled)
    {
        rsm.Enable(Graphics::ServerCapability::DepthTest);
        rsm.DepthFunc(func);
    }
    else
    {
        rsm.Disable(Graphics::ServerCapability::DepthTest);
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
    static int renderCount = 0;
    renderCount++;
    bool logThisRender = (renderCount <= 5);

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

    auto &rsm = m_Context->GetRenderStateManager();
    Graphics::PolygonMode prevMode = rsm.GetPolygonMode();

#ifdef ENABLE_DEBUG_SYSTEM
    if (DebugConfig::ShowWireframe)
    {
        rsm.PolygonMode(Graphics::CullMode::FrontAndBack, Graphics::PolygonMode::Line);
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
            TransformComponent *camTrans = &scene.registry.get<TransformComponent>(camEntity);

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
                        rsm.Enable(Graphics::ServerCapability::Blend);
                        rsm.BlendFunc(Graphics::BlendFactor::SrcAlpha, Graphics::BlendFactor::OneMinusSrcAlpha);
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
                    float farP = shadowRenderer->GetFarPlanePoint();
                    float farS = shadowRenderer->GetFarPlaneSpot();
                    Shadow* shadowObj = &shadowRenderer->GetShadow();

                    glm::mat4 viewMat = cam->viewMatrix;
                    glm::vec3 viewPos = camTrans->position;
                    
                    const glm::mat4* lsmDir = shadowRenderer->GetLightSpaceMatrices();
                    const glm::mat4* lsmSpot = shadowRenderer->GetLightSpaceMatricesSpot();
                    
                    auto shadowDirUniforms = m_ShadowDirUniforms;
                    auto shadowPointUniforms = m_ShadowPointUniforms;
                    auto shadowSpotUniforms = m_ShadowSpotUniforms;
                    auto lsmUniforms = m_LightSpaceMatrixUniforms;
                    auto lsmsUniforms = m_LightSpaceMatrixSpotUniforms;
                    
                    bool isDebugNoTexture = m_DebugNoTexture;
                    int numDir = lightRenderer->GetDirLightCount();
                    int numPoint = lightRenderer->GetPointLightCount();
                    int numSpot = lightRenderer->GetSpotLightCount();

                    threadQueue.Submit([=]() {
                        s->use();
                        s->setMat4("projection", projectionMatrix);
                        s->setMat4("view", viewMat);
                        s->setVec3("viewPos", viewPos);
                        if (enableShadows) {
                            s->setBool("u_ReceiveShadow", true);
                            for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i) {
                                shadowObj->BindTexture_Dir(i, 10 + i);
                                s->setInt(shadowDirUniforms[i], 10 + i);
                                s->setMat4(lsmUniforms[i], lsmDir[i]);
                            }
                            for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i) {
                                shadowObj->BindTexture_Point(i, 12 + i);
                                s->setInt(shadowPointUniforms[i], 12 + i);
                            }
                            for (int i = 0; i < Shadow::MAX_SPOT_LIGHTS_SHADOW; ++i) {
                                shadowObj->BindTexture_Spot(i, 14 + i);
                                s->setInt(shadowSpotUniforms[i], 14 + i);
                                s->setMat4(lsmsUniforms[i], lsmSpot[i]);
                            }
                        } else {
                            s->setBool("u_ReceiveShadow", false);
                        }
                        s->setFloat("farPlanePoint", farP);
                        s->setFloat("farPlaneSpot", farS);
                        s->setBool("debug_noTexture", isDebugNoTexture);
                        s->setInt("numDirLights", numDir);
                        s->setInt("nrPointLights", numPoint);
                        s->setInt("nrSpotLights", numSpot);
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
                    Graphics::BlendFactor bSrc = material ? material->desc.blendSrc : Graphics::BlendFactor::SrcAlpha;
                    Graphics::BlendFactor bDst = material ? material->desc.blendDst : Graphics::BlendFactor::OneMinusSrcAlpha;
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
                        if (enableBlend && matHasTextures) context->GetRenderStateManager().BlendFunc(bSrc, bDst);
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
                        Graphics::BlendFactor bSrc = material ? material->desc.blendSrc : Graphics::BlendFactor::SrcAlpha;
                        Graphics::BlendFactor bDst = material ? material->desc.blendDst : Graphics::BlendFactor::OneMinusSrcAlpha;

                        threadQueue.Submit([=, &scene]() {
                            if (enableBlend && matHasTextures) context->GetRenderStateManager().BlendFunc(bSrc, bDst);
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
                    context->GetRenderStateManager().Disable(Graphics::ServerCapability::Blend);
                });
            }
        }, &counter);
    }

    JobSystem::Instance().Wait(&counter);

    // Merge all thread queues into main command queue
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
        Graphics::PolygonMode capMode = prevMode;
        m_CommandQueue.Submit([context = m_Context, capMode]() {
            context->GetRenderStateManager().PolygonMode(Graphics::CullMode::FrontAndBack, capMode);
        });
    }
#endif

    m_CommandQueue.Execute();
    m_QueuesBuilt = false;
}





void RenderSystem::Render(Scene &scene)
{
}
