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
#include <thread>
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
#include <render/interface/i_render_target_manager.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <core/logic/debug_core.h>
#endif

void RenderSystem::Initialize(IGraphicsContext &context, IShaderLibrary &shaderLib)
{
    m_Context = &context;

    LOGGER_INFO("RenderSystem") << "Initializing shadow and light renderers";
    m_ShadowRenderer.Initialize(context, shaderLib);
    m_LightRenderer.Initialize(*m_Context);

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
    m_OcclusionCuller.Initialize(*m_Context, shaderLib.GetShader("occlusion_query"));
    m_MaterialRenderer.Initialize(*m_Context, m_WhiteTextureID);

    shaderLib.LoadShader("gbuffer", "includes/engine/asset/shaders/gbuffer.vs", "includes/engine/asset/shaders/gbuffer.fs");
    m_GBufferShader = shaderLib.GetShader("gbuffer");

    shaderLib.LoadShader("terrain_gbuffer", "includes/engine/asset/shaders/terrain_gbuffer.vs", "includes/engine/asset/shaders/terrain_gbuffer.fs");

    shaderLib.LoadShader("deferred_light", "includes/engine/asset/shaders/fxaa.vs", "includes/engine/asset/shaders/deferred_light.fs");
    m_DeferredLightShader = shaderLib.GetShader("deferred_light");

    InitQuad();
    m_DeferredRenderingEnabled = true;
}

void RenderSystem::Shutdown()
{
    LOGGER_INFO("RenderSystem") << "Shutting down RenderSystem";
    m_ShadowRenderer.Shutdown();
    m_OcclusionCuller.Shutdown();
    m_GBuffer.Shutdown();

    if (m_QuadVAO.IsValid()) m_Context->GetBufferManager().DeleteVertexArray(m_QuadVAO.id);
    if (m_QuadVBO.IsValid()) m_Context->GetBufferManager().DeleteBuffer(m_QuadVBO.id);
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

void RenderSystem::InitQuad()
{
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    auto& bm = m_Context->GetBufferManager();
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

    if (m_LastWidth != width || m_LastHeight != height) {
        m_GBuffer.Resize(width, height);
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

void RenderSystem::RenderAlpha(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled || !m_Context) return;

    BuildRenderQueues(scene, alpha, width, height);

    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity == entt::null) return;

    CameraComponent *cam = &scene.registry.get<CameraComponent>(camEntity);
    PositionComponent *camPosComp = scene.registry.try_get<PositionComponent>(camEntity);
    glm::vec3 camPos = camPosComp ? camPosComp->value : glm::vec3(0.0f);
    glm::mat4 projectionMatrix = m_JitteredProjection;

    m_LightRenderer.UploadLightData(scene, nullptr);

    // Update UBOs
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

    // Update Global Data UBO
    static auto startTime = std::chrono::steady_clock::now();
    auto currentTime = std::chrono::steady_clock::now();
    m_GlobalData.time = std::chrono::duration<float>(currentTime - startTime).count();
    m_GlobalData.resolution[0] = (float)width; m_GlobalData.resolution[1] = (float)height;
    bm.BindBuffer(BufferType::UniformBuffer, m_GlobalDataUBO->Get());
    bm.BufferSubData(BufferType::UniformBuffer, 0, sizeof(GPUGlobalData), &m_GlobalData);

    auto &rsm = m_Context->GetRenderStateManager();
    
    auto& dc = m_Context->GetDrawContext();
    // 1. Geometry Pass for Opaque
    if (m_DeferredRenderingEnabled) {
        m_GBuffer.BindForWriting();
        rsm.SetViewport(0, 0, width, height);
        dc.ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        dc.Clear(BufferBit::Color | BufferBit::Depth);
    }
    
    if (!m_DeferredRenderingEnabled) dc.Clear(BufferBit::Color | BufferBit::Depth);
    
    // Only reset m_RenderedCount if we are starting the Opaque pass
    m_RenderedCount = 0; 

    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthFunc(CompareFunc::Less);
    rsm.Enable(ServerCapability::CullFace);
    rsm.SetCullFace(CullMode::Back);

    m_CommandQueue.Clear();
    const auto& opaqueQueue = m_RenderQueueObj.GetOpaqueQueue();
    if (!opaqueQueue.empty())
    {
        ExecuteQueue(scene, opaqueQueue, false); // false = opaque
    }

    if (m_DeferredRenderingEnabled) m_GBuffer.Unbind();
    m_QueuesBuilt = false;
}

void RenderSystem::RenderTransparent(Scene& scene, int width, int height, float alpha)
{
    if (!m_Enabled || !m_Context) return;
    
    auto& rsm = m_Context->GetRenderStateManager();
    rsm.Enable(ServerCapability::Blend);
    rsm.SetBlendFunc(BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha);
    rsm.Enable(ServerCapability::DepthTest);
    rsm.SetDepthMask(false);

    const auto& transparentQueue = m_RenderQueueObj.GetTransparentQueue();
    if (!transparentQueue.empty())
    {
        ExecuteQueue(scene, transparentQueue, true);
    }

    rsm.Disable(ServerCapability::Blend);
    rsm.SetDepthMask(true);
}

void RenderSystem::ExecuteQueue(Scene& scene, const std::vector<RenderItem>& queue, bool isTransparentPass)
{
    size_t totalItems = queue.size();
    size_t numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 1;
    size_t chunkSize = (totalItems + numThreads - 1) / numThreads;

    std::vector<CommandQueue> threadQueues(numThreads);
    JobSystem::JobCounter counter(0);
    
    // Diagnostic log for the first few frames
    static int logCounter = 0;
    bool shouldLog = (logCounter < 5);
    if (shouldLog) {
        LOGGER_INFO("RenderSystem") << "ExecuteQueue: totalItems=" << totalItems << " isTransparentPass=" << isTransparentPass;
        logCounter++;
    }
    
    // Increment total rendered count for UI (only once per pass type)
    if (!isTransparentPass) m_RenderedCount += (int)totalItems;
    else m_RenderedCount += (int)totalItems; // Transparent also counts for "Rendered"

    for (size_t i = 0; i < numThreads; ++i)
    {
        size_t startIdx = i * chunkSize;
        if (startIdx >= totalItems) break;
        size_t endIdx = std::min(startIdx + chunkSize, totalItems);

        JobSystem::Instance().Execute([this, &scene, &queue, startIdx, endIdx, &threadQueue = threadQueues[i], isTransparentPass]() {
            Shader *currentShader = nullptr;
            Model *currentModel = nullptr;
            MaterialComponent *currentMaterial = nullptr;
            std::vector<glm::mat4> instanceBatch;

            auto* materialRenderer = &m_MaterialRenderer;
            auto* shadowRenderer = &m_ShadowRenderer;

            auto flushBatch = [&](Shader *shader, Model *model, const std::vector<glm::mat4>& instances)
            {
                if (!instances.empty() && shader && model)
                {
                    threadQueue.Submit([shader, model, instances]() {
                        model->DrawInstanced(*shader, instances);
                    });
                }
            };

            for (size_t k = startIdx; k < endIdx; ++k)
            {
                const auto& item = queue[k];
                if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity))
                    continue;

                auto &renderer = scene.registry.get<MeshRendererComponent>(item.entity);
                auto *material = scene.registry.try_get<MaterialComponent>(item.entity);
                entt::entity entity = item.entity;
                Model* actModel = item.activeModel;

                auto lockedShader = renderer.shader.lock();
                if (!lockedShader) continue;
                Shader *itemShader = lockedShader.get();

                if (m_DeferredRenderingEnabled && !isTransparentPass) {
                    itemShader = m_GBufferShader.get();
                }

                if (currentShader != itemShader)
                {
                    flushBatch(currentShader, currentModel, instanceBatch);
                    instanceBatch.clear();
                    currentShader = itemShader;
                    currentModel = nullptr;

                    Shader* s = currentShader;
                    bool enableShadows = shadowRenderer->IsShadowsEnabled() && shadowRenderer->GetShadowMode() > 0;
                    Shadow* shadowObj = &shadowRenderer->GetShadow();
                    bool isDebugNoTexture = m_DebugNoTexture;
                    bool isGBuffer = (m_GBufferShader && s == m_GBufferShader.get());

                    threadQueue.Submit([=]() {
                        s->use();
                        if (enableShadows && !isGBuffer) {
                            for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i) {
                                shadowObj->BindTexture_Dir(i, 10 + i);
                                s->setInt("shadowMapDir[" + std::to_string(i) + "]", 10 + i);
                            }
                        }
                        s->setBool("debug_noTexture", isDebugNoTexture);
                    });
                }

                bool hasAnimComp = scene.registry.all_of<AnimationComponent>(entity);
                bool isAnimated = hasAnimComp && scene.registry.get<AnimationComponent>(entity).animator;
                bool isNonStatic = item.activeModel && !item.activeModel->IsStatic();

                if (isAnimated || isNonStatic || !m_InstanceBatchingEnabled)
                {
                    flushBatch(currentShader, currentModel, instanceBatch);
                    instanceBatch.clear();
                    currentModel = nullptr;

                    glm::mat4 mtx = item.worldMatrix;
                    if (actModel && !isAnimated) mtx *= actModel->GetRootTransform();

                    glm::vec4 tc = renderer.color;
                    Shader* actShader = currentShader;
                    
                    if (actModel && actShader) {
                        std::vector<glm::mat4> transforms;
                        if (isAnimated) transforms = scene.registry.get<AnimationComponent>(entity).animator->GetFinalBoneMatrices();

                        threadQueue.Submit([=, &scene]() {
                            actShader->setMat4("model", mtx);
                            actShader->setVec4("tintColor", tc);
                            if (!transforms.empty()) actShader->setMat4Array("finalBonesMatrices", transforms);
                            bool matBound = materialRenderer->SetupMaterialUniforms(actShader, entity, scene, m_DebugNoTexture);
                            actShader->setBool("isInstanced", false);
                            actShader->setUInt("entityID", static_cast<unsigned int>(entity));
                            actModel->Draw(*actShader, !matBound);
                        });
                    }
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
                        Shader* actShader = currentShader;
                        threadQueue.Submit([=, &scene]() {
                            actShader->setVec4("tintColor", tc);
                            bool matBound = materialRenderer->SetupMaterialUniforms(actShader, entity, scene, m_DebugNoTexture);
                            actShader->setBool("isInstanced", false);
                            actShader->setUInt("entityID", static_cast<unsigned int>(entity));
                            // For batching, we currently don't have per-mesh fallback easily here as it's deferred to flushBatch
                            // But we'll at least pass !matBound to subsequent individual draws if possible.
                            // However, flushBatch calls DrawInstanced(..., true) by default now in my plan.
                        });
                    }
                    instanceBatch.push_back(item.worldMatrix * (item.activeModel ? item.activeModel->GetRootTransform() : glm::mat4(1.0f)));
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

void RenderSystem::BindForDecals()
{
    if (m_DeferredRenderingEnabled)
    {
        auto& rtm = m_Context->GetRenderTargetManager();
        auto& rsm = m_Context->GetRenderStateManager();
        
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, m_GBuffer.GetFBO());
        rsm.SetViewport(0, 0, m_GBuffer.GetWidth(), m_GBuffer.GetHeight());
        
        // We only want to write to Albedo (Color2)
        // By ONLY enabling Color2 in DrawBuffers, some drivers allow sampling from other attachments.
        FramebufferAttachment att = FramebufferAttachment::Color2;
        rtm.DrawBuffers(1, &att);
        
        // Ensure G-Buffer textures are bound for sampling in DecalSystem
    }
}

void RenderSystem::UnbindForDecals()
{
    if (m_DeferredRenderingEnabled)
    {
        auto& rtm = m_Context->GetRenderTargetManager();
        
        // Restore all 4 draw buffers for subsequent passes if any
        FramebufferAttachment attachments[] = { 
            FramebufferAttachment::Color0, 
            FramebufferAttachment::Color1, 
            FramebufferAttachment::Color2,
            FramebufferAttachment::Color3 
        };
        rtm.DrawBuffers(4, attachments);
        
        rtm.BindFramebuffer(FramebufferTarget::Framebuffer, 0);
    }
}

void RenderSystem::UnbindGBuffer()
{
    if (m_DeferredRenderingEnabled)
    {
        m_GBuffer.Unbind();
    }
}

void RenderSystem::RenderDeferredLighting(Scene &scene, int width, int height)
{
    if (!m_DeferredRenderingEnabled || !m_DeferredLightShader) return;
    
    UnbindGBuffer();
    auto& rsm = m_Context->GetRenderStateManager();
    rsm.SetViewport(0, 0, width, height);
    rsm.Disable(ServerCapability::DepthTest);
    rsm.Disable(ServerCapability::CullFace);

    auto& tm = m_Context->GetTextureManager();
    auto& bm = m_Context->GetBufferManager();
    auto& dc = m_Context->GetDrawContext();
    
    // We do NOT clear the target buffer here because skybox might already be there.
    // However, if we discard in shader, we are fine.
    // If the user expects a specific clear color, it was already handled by PostProcessBeginCapture or Skybox.
    
    m_DeferredLightShader->use();
    
    // Bind G-Buffer textures
    tm.ActiveTexture(TextureUnit::Texture0);
    tm.BindTexture(TextureType::Texture2D, m_GBuffer.GetPositionTexture());
    m_DeferredLightShader->setInt("gPosition", 0);
    
    tm.ActiveTexture(TextureUnit::Texture1);
    tm.BindTexture(TextureType::Texture2D, m_GBuffer.GetNormalTexture());
    m_DeferredLightShader->setInt("gNormal", 1);
    
    tm.ActiveTexture(TextureUnit::Texture2);
    tm.BindTexture(TextureType::Texture2D, m_GBuffer.GetAlbedoSpecTexture());
    m_DeferredLightShader->setInt("gAlbedoSpec", 2);
    
    m_DeferredLightShader->setInt("u_DebugMode", 0); // Restore Lit mode

    // Bind Shadow Maps
    bool enableShadows = m_ShadowRenderer.IsShadowsEnabled() && m_ShadowRenderer.GetShadowMode() > 0;
    Shadow& shadowObj = m_ShadowRenderer.GetShadow();
    if (enableShadows) {
        for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i) {
            shadowObj.BindTexture_Dir(i, 10 + i);
            m_DeferredLightShader->setInt("shadowMapDir[" + std::to_string(i) + "]", 10 + i);
        }
    }

    // Draw full-screen quad
    bm.BindVertexArray(m_QuadVAO.id);
    dc.DrawArrays(Primitive::TriangleStrip, 0, 4);
    bm.BindVertexArray(0);

    rsm.Enable(ServerCapability::DepthTest);
}





void RenderSystem::Render(Scene &scene)
{
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
