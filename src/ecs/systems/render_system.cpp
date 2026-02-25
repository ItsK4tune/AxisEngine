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

#ifdef ENABLE_DEBUG_SYSTEM
#include <debug/debug_config.h>
#endif

void RenderSystem::Init(IGraphicsContext& context, ResourceManager &res)
{
    m_Context = &context;

    LOGGER_INFO("RenderSystem") << "Initializing shadow and light renderers";
    m_ShadowRenderer.Init(res);
    m_LightRenderer.Init(m_Context->GetBufferManager());

    if (m_WhiteTextureID == 0)
    {
        auto& tm = m_Context->GetTextureManager();
        m_WhiteTextureID = tm.GenTexture();
        tm.BindTexture(Graphics::TextureType::Texture2D, m_WhiteTextureID);
        unsigned char white[] = {255, 255, 255, 255};

        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA8, 1, 1, 0,
                      Graphics::TextureFormat::RGBA, Graphics::DataType::UnsignedByte, white);

        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
    }

    m_BonesUniforms.reserve(100);
    for (int i = 0; i < 100; ++i)
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
}

void RenderSystem::Shutdown()
{
    LOGGER_INFO("RenderSystem") << "Shutting down RenderSystem";
    m_ShadowRenderer.Shutdown();
}

void RenderSystem::RenderShadows(Scene &scene)
{
    m_ShadowRenderer.RenderShadows(scene);
}

void RenderSystem::SetFaceCulling(bool enabled, Graphics::CullMode mode)
{
    if (!m_Context) return;
    auto& rsm = m_Context->GetRenderStateManager();

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
    if (!m_Context) return;
    auto& rsm = m_Context->GetRenderStateManager();

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

void RenderSystem::Render(Scene &scene, int width, int height)
{
    if (!m_Enabled || !m_Context)
        return;

    entt::entity camEntity = scene.GetActiveCamera();
    CameraComponent *cam = nullptr;
    TransformComponent *camTrans = nullptr;

    if (camEntity == entt::null)
        return;

    cam = &scene.registry.get<CameraComponent>(camEntity);
    camTrans = &scene.registry.get<TransformComponent>(camEntity);

    glm::mat4 projectionMatrix = cam->projectionMatrix;

    if (m_AAMode == AntiAliasingMode::TAA && cam)
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

        projectionMatrix = jitterMatrix * projectionMatrix;

        m_FrameIndex++;
    }
    else
    {
        m_JitterOffset = glm::vec2(0.0f);
    }

    m_PrevViewProj = m_CurrViewProj;
    m_CurrViewProj = projectionMatrix * cam->viewMatrix;

    if (m_PrevViewProj[3][3] == 0.0f)
        m_PrevViewProj = m_CurrViewProj;

    Frustum frustum;
    if (cam)
        frustum.Update(m_CurrViewProj);

    m_RenderQueue.clear();

    std::vector<entt::entity> visibleEntities;
    if (scene.GetOctree())
    {
        // For now, rebuild every frame to ensure correctness with dynamic objects.
        // In a production engine, you'd only update moved objects.
        std::vector<OctreeElement> elements;
        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : view)
        {
            auto [transform, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);
            if (!renderer.model) continue;

            glm::mat4 modelMatrix = transform.GetWorldModelMatrix(scene.registry);
            elements.push_back({entity, renderer.model->aabb.Transform(modelMatrix)});
        }
        scene.GetOctree()->Rebuild(elements);
        
        if (m_FrustumCullingEnabled)
            scene.GetOctree()->Query(frustum, visibleEntities);
        else
        {
            for (const auto& el : elements) visibleEntities.push_back(el.entity);
        }
    }
    else
    {
        // Fallback if no octree
        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : view) visibleEntities.push_back(entity);
    }

    for (auto entity : visibleEntities)
    {
        if (!scene.registry.all_of<TransformComponent, MeshRendererComponent>(entity))
            continue;

        auto &transform = scene.registry.get<TransformComponent>(entity);
        auto &renderer = scene.registry.get<MeshRendererComponent>(entity);

        if (!renderer.model || renderer.shader.expired())
            continue;

        glm::mat4 modelMatrix = transform.GetWorldModelMatrix(scene.registry);
        
        // Distance Culling (still useful with Octree if needed, though Octree can handle it)
        float distSq = 0.0f;
        if (cam && camTrans)
        {
            glm::vec3 cameraPos = camTrans->position;
            glm::vec3 worldMin = modelMatrix * glm::vec4(renderer.model->aabb.minBound, 1.0f);
            glm::vec3 worldMax = modelMatrix * glm::vec4(renderer.model->aabb.maxBound, 1.0f);
            
            float dx = (std::max)(worldMin.x - cameraPos.x, (std::max)(0.0f, cameraPos.x - worldMax.x));
            float dy = (std::max)(worldMin.y - cameraPos.y, (std::max)(0.0f, cameraPos.y - worldMax.y));
            float dz = (std::max)(worldMin.z - cameraPos.z, (std::max)(0.0f, cameraPos.z - worldMax.z));

            distSq = dx*dx + dy*dy + dz*dz;

            if (m_DistanceCullingSq > 0.0f && distSq > m_DistanceCullingSq)
                continue;
        }

        MaterialComponent *mat = scene.registry.try_get<MaterialComponent>(entity);
        Model *activeModel = renderer.model.get();
        
        if (auto* lod = scene.registry.try_get<LODComponent>(entity))
        {
            for (int i = 0; i < lod->lodDistancesSq.size(); ++i) {
                if (distSq > lod->lodDistancesSq[i] && i < lod->lodModels.size() && lod->lodModels[i]) {
                    activeModel = lod->lodModels[i].get();
                } else {
                    break;
                }
            }
        }

        uint32_t layer = 1;
        int renderOrder = renderer.order;

        if (auto* info = scene.registry.try_get<InfoComponent>(entity))
        {
            layer = info->layer;
        }

        if ((m_FilterLayerMask & layer) == 0) continue;
        if (cam && (cam->cullingMask & layer) == 0) continue;

        m_RenderQueue.emplace_back(RenderItem{entity, &transform, &renderer, mat, activeModel, layer, renderOrder});
    }

    std::sort(m_RenderQueue.begin(), m_RenderQueue.end(), [this](const RenderItem &lhs, const RenderItem &rhs)
    {
        if (m_RenderOrderEnabled)
        {
            if (lhs.layer != rhs.layer)
                return lhs.layer < rhs.layer;
            if (lhs.renderOrder != rhs.renderOrder)
                return lhs.renderOrder < rhs.renderOrder;
        }

        auto lShader = lhs.renderer->shader.lock();
        auto rShader = rhs.renderer->shader.lock();
        unsigned int lID = lShader ? lShader->getID() : 0;
        unsigned int rID = rShader ? rShader->getID() : 0;
        if (lID != rID)
            return lID < rID;
        if (lhs.material != rhs.material)
            return lhs.material < rhs.material;
        return lhs.activeModel < rhs.activeModel; 
    });

    Shader *currentShader = nullptr;
    Model *currentModel = nullptr;
    MaterialComponent *currentMaterial = nullptr;
    std::vector<glm::mat4> instanceBatch;
    m_RenderedCount = 0;

    auto& rsm = m_Context->GetRenderStateManager();
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
        TransformComponent &transform = *item.transform;
        MeshRendererComponent &renderer = *item.renderer;
        MaterialComponent *material = item.material;

        auto lockedShader = renderer.shader.lock();
        if (!lockedShader)
            continue;
        Shader* itemShader = lockedShader.get();

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

                   const glm::mat4* lightSpaceMatrices = m_ShadowRenderer.GetLightSpaceMatrices();
                   for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i)
                   {
                        currentShader->setMat4(m_LightSpaceMatrixUniforms[i], lightSpaceMatrices[i]);
                   }

                   const glm::mat4* lightSpaceMatricesSpot = m_ShadowRenderer.GetLightSpaceMatricesSpot();
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
            m_LightRenderer.UploadLightData(scene, currentShader);
        }

        bool isAnimated = scene.registry.all_of<AnimationComponent>(entity) && scene.registry.get<AnimationComponent>(entity).animator;

        if (isAnimated)
        {
            flushBatch(currentShader, currentModel);
            currentModel = nullptr;
            currentMaterial = nullptr;

            glm::mat4 modelMatrix = transform.GetWorldModelMatrix(scene.registry);
            currentShader->setMat4("model", modelMatrix);
            currentShader->setVec4("tintColor", renderer.color);

            auto &anim = scene.registry.get<AnimationComponent>(entity);
            auto transforms = anim.animator->GetFinalBoneMatrices();
            for (int j = 0; j < transforms.size() && j < 100; ++j)
                currentShader->setMat4(m_BonesUniforms[j], transforms[j]);

            SetupMaterialUniforms(currentShader, entity, scene);

            item.activeModel->Draw(*currentShader);
            m_RenderedCount++;
        }
        else
        {
            if (m_DebugNoTexture)
                currentShader->setBool("debug_noTexture", true);
            else
                currentShader->setBool("debug_noTexture", false);

            if (!m_InstanceBatchingEnabled)
            {
                currentShader->setMat4("model", transform.GetWorldModelMatrix(scene.registry));
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

                instanceBatch.push_back(transform.GetWorldModelMatrix(scene.registry));
            }
        }
    }
    flushBatch(currentShader, currentModel);

#ifdef ENABLE_DEBUG_SYSTEM
    if (DebugConfig::ShowWireframe)
    {
        rsm.PolygonMode(Graphics::CullMode::FrontAndBack, prevMode);
    }
#endif
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

    if (m_DebugNoTexture)
        shader->setBool("debug_noTexture", true);
    else
        shader->setBool("debug_noTexture", false);
}
