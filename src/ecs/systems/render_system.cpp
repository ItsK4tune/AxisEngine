#include <ecs/systems/render_system.h>
#include <graphic/renderer/frustum.h>
#include <string>
#include <algorithm>
#include <vector>
#include <utils/logger.h>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>
#include <resource/resource_manager.h>

void RenderSystem::InitShadows(ResourceManager &res)
{
    LOGGER_INFO("RenderSystem") << "Initializing shadow and light renderers";
    m_ShadowRenderer.Init(res);
    m_LightRenderer.Init();
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

void RenderSystem::SetFaceCulling(bool enabled, int mode)
{
    if (enabled)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(mode);
    }
    else
    {
        glDisable(GL_CULL_FACE);
    }
}

void RenderSystem::SetDepthTest(bool enabled, int func)
{
    if (enabled)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(func);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void RenderSystem::Render(Scene &scene, int width, int height)
{
    if (!m_Enabled)
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

    auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
    for (auto entity : view)
    {
        auto [transform, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);

        if (!renderer.model || !renderer.shader)
            continue;

        glm::mat4 modelMatrix = transform.GetWorldModelMatrix(scene.registry);
        glm::vec3 min = renderer.model->AABBmin;
        glm::vec3 max = renderer.model->AABBmax;

        glm::vec3 center = (min + max) * 0.5f;
        glm::vec3 extent = (max - min) * 0.5f;

        glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(center, 1.0f));

        glm::mat3 rot = glm::mat3(modelMatrix);
        glm::vec3 worldExtent = glm::vec3(
            std::abs(rot[0][0]) * extent.x + std::abs(rot[1][0]) * extent.y + std::abs(rot[2][0]) * extent.z,
            std::abs(rot[0][1]) * extent.x + std::abs(rot[1][1]) * extent.y + std::abs(rot[2][1]) * extent.z,
            std::abs(rot[0][2]) * extent.x + std::abs(rot[1][2]) * extent.y + std::abs(rot[2][2]) * extent.z);

        glm::vec3 worldMin = worldCenter - worldExtent;
        glm::vec3 worldMax = worldCenter + worldExtent;

        if (m_DistanceCullingSq > 0.0f && cam && camTrans)
        {
            glm::vec3 cameraPos = camTrans->position;
            float dx = (std::max)(worldMin.x - cameraPos.x, (std::max)(0.0f, cameraPos.x - worldMax.x));
            float dy = (std::max)(worldMin.y - cameraPos.y, (std::max)(0.0f, cameraPos.y - worldMax.y));
            float dz = (std::max)(worldMin.z - cameraPos.z, (std::max)(0.0f, cameraPos.z - worldMax.z));
            
            float distSq = dx*dx + dy*dy + dz*dz;
            
            if (distSq > m_DistanceCullingSq)
                continue;
        }

        if (cam && m_FrustumCullingEnabled)
        {
            if (!frustum.IsBoxVisible(worldMin, worldMax))
                continue;
        }

        m_RenderQueue.emplace_back(RenderItem{entity, &transform, &renderer});
    }

    std::sort(m_RenderQueue.begin(), m_RenderQueue.end(), [](const RenderItem &lhs, const RenderItem &rhs)
              {
        if (lhs.renderer->shader != rhs.renderer->shader)
            return lhs.renderer->shader < rhs.renderer->shader;
        return lhs.renderer->model < rhs.renderer->model; });

    static std::vector<std::string> bonesUniforms;
    if (bonesUniforms.empty())
    {
        for (int i = 0; i < 100; ++i)
            bonesUniforms.push_back("finalBonesMatrices[" + std::to_string(i) + "]");
    }
    static std::vector<std::string> shadowPointUniforms;
    if (shadowPointUniforms.empty())
    {
        for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i)
            shadowPointUniforms.push_back("shadowMapPoint[" + std::to_string(i) + "]");
    }

    Shader *currentShader = nullptr;
    Model *currentModel = nullptr;
    std::vector<glm::mat4> instanceBatch;
    m_RenderedCount = 0;

    auto flushBatch = [&](Shader *shader, Model *model)
    {
        if (!instanceBatch.empty() && shader && model)
        {
            model->DrawInstanced(*shader, instanceBatch);
            m_RenderedCount += instanceBatch.size();
            instanceBatch.clear();
        }
    };

    for (const auto &item : m_RenderQueue)
    {
        entt::entity entity = item.entity;
        TransformComponent &transform = *item.transform;
        MeshRendererComponent &renderer = *item.renderer;

        if (currentShader != renderer.shader)
        {
            flushBatch(currentShader, currentModel);
            currentShader = renderer.shader;
            currentModel = nullptr;
            currentShader->use();

            if (cam && camTrans)
            {
                currentShader->setMat4("projection", projectionMatrix);
                currentShader->setMat4("view", cam->viewMatrix);
                currentShader->setVec3("viewPos", camTrans->position);

                // Pass shadow uniforms
                if (m_ShadowRenderer.IsShadowsEnabled() && m_ShadowRenderer.GetShadowMode() > 0)
                {
                   currentShader->setBool("u_ReceiveShadow", true); // Or use individual flags if needed

                   // Bind shadow maps
                   for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i)
                   {
                       m_ShadowRenderer.GetShadow().BindTexture_Dir(i, 10 + i);
                       std::string uniformName = "shadowMapDir[" + std::to_string(i) + "]";
                       currentShader->setInt(uniformName, 10 + i);
                   }

                   for (int i = 0; i < Shadow::MAX_POINT_LIGHTS_SHADOW; ++i)
                   {
                       m_ShadowRenderer.GetShadow().BindTexture_Point(i, 12 + i);
                       currentShader->setInt(shadowPointUniforms[i], 12 + i);
                   }

                   for (int i = 0; i < Shadow::MAX_SPOT_LIGHTS_SHADOW; ++i)
                   {
                       m_ShadowRenderer.GetShadow().BindTexture_Spot(i, 14 + i);
                       std::string uniformName = "shadowMapSpot[" + std::to_string(i) + "]";
                       currentShader->setInt(uniformName, 14 + i);
                   }

                   const glm::mat4* lightSpaceMatrices = m_ShadowRenderer.GetLightSpaceMatrices();
                   for (int i = 0; i < Shadow::MAX_DIR_LIGHTS_SHADOW; ++i)
                   {
                        std::string uniformName = "lightSpaceMatrix[" + std::to_string(i) + "]";
                        currentShader->setMat4(uniformName, lightSpaceMatrices[i]);
                   }

                   const glm::mat4* lightSpaceMatricesSpot = m_ShadowRenderer.GetLightSpaceMatricesSpot();
                   for (int i = 0; i < Shadow::MAX_SPOT_LIGHTS_SHADOW; ++i)
                   {
                        std::string uniformName = "lightSpaceMatrixSpot[" + std::to_string(i) + "]";
                        currentShader->setMat4(uniformName, lightSpaceMatricesSpot[i]);
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

            glm::mat4 modelMatrix = transform.GetWorldModelMatrix(scene.registry);
            currentShader->setMat4("model", modelMatrix);
            currentShader->setVec4("tintColor", renderer.color);

            auto &anim = scene.registry.get<AnimationComponent>(entity);
            auto transforms = anim.animator->GetFinalBoneMatrices();
            for (int j = 0; j < transforms.size() && j < 100; ++j)
                currentShader->setMat4(bonesUniforms[j], transforms[j]);

            if (scene.registry.all_of<MaterialComponent>(entity))
            {
                auto &mat = scene.registry.get<MaterialComponent>(entity);
                if (mat.type == MaterialType::PBR)
                {
                    currentShader->setFloat("material.roughness", mat.roughness);
                    currentShader->setFloat("material.metallic", mat.metallic);
                    currentShader->setFloat("material.ao", mat.ao);
                    currentShader->setVec3("material.emission", mat.emission);
                }
                else
                {
                    currentShader->setFloat("material.shininess", mat.shininess);
                    currentShader->setVec3("material.specular", mat.specular);
                    currentShader->setVec3("material.ambient", mat.ambient);
                    currentShader->setVec3("material.emission", mat.emission);
                }
                currentShader->setFloat("material.opacity", mat.opacity);
                currentShader->setVec2("uvScale", mat.uvScale);
                currentShader->setVec2("uvOffset", mat.uvOffset);

                if (m_DebugNoTexture)
                {
                    if (m_WhiteTextureID == 0)
                    {
                        glGenTextures(1, &m_WhiteTextureID);
                        glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                        unsigned char white[] = {255, 255, 255, 255};
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                    }
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                }
            }
            else
            {
                currentShader->setFloat("material.shininess", 32.0f);
                currentShader->setVec3("material.specular", glm::vec3(0.5f));
                currentShader->setVec3("material.ambient", glm::vec3(1.0f));
                currentShader->setVec3("material.emission", glm::vec3(0.0f));
                currentShader->setFloat("material.opacity", 1.0f);
                currentShader->setVec2("uvScale", glm::vec2(1.0f));
                currentShader->setVec2("uvOffset", glm::vec2(0.0f));
            }

            if (m_DebugNoTexture)
                currentShader->setBool("debug_noTexture", true);
            else
                currentShader->setBool("debug_noTexture", false);

            renderer.model->Draw(*currentShader);
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

                if (scene.registry.all_of<MaterialComponent>(entity))
                {
                    auto &mat = scene.registry.get<MaterialComponent>(entity);
                    if (mat.type == MaterialType::PBR)
                    {
                        currentShader->setFloat("material.roughness", mat.roughness);
                        currentShader->setFloat("material.metallic", mat.metallic);
                        currentShader->setFloat("material.ao", mat.ao);
                        currentShader->setVec3("material.emission", mat.emission);
                    }
                    else
                    {
                        currentShader->setFloat("material.shininess", mat.shininess);
                        currentShader->setVec3("material.specular", mat.specular);
                        currentShader->setVec3("material.ambient", mat.ambient);
                        currentShader->setVec3("material.emission", mat.emission);
                    }
                    currentShader->setFloat("material.opacity", mat.opacity);
                    currentShader->setVec2("uvScale", mat.uvScale);
                    currentShader->setVec2("uvOffset", mat.uvOffset);

                    if (m_DebugNoTexture)
                    {
                        if (m_WhiteTextureID == 0)
                        {
                            glGenTextures(1, &m_WhiteTextureID);
                            glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                            unsigned char white[] = {255, 255, 255, 255};
                            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                        }
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                    }
                }
                else
                {
                    currentShader->setFloat("material.shininess", 32.0f);
                    currentShader->setVec3("material.specular", glm::vec3(0.5f));
                    currentShader->setVec3("material.ambient", glm::vec3(1.0f));
                    currentShader->setVec3("material.emission", glm::vec3(0.0f));
                    currentShader->setFloat("material.opacity", 1.0f);
                    currentShader->setVec2("uvScale", glm::vec2(1.0f));
                    currentShader->setVec2("uvOffset", glm::vec2(0.0f));
                }

                if (m_DebugNoTexture)
                    currentShader->setBool("debug_noTexture", true);
                else
                    currentShader->setBool("debug_noTexture", false);

                renderer.model->Draw(*currentShader);
                m_RenderedCount++;
            }
            else
            {
                if (currentModel != renderer.model)
                {
                    flushBatch(currentShader, currentModel);
                    currentModel = renderer.model;

                    currentShader->setVec4("tintColor", renderer.color);

                    if (scene.registry.all_of<MaterialComponent>(entity))
                    {
                        auto &mat = scene.registry.get<MaterialComponent>(entity);
                        if (mat.type == MaterialType::PBR)
                        {
                            currentShader->setFloat("material.roughness", mat.roughness);
                            currentShader->setFloat("material.metallic", mat.metallic);
                            currentShader->setFloat("material.ao", mat.ao);
                            currentShader->setVec3("material.emission", mat.emission);
                        }
                        else
                        {
                            currentShader->setFloat("material.shininess", mat.shininess);
                            currentShader->setVec3("material.specular", mat.specular);
                            currentShader->setVec3("material.ambient", mat.ambient);
                            currentShader->setVec3("material.emission", mat.emission);
                        }
                        currentShader->setFloat("material.opacity", mat.opacity);
                        currentShader->setVec2("uvScale", mat.uvScale);
                        currentShader->setVec2("uvOffset", mat.uvOffset);

                        if (m_DebugNoTexture)
                        {
                            if (m_WhiteTextureID == 0)
                            {
                                glGenTextures(1, &m_WhiteTextureID);
                                glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                                unsigned char white[] = {255, 255, 255, 255};
                                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                            }
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, m_WhiteTextureID);
                        }
                    }
                    else
                    {
                        currentShader->setFloat("material.shininess", 32.0f);
                        currentShader->setVec3("material.specular", glm::vec3(0.5f));
                        currentShader->setVec3("material.ambient", glm::vec3(1.0f));
                        currentShader->setVec3("material.emission", glm::vec3(0.0f));
                        currentShader->setFloat("material.opacity", 1.0f);
                        currentShader->setVec2("uvScale", glm::vec2(1.0f));
                        currentShader->setVec2("uvOffset", glm::vec2(0.0f));
                    }

                    if (m_DebugNoTexture)
                        currentShader->setBool("debug_noTexture", true);
                    else
                        currentShader->setBool("debug_noTexture", false);
                }

                instanceBatch.push_back(transform.GetWorldModelMatrix(scene.registry));
            }
        }
    }
    flushBatch(currentShader, currentModel);
}
