#include <ecs/unit/core_components.h>
#include <render/logic/render_queue.h>
#include <core/logic/job_system.h>
#include <render/logic/frustum_culler.h>
#include <ecs/unit/render_components.h>
#include <render/logic/shader.h>
#include <algorithm>

void RenderQueue::Build(Scene& scene, float alpha, 
                        const FrustumCuller& frustumCuller, bool frustumCullEnabled, 
                        bool occlusionCullEnabled, float distCullSq, 
                        uint32_t filterMask, uint32_t camMask, const glm::vec3& camPos) {
    
    m_OpaqueQueue.clear();
    m_TransparentQueue.clear();
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

                glm::mat4 modelMatrix = world.GetInterpolated(alpha);
                
                AABB worldAABB = renderer.model->aabb.Transform(modelMatrix);
                float distSqResult = worldAABB.DistanceSq(camPos);

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

                res.renderQueue.push_back({entity, activeModel, modelMatrix, worldAABB, layer, renderer.order, distSqResult, isTransparent});
                if (renderer.castShadow) res.shadowQueue.push_back({entity, activeModel, modelMatrix, worldAABB, layer, renderer.order, distSqResult, isTransparent});
            }
        }, &counter);
    }

    JobSystem::Instance().Wait(&counter);

    for (const auto& res : threadResults) {
        for (const auto& item : res.renderQueue) {
            if (item.isTransparent) m_TransparentQueue.push_back(item);
            else m_OpaqueQueue.push_back(item);
        }
        m_ShadowQueue.insert(m_ShadowQueue.end(), res.shadowQueue.begin(), res.shadowQueue.end());
    }

    // Sort Opaque: Front-to-back (actually by layer, then shader/mat for batching)
    std::sort(m_OpaqueQueue.begin(), m_OpaqueQueue.end(), [&scene](const RenderItem& lhs, const RenderItem& rhs) {
        if (lhs.layer != rhs.layer) return lhs.layer < rhs.layer;
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
    });

    // Sort Transparent: Back-to-front
    std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(), [](const RenderItem& lhs, const RenderItem& rhs) {
        if (lhs.layer != rhs.layer) return lhs.layer < rhs.layer;
        return lhs.distSq > rhs.distSq;
    });
}
