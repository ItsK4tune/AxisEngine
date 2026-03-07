#include <rendering/renderer/render_queue.h>
#include <core/job_system.h>
#include <rendering/renderer/frustum_culler.h>
#include <ecs/component.h>
#include <ecs/components/occlusion_component.h>
#include <ecs/components/render_components.h>
#include <ecs/components/info_component.h>
#include <rendering/core/shader.h>
#include <algorithm>

void RenderQueue::Build(Scene& scene, float alpha, 
                        const FrustumCuller& frustumCuller, bool frustumCullEnabled, 
                        bool occlusionCullEnabled, float distCullSq, 
                        uint32_t filterMask, uint32_t camMask, const glm::vec3& camPos) {
    
    m_RenderQueue.clear();
    m_ShadowQueue.clear();

    std::vector<entt::entity> visibleEntities;
    frustumCuller.Cull(scene, frustumCullEnabled, visibleEntities, alpha);

    uint32_t totalVisible = (uint32_t)visibleEntities.size();
    if (totalVisible == 0) return;

    uint32_t numThreads = JobSystem::Instance().GetThreadCount();
    uint32_t chunkSize = (totalVisible + numThreads - 1) / numThreads;

    struct ThreadResult {
        std::vector<RenderItem> renderQueue;
        std::vector<RenderItem> shadowQueue;
    };
    std::vector<ThreadResult> threadResults(numThreads);
    JobSystem::JobCounter counter(0);

    for (uint32_t i = 0; i < numThreads; ++i) {
        uint32_t startIdx = i * chunkSize;
        if (startIdx >= totalVisible) break;
        uint32_t endIdx = (std::min)(startIdx + chunkSize, totalVisible);

        JobSystem::Instance().Execute([&scene, &visibleEntities, startIdx, endIdx, &res = threadResults[i], alpha, occlusionCullEnabled, distCullSq, filterMask, camMask, camPos, &frustumCuller, frustumCullEnabled]() {
            for (uint32_t k = startIdx; k < endIdx; ++k) {
                entt::entity entity = visibleEntities[k];

                if (!scene.registry.all_of<WorldTransformComponent, MeshRendererComponent>(entity)) continue;

                auto& world = scene.registry.get<WorldTransformComponent>(entity);
                auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
                if (!renderer.model || renderer.shader.expired()) continue;

                glm::mat4 modelMatrix = glm::mat4(1.0f);
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        modelMatrix[r][c] = glm::mix(world.prevWorldMatrix[r][c], world.worldMatrix[r][c], alpha);
                    }
                }
                
                AABB worldAABB = renderer.model->aabb.Transform(modelMatrix);
                
                float dx = (std::max)(worldAABB.minBound.x - camPos.x, (std::max)(0.0f, camPos.x - worldAABB.maxBound.x));
                float dy = (std::max)(worldAABB.minBound.y - camPos.y, (std::max)(0.0f, camPos.y - worldAABB.maxBound.y));
                float dz = (std::max)(worldAABB.minBound.z - camPos.z, (std::max)(0.0f, camPos.z - worldAABB.maxBound.z));
                float distSqResult = dx * dx + dy * dy + dz * dz;

                if (distCullSq > 0.0f && distSqResult > distCullSq) continue;

                Model* activeModel = renderer.model.get();
                if (auto* lod = scene.registry.try_get<LODComponent>(entity)) {
                    for (int j = 0; j < (int)lod->lodDistancesSq.size(); ++j) {
                        if (distSqResult > lod->lodDistancesSq[j] && j < (int)lod->lodModels.size() && lod->lodModels[j]) {
                            activeModel = lod->lodModels[j].get();
                        } else break;
                    }
                }

                uint32_t layer = 1;
                if (auto* info = scene.registry.try_get<InfoComponent>(entity)) layer = info->layer;
                if ((filterMask & layer) == 0 || (camMask & layer) == 0) continue;

                if (occlusionCullEnabled) {
                    if (auto* occ = scene.registry.try_get<OcclusionComponent>(entity)) {
                        if (!occ->isVisible) continue;
                    }
                }

                bool isTransparent = false;
                if (auto* mat = scene.registry.try_get<MaterialComponent>(entity)) {
                    if (mat->desc.opacity < 1.0f) isTransparent = true;
                }

                res.renderQueue.push_back({entity, activeModel, modelMatrix, layer, renderer.order, distSqResult, isTransparent});
                if (renderer.castShadow) res.shadowQueue.push_back({entity, activeModel, modelMatrix, layer, renderer.order, distSqResult, isTransparent});
            }
        }, &counter);
    }

    JobSystem::Instance().Wait(&counter);

    for (const auto& res : threadResults) {
        m_RenderQueue.insert(m_RenderQueue.end(), res.renderQueue.begin(), res.renderQueue.end());
        m_ShadowQueue.insert(m_ShadowQueue.end(), res.shadowQueue.begin(), res.shadowQueue.end());
    }

    std::sort(m_RenderQueue.begin(), m_RenderQueue.end(), [&scene](const RenderItem& lhs, const RenderItem& rhs) {
        if (lhs.layer != rhs.layer) return lhs.layer < rhs.layer;
        if (lhs.isTransparent != rhs.isTransparent) return !lhs.isTransparent;

        if (!lhs.isTransparent) {
            if (lhs.renderOrder != rhs.renderOrder) return lhs.renderOrder < rhs.renderOrder;

            auto& lRenderer = scene.registry.get<MeshRendererComponent>(lhs.entity);
            auto& rRenderer = scene.registry.get<MeshRendererComponent>(rhs.entity);

            auto lShader = lRenderer.shader.lock();
            auto rShader = rRenderer.shader.lock();
            unsigned int lID = lShader ? lShader->getID() : 0;
            unsigned int rID = rShader ? rShader->getID() : 0;
            if (lID != rID) return lID < rID;
            
            auto lMat = scene.registry.try_get<MaterialComponent>(lhs.entity);
            auto rMat = scene.registry.try_get<MaterialComponent>(rhs.entity);
            if (lMat != rMat) return lMat < rMat;

            return lhs.activeModel < rhs.activeModel;
        } else {
            return lhs.distSq > rhs.distSq;
        }
    });
}
