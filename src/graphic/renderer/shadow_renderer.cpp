#include <graphic/renderer/shadow_renderer.h>
#include <resource/resource_manager.h>
#include <graphic/model.h>
#include <ecs/component.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>

void ShadowRenderer::Init(ResourceManager &res)
{
    m_Shadow.Init();

    res.LoadShader("shadow_depth", "src/asset/shaders/shadow_depth.vs", "src/asset/shaders/shadow_depth.fs");
    res.LoadShader("shadow_point", "src/asset/shaders/shadow_point.vs", "src/asset/shaders/shadow_point.fs", "src/asset/shaders/shadow_point.gs");

    m_Shadow.SetShaderDir(res.GetShader("shadow_depth"));
    m_Shadow.SetShaderPoint(res.GetShader("shadow_point"));
}

void ShadowRenderer::Shutdown()
{
    m_Shadow.Shutdown();
}

void ShadowRenderer::RenderShadows(Scene &scene)
{
    if (m_ShadowMode == 0)
        return;

    if (!m_EnableShadows)
        return;

    Shader *shaderDir = m_Shadow.GetShaderDir();
    Shader *shaderPoint = m_Shadow.GetShaderPoint();

    if (!shaderDir)
        return;

    std::vector<entt::entity> shadowCastingLights;
    auto dirLightView = scene.registry.view<DirectionalLightComponent>();

    for (auto entity : dirLightView)
    {
        auto &light = dirLightView.get<DirectionalLightComponent>(entity);
        if (light.isCastShadow && light.active)
        {
            shadowCastingLights.push_back(entity);
        }
    }

    if (shadowCastingLights.empty())
        return;

    int numShadowsToRender = 1;
    if (m_ShadowMode == 2)
    {
        numShadowsToRender = (std::min)((int)shadowCastingLights.size(), 4);

        if (shadowCastingLights.size() > 4)
        {
            std::cout << "[ShadowRenderer] More than 4 lights have isCastShadow enabled. Only the first 4 will cast shadows." << std::endl;
        }
    }

    glm::vec3 camPos(0.0f);
    entt::entity camEntity = scene.GetActiveCamera();
    if (camEntity != entt::null)
    {
        auto &camTrans = scene.registry.get<TransformComponent>(camEntity);
        camPos = camTrans.position;
    }

    for (int lightIdx = 0; lightIdx < numShadowsToRender; ++lightIdx)
    {
        entt::entity lightEntity = shadowCastingLights[lightIdx];
        auto &light = scene.registry.get<DirectionalLightComponent>(lightEntity);

        glm::vec3 lightDir(0, -1, 0);
        if (scene.registry.all_of<TransformComponent>(lightEntity))
        {
            auto &trans = scene.registry.get<TransformComponent>(lightEntity);
            lightDir = trans.rotation * glm::vec3(0, -1, 0);
        }
        lightDir = glm::normalize(lightDir);
        glm::vec3 lightPos = -lightDir * m_ShadowProjectionSize;

        glm::mat4 lightProjection = glm::ortho(-m_ShadowProjectionSize, m_ShadowProjectionSize,
                                               -m_ShadowProjectionSize, m_ShadowProjectionSize,
                                               0.1f, 200.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        m_LightSpaceMatrixDir[lightIdx] = lightProjection * lightView;

        Frustum lightFrustum;
        if (m_ShadowFrustumCullingEnabled)
        {
            lightFrustum.Update(m_LightSpaceMatrixDir[lightIdx]);
        }

        m_Shadow.BindFBO_Dir(lightIdx);
        glClear(GL_DEPTH_BUFFER_BIT);

        shaderDir->use();
        shaderDir->setMat4("lightSpaceMatrix", m_LightSpaceMatrixDir[lightIdx]);

        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : view)
        {
            auto [trans, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);

            if (!renderer.model || !renderer.castShadow)
                continue;

            glm::mat4 modelMatrix = trans.GetWorldModelMatrix(scene.registry);
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

            if (m_ShadowDistanceCullingSq > 0.0f)
            {
                float dx = (std::max)(worldMin.x - camPos.x, (std::max)(0.0f, camPos.x - worldMax.x));
                float dy = (std::max)(worldMin.y - camPos.y, (std::max)(0.0f, camPos.y - worldMax.y));
                float dz = (std::max)(worldMin.z - camPos.z, (std::max)(0.0f, camPos.z - worldMax.z));
                
                float distSq = dx*dx + dy*dy + dz*dz;
                
                if (distSq > m_ShadowDistanceCullingSq)
                    continue;
            }

            if (m_ShadowFrustumCullingEnabled)
            {
                if (!lightFrustum.IsBoxVisible(worldMin, worldMax))
                    continue;
            }

            shaderDir->setMat4("model", trans.GetWorldModelMatrix(scene.registry));

            if (scene.registry.all_of<AnimationComponent>(entity))
            {
                auto &anim = scene.registry.get<AnimationComponent>(entity);
                if (anim.animator)
                {
                    auto transforms = anim.animator->GetFinalBoneMatrices();
                    for (int j = 0; j < transforms.size() && j < 100; ++j)
                        shaderDir->setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
                    shaderDir->setBool("hasAnimation", true);
                }
                else
                {
                    shaderDir->setBool("hasAnimation", false);
                }
            }
            else
            {
                shaderDir->setBool("hasAnimation", false);
            }

            renderer.model->Draw(*shaderDir);
        }
    }

    m_Shadow.UnbindFBO();

    auto pointView = scene.registry.view<PointLightComponent, TransformComponent>();

    if (!shaderPoint)
        return;

    std::vector<entt::entity> shadowCastingPointLights;
    for (auto entity : pointView)
    {
        auto &light = pointView.get<PointLightComponent>(entity);
        if (light.isCastShadow && light.active)
        {
            shadowCastingPointLights.push_back(entity);
            if (shadowCastingPointLights.size() >= 4)
                break;
        }
    }

    if (shadowCastingPointLights.empty())
        return;

    int pIdx = 0;
    shaderPoint->use();
    for (auto entity : shadowCastingPointLights)
    {
        if (pIdx >= Shadow::MAX_POINT_LIGHTS_SHADOW)
            break;

        auto [light, trans] = pointView.get<PointLightComponent, TransformComponent>(entity);
        glm::vec3 lightPos = trans.position;

        float aspect = (float)m_Shadow.GetShadowPointWidth() / (float)m_Shadow.GetShadowPointHeight();
        float nearP = 1.0f;
        float farP = m_FarPlanePoint;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, nearP, farP);

        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0)));

        for (int i = 0; i < 6; ++i)
            shaderPoint->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);

        shaderPoint->setFloat("farPlane", farP);
        shaderPoint->setVec3("lightPos", lightPos);

        m_Shadow.BindFBO_Point(pIdx);
        glClear(GL_DEPTH_BUFFER_BIT);

        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto obj : view)
        {
            auto [tObj, rObj] = view.get<TransformComponent, MeshRendererComponent>(obj);
            if (rObj.model)
            {
                shaderPoint->setMat4("model", tObj.GetWorldModelMatrix(scene.registry));
                if (scene.registry.all_of<AnimationComponent>(obj))
                {
                    auto &anim = scene.registry.get<AnimationComponent>(obj);
                    if (anim.animator)
                    {
                        auto transforms = anim.animator->GetFinalBoneMatrices();
                        for (int j = 0; j < transforms.size() && j < 100; ++j)
                            shaderPoint->setMat4("finalBonesMatrices[" + std::to_string(j) + "]", transforms[j]);
                        shaderPoint->setBool("hasAnimation", true);
                    }
                    else
                    {
                        shaderPoint->setBool("hasAnimation", false);
                    }
                }
                else
                {
                    shaderPoint->setBool("hasAnimation", false);
                }
                rObj.model->Draw(*shaderPoint);
            }
        }

        pIdx++;
    }

    m_Shadow.UnbindFBO();
}
