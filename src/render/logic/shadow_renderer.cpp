#include <render/logic/shadow_renderer.h>
#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <resource/unit/model.h>
#include <core/logic/job_system.h>
#include <render/interface/i_draw_context.h>
#include <resource/logic/resource_manager.h>
#include <core/logic/logger.h>
#include <render/unit/command_queue.h>
#include <core/logic/config_manager.h>
#include <core/type/app_config.h>
#include <core/type/lighting_mode.h>
#include <core/logic/service_locator.h>

void ShadowRenderer::Initialize(IGraphicsContext& context, IShaderLibrary &shaderLib)
{
    m_Shadow.Initialize(context);

    m_Shadow.SetShaderDir(shaderLib.GetShader("shadow_depth").get());
    m_Shadow.SetShaderPoint(shaderLib.GetShader("shadow_point").get());
    m_Shadow.SetShaderSpot(shaderLib.GetShader("shadow_spot").get());
}

void ShadowRenderer::Shutdown()
{
    m_Shadow.Shutdown();
}

void ShadowRenderer::PerformShadowPass(const RenderSceneData& sceneData)
{
    if (m_ShadowMode == 0 || !m_EnableShadows) return;

    auto& config = ServiceLocator::Instance().Require<ConfigManager>().GetConfig();
    LightingMode lightingMode = config.lightingMode;

    Shader *shaderDir = m_Shadow.GetShaderDir();
    Shader *shaderPoint = m_Shadow.GetShaderPoint();
    Shader *shaderSpot = m_Shadow.GetShaderSpot();

    if (!shaderDir) return;

    std::vector<const RenderLight*> shadowCastingDirLights;
    std::vector<const RenderLight*> shadowCastingPointLights;
    std::vector<const RenderLight*> shadowCastingSpotLights;

    for (const auto& light : sceneData.lights) {
        if (!light.castShadows) continue;
        if (light.type == RenderLightType::Directional) shadowCastingDirLights.push_back(&light);
        else if (light.type == RenderLightType::Point) shadowCastingPointLights.push_back(&light);
        else if (light.type == RenderLightType::Spot) shadowCastingSpotLights.push_back(&light);
    }

    if (shadowCastingDirLights.empty() && 
        shadowCastingPointLights.empty() && 
        shadowCastingSpotLights.empty()) return;

    glm::vec3 camPos = sceneData.cameraPosition;
    CommandQueue shadowQueueMain;
    JobSystem::JobCounter counter(0);

    const auto& shadowQueue = sceneData.shadowQueue;

    // Directional Lights
    int numDirShadows = (m_ShadowMode == 2) ? (std::min)((int)shadowCastingDirLights.size(), Shadow::MAX_DIR_LIGHTS_SHADOW) : (shadowCastingDirLights.empty() ? 0 : 1);
    
    for (int lightIdx = 0; lightIdx < numDirShadows; ++lightIdx) {
        const auto* light = shadowCastingDirLights[lightIdx];
        m_LightSpaceMatrixDir[lightIdx] = light->viewProj;

        // Bake mode optimization: skip if light version matches and already initialized
        bool needsUpdate = (lightingMode != LightingMode::Bake) || 
                           (light->version != m_LastLightVersionsDir[lightIdx]) || 
                           (!m_ShadowMapInitializedDir[lightIdx]);

        if (!needsUpdate) continue;

        m_LastLightVersionsDir[lightIdx] = light->version;
        m_ShadowMapInitializedDir[lightIdx] = true;

        shadowQueueMain.Submit([this, lightIdx]() { m_Shadow.BindFBO_Dir(lightIdx); m_Shadow.GetDrawContext().Clear(BufferBit::Depth); });

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

            JobSystem::Instance().Execute([this, &shadowQueue, startIdx, endIdx, &threadQueue = threadQueues[i], shaderDir, lightIdx, lightFrustum, camPos, lightingMode]() {
                threadQueue.Submit([shaderDir, this, lightIdx]() {
                    shaderDir->use();
                    shaderDir->setMat4("u_LightSpaceMatrix", m_LightSpaceMatrixDir[lightIdx]);
                });
                for (size_t k = startIdx; k < endIdx; ++k) {
                    const auto& item = shadowQueue[k];
                    
                    if (m_ShadowDistanceCullingSq > 0.0f && item.distanceSq > m_ShadowDistanceCullingSq) continue;
                    if (m_ShadowFrustumCullingEnabled && !lightFrustum.IsBoxVisible(item.worldAABB)) continue;

                    // Bake mode: only static objects cast shadows
                    if (lightingMode == LightingMode::Bake && !item.isStatic) continue;

                    Model* activeModel = item.model;
                    glm::mat4 worldMat = item.worldMatrix;

                    if (activeModel) {
                        threadQueue.Submit([shaderDir, worldMat, activeModel, hasAnim = item.hasAnimation, bones = item.boneMatrices]() {
                            shaderDir->setMat4("u_Model", worldMat);
                            shaderDir->setBool("u_HasAnimation", hasAnim);
                            if (hasAnim) shaderDir->setMat4Array("u_FinalBonesMatrices", bones);
                            activeModel->Draw(*shaderDir);
                        });
                    }
                }
            }, &counter);
        }
        JobSystem::Instance().Wait(&counter);
        for (auto& tq : threadQueues) shadowQueueMain.Merge(tq);
    }

    // Point Lights
    if (shaderPoint) {
        int numPointShadows = (std::min)((int)shadowCastingPointLights.size(), Shadow::MAX_POINT_LIGHTS_SHADOW);
        for (int pIdx = 0; pIdx < numPointShadows; ++pIdx) {
            const auto* light = shadowCastingPointLights[pIdx];

            // Bake mode optimization
            bool needsUpdate = (lightingMode != LightingMode::Bake) || 
                               (light->version != m_LastLightVersionsPoint[pIdx]) || 
                               (!m_ShadowMapInitializedPoint[pIdx]);

            if (!needsUpdate) continue;

            m_LastLightVersionsPoint[pIdx] = light->version;
            m_ShadowMapInitializedPoint[pIdx] = true;

            glm::vec3 lightPos = light->position;
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

            shadowQueueMain.Submit([this, pIdx]() { m_Shadow.BindFBO_Point(pIdx); m_Shadow.GetDrawContext().Clear(BufferBit::Depth); });

            size_t totalItems = shadowQueue.size();
            uint32_t numThreads = JobSystem::Instance().GetThreadCount();
            size_t chunkSize = (totalItems + numThreads - 1) / numThreads;
            std::vector<CommandQueue> threadQueues(numThreads);

            for (size_t i = 0; i < numThreads; ++i) {
                size_t startIdx = i * chunkSize;
                if (startIdx >= totalItems) break;
                size_t endIdx = std::min(startIdx + chunkSize, totalItems);
                JobSystem::Instance().Execute([this, &shadowQueue, startIdx, endIdx, &threadQueue = threadQueues[i], shaderPoint, shadowTransforms, farP, lightPos, camPos, lightingMode]() {
                    threadQueue.Submit([shaderPoint, shadowTransforms, farP, lightPos]() {
                        shaderPoint->use();
                        for (int k = 0; k < 6; ++k) shaderPoint->setMat4("u_ShadowMatrices[" + std::to_string(k) + "]", shadowTransforms[k]);
                        shaderPoint->setFloat("u_FarPlane", farP);
                        shaderPoint->setVec3("u_LightPos", lightPos);
                    });
                    for (size_t k = startIdx; k < endIdx; ++k) {
                        const auto& item = shadowQueue[k];
                        if (m_ShadowDistanceCullingSq > 0.0f && item.distanceSq > m_ShadowDistanceCullingSq) continue;

                        // Bake mode: only static objects cast shadows
                        if (lightingMode == LightingMode::Bake && !item.isStatic) continue;

                        Model* activeModel = item.model;
                        glm::mat4 worldMat = item.worldMatrix;
                        if (activeModel) {
                            threadQueue.Submit([shaderPoint, worldMat, activeModel, hasAnim = item.hasAnimation, bones = item.boneMatrices]() {
                                shaderPoint->setMat4("u_Model", worldMat);
                                shaderPoint->setBool("u_HasAnimation", hasAnim);
                                if (hasAnim) shaderPoint->setMat4Array("u_FinalBonesMatrices", bones);
                                activeModel->Draw(*shaderPoint);
                            });
                        }
                    }
                }, &counter);
            }
            JobSystem::Instance().Wait(&counter);
            for (auto& tq : threadQueues) shadowQueueMain.Merge(tq);
        }
    }

    // Spot Lights
    if (shaderSpot) {
        int numSpotShadows = (std::min)((int)shadowCastingSpotLights.size(), Shadow::MAX_SPOT_LIGHTS_SHADOW);
        for (int sIdx = 0; sIdx < numSpotShadows; ++sIdx) {
            const auto* light = shadowCastingSpotLights[sIdx];
            m_LightSpaceMatrixSpot[sIdx] = light->viewProj;

            // Bake mode optimization
            bool needsUpdate = (lightingMode != LightingMode::Bake) || 
                               (light->version != m_LastLightVersionsSpot[sIdx]) || 
                               (!m_ShadowMapInitializedSpot[sIdx]);

            if (!needsUpdate) continue;

            m_LastLightVersionsSpot[sIdx] = light->version;
            m_ShadowMapInitializedSpot[sIdx] = true;

            shadowQueueMain.Submit([this, sIdx]() { m_Shadow.BindFBO_Spot(sIdx); m_Shadow.GetDrawContext().Clear(BufferBit::Depth); });

            Frustum lightFrustum;
            if (m_ShadowFrustumCullingEnabled) lightFrustum.Update(m_LightSpaceMatrixSpot[sIdx]);

            size_t totalItems = shadowQueue.size();
            uint32_t numThreads = JobSystem::Instance().GetThreadCount();
            size_t chunkSize = (totalItems + numThreads - 1) / numThreads;
            std::vector<CommandQueue> threadQueues(numThreads);

            for (size_t i = 0; i < numThreads; ++i) {
                size_t startIdx = i * chunkSize;
                if (startIdx >= totalItems) break;
                size_t endIdx = std::min(startIdx + chunkSize, totalItems);
                JobSystem::Instance().Execute([this, &shadowQueue, startIdx, endIdx, &threadQueue = threadQueues[i], shaderSpot, sIdx, lightFrustum, camPos, lightingMode]() {
                    threadQueue.Submit([shaderSpot, this, sIdx]() {
                        shaderSpot->use();
                        shaderSpot->setMat4("u_LightSpaceMatrix", m_LightSpaceMatrixSpot[sIdx]);
                    });
                    for (size_t k = startIdx; k < endIdx; ++k) {
                        const auto& item = shadowQueue[k];
                        if (m_ShadowDistanceCullingSq > 0.0f && item.distanceSq > m_ShadowDistanceCullingSq) continue;
                        if (m_ShadowFrustumCullingEnabled && !lightFrustum.IsBoxVisible(item.worldAABB)) continue;
                        
                        // Bake mode: only static objects cast shadows
                        if (lightingMode == LightingMode::Bake && !item.isStatic) continue;
                        
                        Model* activeModel = item.model;
                        glm::mat4 worldMat = item.worldMatrix;
                        if (activeModel) {
                            threadQueue.Submit([shaderSpot, worldMat, activeModel, hasAnim = item.hasAnimation, bones = item.boneMatrices]() {
                                shaderSpot->setMat4("u_Model", worldMat);
                                shaderSpot->setBool("u_HasAnimation", hasAnim);
                                if (hasAnim) shaderSpot->setMat4Array("u_FinalBonesMatrices", bones);
                                activeModel->Draw(*shaderSpot);
                            });
                        }
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
