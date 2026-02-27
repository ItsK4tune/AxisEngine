#include <ecs/systems/render_system.h>
#include <graphic/renderer/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <utils/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <resource/resource_manager.h>
#include <interface/graphic/i_graphics_context.h>
#include <interface/graphic/i_texture_manager.h>
#include <interface/graphic/i_render_state_manager.h>
#include <interface/graphic/i_draw_context.h>
#include <interface/graphic/i_buffer_manager.h>
#include <interface/graphic/i_query_manager.h>

#ifdef ENABLE_DEBUG_SYSTEM
#include <debug/debug_config.h>
#endif

void RenderSystem::Init(IGraphicsContext &context, ResourceManager &res)
{
    m_Context = &context;

    LOGGER_INFO("RenderSystem") << "Initializing shadow and light renderers";
    m_ShadowRenderer.Init(res);
    m_LightRenderer.Init(m_Context->GetBufferManager());

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

    res.LoadShader("occlusion_query", "includes/engine/asset/shaders/occlusion_query.vs", "includes/engine/asset/shaders/occlusion_query.fs");
    m_OcclusionQueryShader = res.GetShader("occlusion_query");

    InitOcclusionCube();
}

void RenderSystem::Shutdown()
{
    LOGGER_INFO("RenderSystem") << "Shutting down RenderSystem";
    m_ShadowRenderer.Shutdown();

    if (m_Context)
    {
        auto &bm = m_Context->GetBufferManager();
        auto &qm = m_Context->GetQueryManager();

        if (m_CubeVAO != 0)
            bm.DeleteVertexArray(m_CubeVAO);
        if (m_CubeVBO != 0)
            bm.DeleteBuffer(m_CubeVBO);
        if (m_CubeEBO != 0)
            bm.DeleteBuffer(m_CubeEBO);

        for (uint32_t queryId : m_OcclusionQueries)
        {
            qm.DeleteQuery(queryId);
        }
        m_OcclusionQueries.clear();
    }

    m_RenderQueue.clear();
    m_ShadowQueue.clear();
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
    if (m_QueuesBuilt && m_LastAlpha == alpha && m_LastWidth == width && m_LastHeight == height)
    {
        return;
    }

    m_LastWidth = width;
    m_LastHeight = height;

    m_RenderQueue.clear();
    m_ShadowQueue.clear();

    if (alpha <= 0.0f || alpha > 1.0f)
    {
        LOGGER_WARN("RenderSystem") << "Invalid alpha value: " << alpha;
    }

    entt::entity camEntity = scene.GetActiveCamera();
    CameraComponent *cam = nullptr;
    TransformComponent *camTrans = nullptr;

    if (camEntity == entt::null)
    {
        m_QueuesBuilt = true;
        m_LastAlpha = alpha;
        return;
    }

    cam = &scene.registry.get<CameraComponent>(camEntity);
    camTrans = &scene.registry.get<TransformComponent>(camEntity);

    if (!m_QueuesBuilt)
    {
        m_PrevViewProj = m_CurrViewProj;
    }

    m_JitteredProjection = cam->projectionMatrix;
    m_JitterOffset = glm::vec2(0.0f);

    if (m_AAMode == AntiAliasingMode::TAA && width > 0 && height > 0)
    {
        auto HaltonSequence = [](int index, int base) -> float
        {
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
        int frameIdx = m_FrameIndex % sampleCount;

        float jitterX = HaltonSequence(frameIdx + 1, 2) - 0.5f;
        float jitterY = HaltonSequence(frameIdx + 1, 3) - 0.5f;

        m_JitterOffset = glm::vec2(jitterX, jitterY);

        glm::mat4 jitterMatrix = glm::mat4(1.0f);
        jitterMatrix[3][0] = jitterX * 2.0f / (float)width;
        jitterMatrix[3][1] = jitterY * 2.0f / (float)height;

        m_JitteredProjection = jitterMatrix * m_JitteredProjection;

        if (!m_QueuesBuilt)
            m_FrameIndex++;
    }

    m_CurrViewProj = m_JitteredProjection * cam->viewMatrix;

    glm::mat4 stableVP = cam->projectionMatrix * cam->viewMatrix;
    Frustum frustum;
    frustum.Update(stableVP);

    if (m_OcclusionCullingEnabled)
    {
        UpdateOcclusionResults(scene);
    }

    std::vector<entt::entity> visibleEntities;
    if (scene.GetOctree())
    {
        std::vector<OctreeElement> elements;
        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : view)
        {
            auto [transform, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);
            if (!renderer.model)
                continue;

            glm::mat4 modelMatrix = transform.GetInterpolatedWorldMatrix(scene.registry, alpha);
            elements.push_back({entity, renderer.model->aabb.Transform(modelMatrix)});
        }
        scene.GetOctree()->Rebuild(elements);

        if (m_FrustumCullingEnabled)
            scene.GetOctree()->Query(frustum, visibleEntities);
        else
        {
            for (const auto &el : elements)
                visibleEntities.push_back(el.entity);
        }
    }
    else
    {
        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : view)
            visibleEntities.push_back(entity);
    }

    for (auto entity : visibleEntities)
    {
        if (!scene.registry.all_of<TransformComponent, MeshRendererComponent>(entity))
            continue;

        auto &transform = scene.registry.get<TransformComponent>(entity);
        auto &renderer = scene.registry.get<MeshRendererComponent>(entity);

        if (!renderer.model || renderer.shader.expired())
            continue;

        glm::mat4 modelMatrix = transform.GetInterpolatedWorldMatrix(scene.registry, alpha);

        float distSq = 0.0f;
        if (cam && camTrans)
        {
            glm::vec3 cameraPos = camTrans->position;
            glm::vec3 worldMin = modelMatrix * glm::vec4(renderer.model->aabb.minBound, 1.0f);
            glm::vec3 worldMax = modelMatrix * glm::vec4(renderer.model->aabb.maxBound, 1.0f);

            float dx = (std::max)(worldMin.x - cameraPos.x, (std::max)(0.0f, cameraPos.x - worldMax.x));
            float dy = (std::max)(worldMin.y - cameraPos.y, (std::max)(0.0f, cameraPos.y - worldMax.y));
            float dz = (std::max)(worldMin.z - cameraPos.z, (std::max)(0.0f, cameraPos.z - worldMax.z));

            distSq = dx * dx + dy * dy + dz * dz;

            if (m_DistanceCullingSq > 0.0f && distSq > m_DistanceCullingSq)
                continue;
        }

        MaterialComponent *mat = scene.registry.try_get<MaterialComponent>(entity);
        Model *activeModel = renderer.model.get();

        if (auto *lod = scene.registry.try_get<LODComponent>(entity))
        {
            for (int i = 0; i < (int)lod->lodDistancesSq.size(); ++i)
            {
                if (distSq > lod->lodDistancesSq[i] && i < (int)lod->lodModels.size() && lod->lodModels[i])
                {
                    activeModel = lod->lodModels[i].get();
                }
                else
                {
                    break;
                }
            }
        }

        uint32_t layer = 1;
        int renderOrder = renderer.order;

        if (auto *info = scene.registry.try_get<InfoComponent>(entity))
            layer = info->layer;

        if ((m_FilterLayerMask & layer) == 0)
            continue;
        if (cam && (cam->cullingMask & layer) == 0)
            continue;

        if (m_OcclusionCullingEnabled)
        {
            if (scene.registry.all_of<OcclusionComponent>(entity))
            {
                if (!scene.registry.get<OcclusionComponent>(entity).isVisible)
                    continue;
            }
        }

        m_RenderQueue.push_back({entity, activeModel, modelMatrix, layer, renderOrder, distSq});

        if (renderer.castShadow)
        {
            m_ShadowQueue.push_back({entity, activeModel, modelMatrix, layer, renderOrder, distSq});
        }
    }

    static int sortLogCount = 0;
    if (sortLogCount < 5)
    {
        sortLogCount++;
    }

    std::sort(m_RenderQueue.begin(), m_RenderQueue.end(), [&scene](const RenderItem &lhs, const RenderItem &rhs)
              {
        if (lhs.layer != rhs.layer)
            return lhs.layer < rhs.layer;
        if (lhs.renderOrder != rhs.renderOrder)
            return lhs.renderOrder < rhs.renderOrder;

        auto &lRenderer = scene.registry.get<MeshRendererComponent>(lhs.entity);
        auto &rRenderer = scene.registry.get<MeshRendererComponent>(rhs.entity);

        auto lShader = lRenderer.shader.lock();
        auto rShader = rRenderer.shader.lock();
        unsigned int lID = lShader ? lShader->getID() : 0;
        unsigned int rID = rShader ? rShader->getID() : 0;
        if (lID != rID)
            return lID < rID;
        
        auto lMat = scene.registry.try_get<MaterialComponent>(lhs.entity);
        auto rMat = scene.registry.try_get<MaterialComponent>(rhs.entity);
        if (lMat != rMat)
            return lMat < rMat;

        return lhs.activeModel < rhs.activeModel; });

    m_QueuesBuilt = true;
    m_LastAlpha = alpha;
}

void RenderSystem::RenderShadows(Scene &scene)
{
    m_ShadowRenderer.RenderShadows(scene, m_ShadowQueue);
}

void RenderSystem::Render(Scene &scene, int width, int height, float alpha)
{
    static int renderCount = 0;
    renderCount++;
    bool logThisRender = (renderCount <= 5);

    if (!m_Enabled || !m_Context)
    {
        return;
    }

    BuildRenderQueues(scene, alpha, width, height);

    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity == entt::null)
        return;

    CameraComponent *cam = &scene.registry.get<CameraComponent>(camEntity);
    TransformComponent *camTrans = &scene.registry.get<TransformComponent>(camEntity);

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

    auto flushBatch = [&](Shader *shader, Model *model)
    {
        if (!instanceBatch.empty() && shader && model)
        {
            model->DrawInstanced(*shader, instanceBatch);
            m_RenderedCount += instanceBatch.size();
            instanceBatch.clear();
        }
    };

    int lastRenderOrder = -1;
    bool firstItem = true;

    for (const auto &item : m_RenderQueue)
    {
        if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity))
            continue;

        auto &renderer = scene.registry.get<MeshRendererComponent>(item.entity);
        auto *material = scene.registry.try_get<MaterialComponent>(item.entity);

        if (m_RenderOrderEnabled)
        {
            if (firstItem || item.renderOrder != lastRenderOrder)
            {
                flushBatch(currentShader, currentModel);
                m_Context->GetDrawContext().Clear(Graphics::BufferBit::Depth);
                lastRenderOrder = item.renderOrder;
                firstItem = false;
            }
        }

        entt::entity entity = item.entity;

        auto lockedShader = renderer.shader.lock();
        if (!lockedShader)
            continue;
        Shader *itemShader = lockedShader.get();

        if (currentShader != itemShader)
        {
            flushBatch(currentShader, currentModel);
            currentShader = itemShader;
            currentModel = nullptr;
            currentMaterial = nullptr;
            currentShader->use();

            if (cam && camTrans)
            {
                currentShader->setMat4("projection", projectionMatrix);
                currentShader->setMat4("view", cam->viewMatrix);
                currentShader->setVec3("viewPos", camTrans->position);

                if (m_ShadowRenderer.IsShadowsEnabled() && m_ShadowRenderer.GetShadowMode() > 0)
                {
                    currentShader->setBool("u_ReceiveShadow", true);

                    for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i)
                    {
                        m_ShadowRenderer.GetShadow().BindTexture_Dir(i, 10 + i);
                        currentShader->setInt(m_ShadowDirUniforms[i], 10 + i);
                    }

                    for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i)
                    {
                        m_ShadowRenderer.GetShadow().BindTexture_Point(i, 12 + i);
                        currentShader->setInt(m_ShadowPointUniforms[i], 12 + i);
                    }

                    for (int i = 0; i < Shadow::MAX_SPOT_LIGHTS_SHADOW; ++i)
                    {
                        m_ShadowRenderer.GetShadow().BindTexture_Spot(i, 14 + i);
                        currentShader->setInt(m_ShadowSpotUniforms[i], 14 + i);
                    }

                    const glm::mat4 *lightSpaceMatrices = m_ShadowRenderer.GetLightSpaceMatrices();
                    for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i)
                    {
                        currentShader->setMat4(m_LightSpaceMatrixUniforms[i], lightSpaceMatrices[i]);
                    }

                    const glm::mat4 *lightSpaceMatricesSpot = m_ShadowRenderer.GetLightSpaceMatricesSpot();
                    for (int i = 0; i < Shadow::MAX_SPOT_LIGHTS_SHADOW; ++i)
                    {
                        currentShader->setMat4(m_LightSpaceMatrixSpotUniforms[i], lightSpaceMatricesSpot[i]);
                    }
                }
                else
                {
                    currentShader->setBool("u_ReceiveShadow", false);
                }

                currentShader->setFloat("farPlanePoint", m_ShadowRenderer.GetFarPlanePoint());
                currentShader->setFloat("farPlaneSpot", m_ShadowRenderer.GetFarPlaneSpot());
            }
            if (m_DebugNoTexture)
                currentShader->setBool("debug_noTexture", true);
            else
                currentShader->setBool("debug_noTexture", false);

            currentShader->setInt("numDirLights", m_LightRenderer.GetDirLightCount());
            currentShader->setInt("nrPointLights", m_LightRenderer.GetPointLightCount());
            currentShader->setInt("nrSpotLights", m_LightRenderer.GetSpotLightCount());
        }

        bool hasAnimComp = scene.registry.all_of<AnimationComponent>(entity);
        bool isAnimated = hasAnimComp && scene.registry.get<AnimationComponent>(entity).animator;
        bool isNonStatic = item.activeModel && !item.activeModel->IsStatic();

        if (isAnimated || isNonStatic)
        {
            flushBatch(currentShader, currentModel);
            currentModel = nullptr;
            currentMaterial = nullptr;

            currentShader->setMat4("model", item.worldMatrix);
            currentShader->setVec4("tintColor", renderer.color);

            if (isAnimated)
            {
                auto &animComp = scene.registry.get<AnimationComponent>(entity);
                if (animComp.animator)
                {
                    auto transforms = animComp.animator->GetFinalBoneMatrices();
                    currentShader->setMat4Array("finalBonesMatrices", transforms);
                }
                else
                {
                    if (logThisRender)
                        LOGGER_WARN("RenderSystem") << "[Render] isAnimated true but animator is null for entity: " << (uint32_t)entity;
                }
            }
            else if (isNonStatic && item.activeModel)
            {
                std::vector<glm::mat4> bindPoseMatrices(200, item.activeModel->GetRootTransform());
                currentShader->setMat4Array("finalBonesMatrices", bindPoseMatrices);
            }

            SetupMaterialUniforms(currentShader, entity, scene);

            if (item.activeModel)
            {
                item.activeModel->Draw(*currentShader);
            }
            m_RenderedCount++;
        }
            else
            {
                if (!m_InstanceBatchingEnabled)
                {
                    currentShader->setMat4("model", item.worldMatrix * item.activeModel->GetRootTransform());
                    currentShader->setVec4("tintColor", renderer.color);

                    SetupMaterialUniforms(currentShader, entity, scene);

                    item.activeModel->Draw(*currentShader);
                    m_RenderedCount++;
                }
                else
                {
                    if (currentModel != item.activeModel || currentMaterial != material)
                    {
                        flushBatch(currentShader, currentModel);
                        currentModel = item.activeModel;
                        currentMaterial = material;

                        currentShader->setVec4("tintColor", renderer.color);

                        SetupMaterialUniforms(currentShader, entity, scene);
                    }

                    instanceBatch.push_back(item.worldMatrix * item.activeModel->GetRootTransform());
                }
            }
    }
    flushBatch(currentShader, currentModel);

    if (m_OcclusionCullingEnabled)
    {
        RenderOcclusionQueries(scene, projectionMatrix, cam->viewMatrix, alpha);
    }

#ifdef ENABLE_DEBUG_SYSTEM
    if (DebugConfig::ShowWireframe)
    {
        rsm.PolygonMode(Graphics::CullMode::FrontAndBack, prevMode);
    }
#endif
    m_QueuesBuilt = false;
}

void RenderSystem::SetupMaterialUniforms(Shader *shader, entt::entity entity, Scene &scene)
{
    if (scene.registry.all_of<MaterialComponent>(entity))
    {
        auto &mat = scene.registry.get<MaterialComponent>(entity);
        if (mat.type == MaterialType::PBR)
        {
            shader->setFloat("material.roughness", mat.roughness);
            shader->setFloat("material.metallic", mat.metallic);
            shader->setFloat("material.ao", mat.ao);
            shader->setVec3("material.emission", mat.emission);
        }
        else
        {
            shader->setFloat("material.shininess", mat.shininess);
            shader->setVec3("material.specular", mat.specular);
            shader->setVec3("material.ambient", mat.ambient);
            shader->setVec3("material.emission", mat.emission);
        }
        shader->setFloat("material.opacity", mat.opacity);
        shader->setVec2("uvScale", mat.uvScale);
        shader->setVec2("uvOffset", mat.uvOffset);

        if (m_DebugNoTexture)
        {
            m_Context->GetTextureManager().ActiveTexture(Graphics::TextureUnit::Texture0);
            m_Context->GetTextureManager().BindTexture(Graphics::TextureType::Texture2D, m_WhiteTextureID);
        }
    }
    else
    {
        shader->setFloat("material.shininess", 32.0f);
        shader->setVec3("material.specular", glm::vec3(0.5f));
        shader->setVec3("material.ambient", glm::vec3(1.0f));
        shader->setVec3("material.emission", glm::vec3(0.0f));
        shader->setFloat("material.opacity", 1.0f);
        shader->setVec2("uvScale", glm::vec2(1.0f));
        shader->setVec2("uvOffset", glm::vec2(0.0f));
    }
}

void RenderSystem::InitOcclusionCube()
{
    float vertices[] = {
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f};

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 4, 7, 7, 3, 0,
        1, 5, 6, 6, 2, 1,
        0, 1, 5, 5, 4, 0,
        3, 2, 6, 6, 7, 3};

    auto &bm = m_Context->GetBufferManager();
    m_CubeVAO = bm.GenVertexArray();
    m_CubeVBO = bm.GenBuffer();
    m_CubeEBO = bm.GenBuffer();

    bm.BindVertexArray(m_CubeVAO);

    bm.BindBuffer(Graphics::BufferType::ArrayBuffer, m_CubeVBO);
    bm.BufferData(Graphics::BufferType::ArrayBuffer, sizeof(vertices), vertices, Graphics::BufferUsage::StaticDraw);

    bm.BindBuffer(Graphics::BufferType::ElementArrayBuffer, m_CubeEBO);
    bm.BufferData(Graphics::BufferType::ElementArrayBuffer, sizeof(indices), indices, Graphics::BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, Graphics::DataType::Float, false, 3 * sizeof(float), (void *)0);

    bm.BindVertexArray(0);
}

void RenderSystem::UpdateOcclusionResults(Scene &scene)
{
    auto &qm = m_Context->GetQueryManager();
    auto view = scene.registry.view<OcclusionComponent>();

    for (auto entity : view)
    {
        auto &occ = view.get<OcclusionComponent>(entity);
        if (occ.queryPending && occ.lastQueryId != 0)
        {
            if (qm.IsResultAvailable(occ.lastQueryId))
            {
                uint32_t samples = qm.GetQueryResult(occ.lastQueryId);
                occ.isVisible = (samples > 0);
                occ.queryPending = false;
            }
        }
    }
}

void RenderSystem::RenderOcclusionQueries(Scene &scene, const glm::mat4 &projection, const glm::mat4 &view, float alpha)
{
    if (!m_OcclusionQueryShader || m_CubeVAO == 0)
        return;

    auto &rsm = m_Context->GetRenderStateManager();
    auto &qm = m_Context->GetQueryManager();
    auto &bm = m_Context->GetBufferManager();

    rsm.ColorMask(false, false, false, false);
    rsm.DepthMask(false);

    m_OcclusionQueryShader->use();
    m_OcclusionQueryShader->setMat4("projection", projection);
    m_OcclusionQueryShader->setMat4("view", view);

    bm.BindVertexArray(m_CubeVAO);

    auto occView = scene.registry.view<TransformComponent, MeshRendererComponent>();
    for (auto entity : occView)
    {
        auto [transform, renderer] = occView.get<TransformComponent, MeshRendererComponent>(entity);
        if (!renderer.model)
            continue;

        if (!scene.registry.all_of<OcclusionComponent>(entity))
        {
            auto &occ = scene.registry.emplace<OcclusionComponent>(entity);
            occ.lastQueryId = qm.GenQuery();
            m_OcclusionQueries.push_back(occ.lastQueryId);
        }

        auto &occ = scene.registry.get<OcclusionComponent>(entity);

        if (occ.queryPending)
            continue;

        glm::mat4 modelMatrix = transform.GetInterpolatedWorldMatrix(scene.registry, alpha);
        AABB aabb = renderer.model->aabb;

        glm::vec3 center = (aabb.minBound + aabb.maxBound) * 0.5f;
        glm::vec3 halfSize = (aabb.maxBound - aabb.minBound) * 0.5f;

        glm::mat4 boxTransform = modelMatrix * glm::translate(glm::mat4(1.0f), center) * glm::scale(glm::mat4(1.0f), halfSize);
        m_OcclusionQueryShader->setMat4("model", boxTransform);

        qm.BeginQuery(Graphics::QueryType::AnySamplesPassed, occ.lastQueryId);
        m_Context->GetDrawContext().DrawElements(Graphics::Primitive::Triangles, 36, Graphics::DataType::UnsignedInt, 0);
        qm.EndQuery(Graphics::QueryType::AnySamplesPassed);

        occ.queryPending = true;
    }

    bm.BindVertexArray(0);

    rsm.ColorMask(true, true, true, true);
    rsm.DepthMask(true);
}
