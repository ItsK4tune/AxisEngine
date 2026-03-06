#include <algorithm>
#include <ecs/component.h>
#include <ecs/entity_manager.h>
#include <ecs/systems/render_system.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <rendering/geometry/model.h>
#include <rendering/renderer/shadow_renderer.h>
#include <core/job_system.h>
#include <rendering/interfaces/i_draw_context.h>
#include <iostream>
#include <resource/resource_manager.h>
#include <core/utils/logger.h>

void ShadowRenderer::Init(IGraphicsContext& context, IShaderLibrary &shaderLib)
{
    m_Shadow.Init(context);

    shaderLib.LoadShader("shadow_depth", "includes/engine/asset/shaders/shadow_depth.vs", "includes/engine/asset/shaders/shadow_depth.fs");
    shaderLib.LoadShader("shadow_point", "includes/engine/asset/shaders/shadow_point.vs", "includes/engine/asset/shaders/shadow_point.fs", "includes/engine/asset/shaders/shadow_point.gs");
    shaderLib.LoadShader("shadow_spot", "includes/engine/asset/shaders/shadow_spot.vs", "includes/engine/asset/shaders/shadow_spot.fs");

    m_Shadow.SetShaderDir(shaderLib.GetShader("shadow_depth").get());
    m_Shadow.SetShaderPoint(shaderLib.GetShader("shadow_point").get());
    m_Shadow.SetShaderSpot(shaderLib.GetShader("shadow_spot").get());
}

void ShadowRenderer::Shutdown()
{
    m_Shadow.Shutdown();
}

void ShadowRenderer::RenderShadows(Scene &scene, const std::vector<RenderItem>& shadowQueue)
{
    if (m_ShadowMode == 0 || !m_EnableShadows) return;

    Shader *shaderDir = m_Shadow.GetShaderDir();
    Shader *shaderPoint = m_Shadow.GetShaderPoint();
    Shader *shaderSpot = m_Shadow.GetShaderSpot();

    if (!shaderDir) return;

    auto dirLightView = scene.registry.view<DirectionalLightComponent>();
    std::vector<entt::entity> shadowCastingLights;
    for (auto entity : dirLightView) {
        auto &light = dirLightView.get<DirectionalLightComponent>(entity);
        if (light.isCastShadow && light.active) shadowCastingLights.push_back(entity);
    }

    if (shadowCastingLights.empty() && 
        scene.registry.view<PointLightComponent>().empty() && 
        scene.registry.view<SpotLightComponent>().empty()) return;

    glm::vec3 camPos(0.0f);
    entt::entity camEntity = EntityManager::GetActiveCamera(scene);
    if (camEntity != entt::null) {
        if (auto* p = scene.registry.try_get<PositionComponent>(camEntity)) camPos = p->value;
    }

    CommandQueue shadowQueueMain;
    JobSystem::JobCounter counter(0);

    // 1. Directional Shadows
    int numDirShadows = (m_ShadowMode == 2) ? (std::min)((int)shadowCastingLights.size(), Shadow::MAX_DIR_LIGHTS_SHADOW) : (shadowCastingLights.empty() ? 0 : 1);
    
    for (int lightIdx = 0; lightIdx < numDirShadows; ++lightIdx) {
        entt::entity lightEntity = shadowCastingLights[lightIdx];
        glm::vec3 lightDir(0, -1, 0);
        if (scene.registry.all_of<RotationComponent>(lightEntity)) {
            lightDir = scene.registry.get<RotationComponent>(lightEntity).value * glm::vec3(0, -1, 0);
        }
        lightDir = glm::normalize(lightDir);
        glm::vec3 lightPos = -lightDir * m_ShadowProjectionSize;
        glm::mat4 lightProjection = glm::ortho(-m_ShadowProjectionSize, m_ShadowProjectionSize, -m_ShadowProjectionSize, m_ShadowProjectionSize, 0.1f, 200.0f);
        m_LightSpaceMatrixDir[lightIdx] = lightProjection * glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        shadowQueueMain.Submit([this, lightIdx]() { m_Shadow.BindFBO_Dir(lightIdx); m_Shadow.GetDrawContext().Clear(Graphics::BufferBit::Depth); });

        Frustum lightFrustum;
        if (m_ShadowFrustumCullingEnabled) lightFrustum.Update(m_LightSpaceMatrixDir[lightIdx]);

        size_t totalItems = shadowQueue.size();
        uint32_t numThreads = JobSystem::Instance().GetThreadCount();
        size_t chunkSize = (totalItems + numThreads - 1) / numThreads;
        std::vector<CommandQueue> threadQueues(numThreads);

        for (size_t i = 0; i < numThreads; ++i) {
            size_t startIdx = i * chunkSize;
            if (startIdx >= totalItems) break;
            size_t endIdx = std::min(startIdx + chunkSize, totalItems);

            JobSystem::Instance().Execute([this, &scene, &shadowQueue, startIdx, endIdx, &threadQueue = threadQueues[i], shaderDir, lightIdx, lightFrustum, camPos]() {
                threadQueue.Submit([shaderDir, this, lightIdx]() {
                    shaderDir->use();
                    shaderDir->setMat4("lightSpaceMatrix", m_LightSpaceMatrixDir[lightIdx]);
                });
                for (size_t k = startIdx; k < endIdx; ++k) {
                    const auto& item = shadowQueue[k];
                    if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity)) continue;
                    auto& renderer = scene.registry.get<MeshRendererComponent>(item.entity);
                    AABB worldAABB = renderer.model->aabb.Transform(item.worldMatrix);
                    float dx = (std::max)(worldAABB.minBound.x - camPos.x, (std::max)(0.0f, camPos.x - worldAABB.maxBound.x));
                    float dy = (std::max)(worldAABB.minBound.y - camPos.y, (std::max)(0.0f, camPos.y - worldAABB.maxBound.y));
                    float dz = (std::max)(worldAABB.minBound.z - camPos.z, (std::max)(0.0f, camPos.z - worldAABB.maxBound.z));
                    if (m_ShadowDistanceCullingSq > 0.0f && (dx*dx+dy*dy+dz*dz) > m_ShadowDistanceCullingSq) continue;
                    if (m_ShadowFrustumCullingEnabled && !lightFrustum.IsBoxVisible(worldAABB.minBound, worldAABB.maxBound)) continue;

                    Model* activeModel = item.activeModel;
                    glm::mat4 worldMat = item.worldMatrix;
                    std::vector<glm::mat4> transforms;
                    bool hasAnim = false;
                    if (scene.registry.all_of<AnimationComponent>(item.entity)) {
                        auto &anim = scene.registry.get<AnimationComponent>(item.entity);
                        if (anim.animator) { transforms = anim.animator->GetFinalBoneMatrices(); hasAnim = true; }
                    }

                    threadQueue.Submit([shaderDir, worldMat, activeModel, transforms, hasAnim]() {
                        shaderDir->setMat4("model", worldMat);
                        shaderDir->setBool("hasAnimation", hasAnim);
                        if (hasAnim) shaderDir->setMat4Array("finalBonesMatrices", transforms);
                        activeModel->Draw(*shaderDir);
                    });
                }
            }, &counter);
        }
        JobSystem::Instance().Wait(&counter);
        for (auto& tq : threadQueues) shadowQueueMain.Merge(tq);
    }

    // 2. Point Shadows
    if (shaderPoint) {
        auto pointView = scene.registry.view<PointLightComponent, PositionComponent>();
        std::vector<entt::entity> pointLights;
        for (auto entity : pointView) {
            if (pointView.get<PointLightComponent>(entity).isCastShadow && pointView.get<PointLightComponent>(entity).active) {
                pointLights.push_back(entity);
                if (pointLights.size() >= Shadow::MAX_POINT_LIGHTS_SHADOW) break;
            }
        }
        for (int pIdx = 0; pIdx < pointLights.size(); ++pIdx) {
            auto [light, posComp] = pointView.get<PointLightComponent, PositionComponent>(pointLights[pIdx]);
            glm::vec3 lightPos = posComp.value;
            float farP = m_FarPlanePoint;
            float aspect = (float)m_Shadow.GetShadowPointWidth() / (float)m_Shadow.GetShadowPointHeight();
            glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), aspect, 1.0f, farP);
            std::vector<glm::mat4> shadowTransforms = {
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1,0,0), glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1,0,0), glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,1,0), glm::vec3(0,0,1)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,-1,0), glm::vec3(0,0,-1)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,0,1), glm::vec3(0,-1,0)),
                shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0,0,-1), glm::vec3(0,-1,0))
            };

            shadowQueueMain.Submit([this, pIdx]() { m_Shadow.BindFBO_Point(pIdx); m_Shadow.GetDrawContext().Clear(Graphics::BufferBit::Depth); });

            size_t totalItems = shadowQueue.size();
            size_t numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 1;
            size_t chunkSize = (totalItems + numThreads - 1) / numThreads;
            std::vector<CommandQueue> threadQueues(numThreads);

            for (size_t i = 0; i < numThreads; ++i) {
                size_t startIdx = i * chunkSize;
                if (startIdx >= totalItems) break;
                size_t endIdx = std::min(startIdx + chunkSize, totalItems);
                JobSystem::Instance().Execute([this, &scene, &shadowQueue, startIdx, endIdx, &threadQueue = threadQueues[i], shaderPoint, shadowTransforms, farP, lightPos, camPos]() {
                    threadQueue.Submit([shaderPoint, shadowTransforms, farP, lightPos]() {
                        shaderPoint->use();
                        for (int k = 0; k < 6; ++k) shaderPoint->setMat4("shadowMatrices[" + std::to_string(k) + "]", shadowTransforms[k]);
                        shaderPoint->setFloat("farPlane", farP);
                        shaderPoint->setVec3("lightPos", lightPos);
                    });
                    for (size_t k = startIdx; k < endIdx; ++k) {
                        const auto& item = shadowQueue[k];
                        if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity)) continue;
                        auto& renderer = scene.registry.get<MeshRendererComponent>(item.entity);
                        AABB worldAABB = renderer.model->aabb.Transform(item.worldMatrix);
                        float dx = (std::max)(worldAABB.minBound.x - camPos.x, (std::max)(0.0f, camPos.x - worldAABB.maxBound.x));
                        float dy = (std::max)(worldAABB.minBound.y - camPos.y, (std::max)(0.0f, camPos.y - worldAABB.maxBound.y));
                        float dz = (std::max)(worldAABB.minBound.z - camPos.z, (std::max)(0.0f, camPos.z - worldAABB.maxBound.z));
                        if (m_ShadowDistanceCullingSq > 0.0f && (dx*dx+dy*dy+dz*dz) > m_ShadowDistanceCullingSq) continue;

                        Model* activeModel = item.activeModel;
                        glm::mat4 worldMat = item.worldMatrix;
                        std::vector<glm::mat4> transforms;
                        bool hasAnim = false;
                        if (scene.registry.all_of<AnimationComponent>(item.entity)) {
                            auto &anim = scene.registry.get<AnimationComponent>(item.entity);
                            if (anim.animator) { transforms = anim.animator->GetFinalBoneMatrices(); hasAnim = true; }
                        }
                        threadQueue.Submit([shaderPoint, worldMat, activeModel, transforms, hasAnim]() {
                            shaderPoint->setMat4("model", worldMat);
                            shaderPoint->setBool("hasAnimation", hasAnim);
                            if (hasAnim) shaderPoint->setMat4Array("finalBonesMatrices", transforms);
                            activeModel->Draw(*shaderPoint);
                        });
                    }
                }, &counter);
            }
            JobSystem::Instance().Wait(&counter);
            for (auto& tq : threadQueues) shadowQueueMain.Merge(tq);
        }
    }

    // 3. Spot Shadows
    if (shaderSpot) {
        auto spotView = scene.registry.view<SpotLightComponent, PositionComponent, RotationComponent>();
        std::vector<entt::entity> spotLights;
        for (auto entity : spotView) {
            if (spotView.get<SpotLightComponent>(entity).isCastShadow && spotView.get<SpotLightComponent>(entity).active) {
                spotLights.push_back(entity);
                if (spotLights.size() >= Shadow::MAX_SPOT_LIGHTS_SHADOW) break;
            }
        }
        for (int sIdx = 0; sIdx < spotLights.size(); ++sIdx) {
            auto [light, posComp, rotComp] = spotView.get<SpotLightComponent, PositionComponent, RotationComponent>(spotLights[sIdx]);
            glm::vec3 lightPos = posComp.value;
            glm::vec3 lightDir = glm::normalize(rotComp.value * glm::vec3(0, -1, 0));
            float fov = glm::acos(light.cutOff) * 2.0f;
            glm::mat4 spotProjection = glm::perspective(fov, 1.0f, 0.1f, m_FarPlaneSpot);
            glm::vec3 up = std::abs(glm::dot(lightDir, glm::vec3(0,1,0))) > 0.99f ? glm::vec3(0,0,1) : glm::vec3(0,1,0);
            m_LightSpaceMatrixSpot[sIdx] = spotProjection * glm::lookAt(lightPos, lightPos + lightDir, up);

            shadowQueueMain.Submit([this, sIdx]() { m_Shadow.BindFBO_Spot(sIdx); m_Shadow.GetDrawContext().Clear(Graphics::BufferBit::Depth); });

            Frustum lightFrustum;
            if (m_ShadowFrustumCullingEnabled) lightFrustum.Update(m_LightSpaceMatrixSpot[sIdx]);

            size_t totalItems = shadowQueue.size();
            size_t numThreads = std::thread::hardware_concurrency();
            if (numThreads == 0) numThreads = 1;
            size_t chunkSize = (totalItems + numThreads - 1) / numThreads;
            std::vector<CommandQueue> threadQueues(numThreads);

            for (size_t i = 0; i < numThreads; ++i) {
                size_t startIdx = i * chunkSize;
                if (startIdx >= totalItems) break;
                size_t endIdx = std::min(startIdx + chunkSize, totalItems);
                JobSystem::Instance().Execute([this, &scene, &shadowQueue, startIdx, endIdx, &threadQueue = threadQueues[i], shaderSpot, sIdx, lightFrustum, camPos]() {
                    threadQueue.Submit([shaderSpot, this, sIdx]() {
                        shaderSpot->use();
                        shaderSpot->setMat4("lightSpaceMatrix", m_LightSpaceMatrixSpot[sIdx]);
                    });
                    for (size_t k = startIdx; k < endIdx; ++k) {
                        const auto& item = shadowQueue[k];
                        if (!scene.registry.valid(item.entity) || !scene.registry.all_of<MeshRendererComponent>(item.entity)) continue;
                        auto& renderer = scene.registry.get<MeshRendererComponent>(item.entity);
                        AABB worldAABB = renderer.model->aabb.Transform(item.worldMatrix);
                        float dx = (std::max)(worldAABB.minBound.x - camPos.x, (std::max)(0.0f, camPos.x - worldAABB.maxBound.x));
                        float dy = (std::max)(worldAABB.minBound.y - camPos.y, (std::max)(0.0f, camPos.y - worldAABB.maxBound.y));
                        float dz = (std::max)(worldAABB.minBound.z - camPos.z, (std::max)(0.0f, camPos.z - worldAABB.maxBound.z));
                        if (m_ShadowDistanceCullingSq > 0.0f && (dx*dx+dy*dy+dz*dz) > m_ShadowDistanceCullingSq) continue;
                        if (m_ShadowFrustumCullingEnabled && !lightFrustum.IsBoxVisible(worldAABB.minBound, worldAABB.maxBound)) continue;
                        Model* activeModel = item.activeModel;
                        glm::mat4 worldMat = item.worldMatrix;
                        std::vector<glm::mat4> transforms;
                        bool hasAnim = false;
                        if (scene.registry.all_of<AnimationComponent>(item.entity)) {
                            auto &anim = scene.registry.get<AnimationComponent>(item.entity);
                            if (anim.animator) { transforms = anim.animator->GetFinalBoneMatrices(); hasAnim = true; }
                        }
                        threadQueue.Submit([shaderSpot, worldMat, activeModel, transforms, hasAnim]() {
                            shaderSpot->setMat4("model", worldMat);
                            shaderSpot->setBool("hasAnimation", hasAnim);
                            if (hasAnim) shaderSpot->setMat4Array("finalBonesMatrices", transforms);
                            activeModel->Draw(*shaderSpot);
                        });
                    }
                }, &counter);
            }
            JobSystem::Instance().Wait(&counter);
            for (auto& tq : threadQueues) shadowQueueMain.Merge(tq);
        }
    }

    shadowQueueMain.Submit([this]() { m_Shadow.UnbindFBO(); });
    shadowQueueMain.Execute();
}
