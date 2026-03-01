#include <utils/logger.h>
#include <graphic/renderer/shadow_renderer.h>
#include <interface/graphic/i_draw_context.h>
#include <resource/resource_manager.h>
#include <graphic/geometry/model.h>
#include <ecs/component.h>
#include <glm/gtc/matrix_transform.hpp>
#include <ecs/entity_manager.h>
#include <iostream>
#include <algorithm>
#include <ecs/systems/render_system.h>

void ShadowRenderer::Init(ResourceManager &res)
{
    m_Shadow.Init();

    res.LoadShader("shadow_depth", "includes/engine/asset/shaders/shadow_depth.vs", "includes/engine/asset/shaders/shadow_depth.fs");
    res.LoadShader("shadow_point", "includes/engine/asset/shaders/shadow_point.vs", "includes/engine/asset/shaders/shadow_point.fs", "includes/engine/asset/shaders/shadow_point.gs");
    res.LoadShader("shadow_spot", "includes/engine/asset/shaders/shadow_spot.vs", "includes/engine/asset/shaders/shadow_spot.fs");

    m_Shadow.SetShaderDir(res.GetShader("shadow_depth").get());
    m_Shadow.SetShaderPoint(res.GetShader("shadow_point").get());
    m_Shadow.SetShaderSpot(res.GetShader("shadow_spot").get());
}

void ShadowRenderer::Shutdown()
{
    m_Shadow.Shutdown();
}

void ShadowRenderer::RenderShadows(Scene &scene, const std::vector<RenderItem>& shadowQueue)
{
    static int shadowRenderCount = 0;
    shadowRenderCount++;
    bool logThisRender = (shadowRenderCount <= 5);

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
        numShadowsToRender = (std::min)((int)shadowCastingLights.size(), Shadow::MAX_DIR_LIGHTS_SHADOW);

        if (shadowCastingLights.size() > Shadow::MAX_DIR_LIGHTS_SHADOW)
        {
            if (!m_DirLightLimitWarned)
            {
                LOGGER_WARN("ShadowRenderer") << "More than " << Shadow::MAX_DIR_LIGHTS_SHADOW << " lights have isCastShadow enabled. Only the first " << Shadow::MAX_DIR_LIGHTS_SHADOW << " will cast shadows.";
                m_DirLightLimitWarned = true;
            }
        }
    }

    glm::vec3 camPos(0.0f);
    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
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
        m_Shadow.GetDrawContext().Clear(Graphics::BufferBit::Depth);

        shaderDir->use();
        shaderDir->setMat4("lightSpaceMatrix", m_LightSpaceMatrixDir[lightIdx]);

        for (const auto& item : shadowQueue)
        {
            if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity))
                continue;

            entt::entity entity = item.entity;
            auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
            
            AABB worldAABB = renderer.model->aabb.Transform(item.worldMatrix);
            glm::vec3 worldMin = worldAABB.minBound;
            glm::vec3 worldMax = worldAABB.maxBound;

            float distSq = 0.0f;
            float dx = (std::max)(worldMin.x - camPos.x, (std::max)(0.0f, camPos.x - worldMax.x));
            float dy = (std::max)(worldMin.y - camPos.y, (std::max)(0.0f, camPos.y - worldMax.y));
            float dz = (std::max)(worldMin.z - camPos.z, (std::max)(0.0f, camPos.z - worldMax.z));

            distSq = dx*dx + dy*dy + dz*dz;

            if (m_ShadowDistanceCullingSq > 0.0f && distSq > m_ShadowDistanceCullingSq)
            {
                continue;
            }

            if (m_ShadowFrustumCullingEnabled)
            {
                if (!lightFrustum.IsBoxVisible(worldMin, worldMax))
                    continue;
            }

            Model* activeModel = item.activeModel;

            shaderDir->setMat4("model", item.worldMatrix);

            if (scene.registry.all_of<AnimationComponent>(entity))
            {
                auto &anim = scene.registry.get<AnimationComponent>(entity);
                if (anim.animator)
                {
                    auto transforms = anim.animator->GetFinalBoneMatrices();
                    shaderDir->setMat4Array("finalBonesMatrices", transforms);
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

            activeModel->Draw(*shaderDir);
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
            if (shadowCastingPointLights.size() >= Shadow::MAX_POINT_LIGHTS_SHADOW)
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
        m_Shadow.GetDrawContext().Clear(Graphics::BufferBit::Depth);

        for (const auto& item : shadowQueue)
        {
            if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity))
                continue;

            entt::entity obj = item.entity;
            auto& rObj = scene.registry.get<MeshRendererComponent>(obj);

            AABB worldAABB = rObj.model->aabb.Transform(item.worldMatrix);
            glm::vec3 worldMin = worldAABB.minBound;
            glm::vec3 worldMax = worldAABB.maxBound;

            float distSq = 0.0f;
            float dx = (std::max)(worldMin.x - camPos.x, (std::max)(0.0f, camPos.x - worldMax.x));
            float dy = (std::max)(worldMin.y - camPos.y, (std::max)(0.0f, camPos.y - worldMax.y));
            float dz = (std::max)(worldMin.z - camPos.z, (std::max)(0.0f, camPos.z - worldMax.z));
            distSq = dx*dx + dy*dy + dz*dz;

            if (m_ShadowDistanceCullingSq > 0.0f && distSq > m_ShadowDistanceCullingSq)
                continue;

            Model* activeModel = item.activeModel;

            shaderPoint->setMat4("model", item.worldMatrix);
            if (scene.registry.all_of<AnimationComponent>(obj))
            {
                auto &anim = scene.registry.get<AnimationComponent>(obj);
                if (anim.animator)
                {
                    auto transforms = anim.animator->GetFinalBoneMatrices();
                    shaderPoint->setMat4Array("finalBonesMatrices", transforms);
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
            activeModel->Draw(*shaderPoint);
        }

        pIdx++;
    }

    m_Shadow.UnbindFBO();

    Shader *shaderSpot = m_Shadow.GetShaderSpot();
    if (!shaderSpot)
        return;

    auto spotView = scene.registry.view<SpotLightComponent, TransformComponent>();
    std::vector<entt::entity> shadowCastingSpotLights;

    for (auto entity : spotView)
    {
        auto &light = spotView.get<SpotLightComponent>(entity);
        if (light.isCastShadow && light.active)
        {
            shadowCastingSpotLights.push_back(entity);
            if (shadowCastingSpotLights.size() >= Shadow::MAX_SPOT_LIGHTS_SHADOW)
                break;
        }
    }

    if (shadowCastingSpotLights.empty())
        return;

    shaderSpot->use();
    int sIdx = 0;

    for (auto entity : shadowCastingSpotLights)
    {
        if (sIdx >= Shadow::MAX_SPOT_LIGHTS_SHADOW)
            break;

        auto [light, trans] = spotView.get<SpotLightComponent, TransformComponent>(entity);
        glm::vec3 lightPos = trans.position;
        glm::vec3 lightDir = trans.rotation * glm::vec3(0, -1, 0);
        lightDir = glm::normalize(lightDir);

        float coneAngle = glm::acos(light.cutOff);
        float fov = coneAngle * 2.0f;
        float aspect = 1.0f;
        float nearPlane = 0.1f;
        float farPlane = m_FarPlaneSpot;

        glm::mat4 spotProjection = glm::perspective(fov, aspect, nearPlane, farPlane);

        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(lightDir, up)) > 0.99f)
            up = glm::vec3(0.0f, 0.0f, 1.0f);

        glm::mat4 spotView = glm::lookAt(lightPos, lightPos + lightDir, up);
        m_LightSpaceMatrixSpot[sIdx] = spotProjection * spotView;

        Frustum lightFrustum;
        if (m_ShadowFrustumCullingEnabled)
        {
            lightFrustum.Update(m_LightSpaceMatrixSpot[sIdx]);
        }

        m_Shadow.BindFBO_Spot(sIdx);
        m_Shadow.GetDrawContext().Clear(Graphics::BufferBit::Depth);

        shaderSpot->setMat4("lightSpaceMatrix", m_LightSpaceMatrixSpot[sIdx]);

        for (const auto& item : shadowQueue)
        {
            if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity))
                continue;

            entt::entity obj = item.entity;
            auto& rObj = scene.registry.get<MeshRendererComponent>(obj);

            AABB worldAABB = rObj.model->aabb.Transform(item.worldMatrix);
            glm::vec3 worldMin = worldAABB.minBound;
            glm::vec3 worldMax = worldAABB.maxBound;

            float distSq = 0.0f;
            float dx = (std::max)(worldMin.x - camPos.x, (std::max)(0.0f, camPos.x - worldMax.x));
            float dy = (std::max)(worldMin.y - camPos.y, (std::max)(0.0f, camPos.y - worldMax.y));
            float dz = (std::max)(worldMin.z - camPos.z, (std::max)(0.0f, camPos.z - worldMax.z));

            distSq = dx*dx + dy*dy + dz*dz;

            if (m_ShadowDistanceCullingSq > 0.0f && distSq > m_ShadowDistanceCullingSq)
            {
                continue;
            }

            if (m_ShadowFrustumCullingEnabled)
            {
                if (!lightFrustum.IsBoxVisible(worldMin, worldMax))
                    continue;
            }

            Model* activeModel = item.activeModel;

            shaderSpot->setMat4("model", item.worldMatrix);

            if (scene.registry.all_of<AnimationComponent>(obj))
            {
                auto &anim = scene.registry.get<AnimationComponent>(obj);
                if (anim.animator)
                {
                    auto transforms = anim.animator->GetFinalBoneMatrices();
                    shaderSpot->setMat4Array("finalBonesMatrices", transforms);
                    shaderSpot->setBool("hasAnimation", true);
                }
                else
                {
                    shaderSpot->setBool("hasAnimation", false);
                }
            }
            else
            {
                shaderSpot->setBool("hasAnimation", false);
            }

            activeModel->Draw(*shaderSpot);
        }

        sIdx++;
    }

    m_Shadow.UnbindFBO();
}
