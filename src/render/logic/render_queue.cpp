#include <ecs/unit/core_components.h>
#include <render/unit/render_queue.h>
#include <core/logic/job_system.h>
#include <render/logic/frustum_culler.h>
#include <ecs/unit/render_components.h>
#include <resource/unit/shader.h>
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
                if (!renderer.model) continue;

                glm::mat4 modelMatrix = world.GetInterpolated(alpha);
                
                AABB worldAABB = renderer.model->aabb.Transform(modelMatrix);
                float distSqResult = worldAABB.DistanceSq(camPos);

                if (distCullSq > 0.0f && distSqResult > distCullSq) continue;

                Model* activeModel = renderer.model.get();
                Shader* itemShader = renderer.shader.expired() ? nullptr : renderer.shader.lock().get();

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
                auto* material = scene.registry.try_get<MaterialComponent>(entity);
                if (material) {
                    if (material->desc.opacity < 1.0f) isTransparent = true;
                }

                uint64_t key = 0;
                uint64_t sId = itemShader ? itemShader->getID() : 0;
                if (!isTransparent) {
                    // Opaque: Layer (8) | Order (8) | Shader (16) | Material (16) | Model (8) | Depth (8)
                    uint64_t l = (uint64_t)(layer & 0xFF) << 56;
                    uint64_t o = (uint64_t)(renderer.order & 0xFF) << 48;
                    uint64_t s = (uint64_t)(sId & 0xFFFF) << 32;
                    uint64_t m = (uint64_t)((uintptr_t)material & 0xFFFF) << 16;
                    uint64_t mod = (uint64_t)((uintptr_t)activeModel & 0xFF) << 8;
                    uint64_t d = (uint64_t)(glm::clamp(distSqResult * 0.1f, 0.0f, 255.0f)) & 0xFF;
                    key = l | o | s | m | mod | d;
                } else {
                    // Transparent: Layer (8) | Reversed Depth (56)
                    uint64_t l = (uint64_t)(layer & 0xFF) << 56;
                    float invDepth = 1000000.0f - distSqResult; 
                    if (invDepth < 0) invDepth = 0;
                    uint64_t d = (uint64_t)(invDepth) & 0x00FFFFFFFFFFFFFFULL;
                    key = l | d;
                }

                res.renderQueue.push_back({entity, activeModel, itemShader, material, modelMatrix, worldAABB, layer, renderer.order, distSqResult, isTransparent, key});
                if (renderer.castShadow) res.shadowQueue.push_back({entity, activeModel, itemShader, material, modelMatrix, worldAABB, layer, renderer.order, distSqResult, isTransparent, key});
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

    // Sort Opaque: Using pre-computed sortKey
    std::sort(m_OpaqueQueue.begin(), m_OpaqueQueue.end(), [](const RenderItem& lhs, const RenderItem& rhs) {
        return lhs.sortKey < rhs.sortKey;
    });

    // Sort Transparent: Using pre-computed sortKey
    std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(), [](const RenderItem& lhs, const RenderItem& rhs) {
        return lhs.sortKey < rhs.sortKey;
    });
}
