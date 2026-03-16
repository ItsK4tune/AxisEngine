#include <ecs/unit/core_components.h>
#include <render/logic/frustum_culler.h>
#include <core/logic/job_system.h>
#include <ecs/unit/render_components.h>
#include <algorithm>

void FrustumCuller::BuildFrustum(const glm::mat4& viewProj) {
    m_Frustum.Update(viewProj);
}

bool FrustumCuller::IsVisible(const glm::vec3& minBound, const glm::vec3& maxBound) const {
    return m_Frustum.IsBoxVisible(minBound, maxBound);
}

bool FrustumCuller::IsVisible(Scene& scene, entt::entity entity, float alpha) const {
    if (!scene.registry.all_of<WorldTransformComponent, MeshRendererComponent>(entity)) return false;
    auto [world, renderer] = scene.registry.get<WorldTransformComponent, MeshRendererComponent>(entity);
    if (!renderer.model) return false;

    glm::mat4 modelMatrix = world.GetInterpolated(alpha);
    AABB transformedAABB = renderer.model->aabb.Transform(modelMatrix);
    return IsVisible(transformedAABB.minBound, transformedAABB.maxBound);
}

void FrustumCuller::Cull(Scene& scene, bool frustumCullEnabled, std::vector<entt::entity>& outVisibleEntities, float alpha) const {
    outVisibleEntities.clear();
    
    auto renderView = scene.registry.view<WorldTransformComponent, MeshRendererComponent>();
    uint32_t totalEntities = (uint32_t)renderView.size_hint();
    if (totalEntities == 0) return;

    // Threshold for Octree rebuild. Rebuilding Octree is expensive (O(N log N)).
    // Direct culling is O(N) but very fast per-item.
    const uint32_t OCTREE_THRESHOLD = 5000; 

    if (scene.GetOctree() && totalEntities >= OCTREE_THRESHOLD) {
        // Rebuild and Query Octree path
        std::vector<entt::entity> entities;
        entities.reserve(totalEntities);
        for (auto entity : renderView) entities.push_back(entity);

        uint32_t numThreads = JobSystem::Instance().GetThreadCount();
        uint32_t chunkSize = (totalEntities + numThreads - 1) / numThreads;
        std::vector<std::vector<OctreeElement>> threadElements(numThreads);
        JobSystem::JobCounter counter(0);

        for (uint32_t i = 0; i < numThreads; ++i) {
            uint32_t startIdx = i * chunkSize;
            if (startIdx >= totalEntities) break;
            uint32_t endIdx = (std::min)(startIdx + chunkSize, totalEntities);

            JobSystem::Instance().Execute([&scene, &entities, startIdx, endIdx, &res = threadElements[i], alpha]() {
                for (uint32_t k = startIdx; k < endIdx; ++k) {
                    entt::entity entity = entities[k];
                    auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
                    if (!renderer.model) continue;

                    auto& world = scene.registry.get<WorldTransformComponent>(entity);
                    glm::mat4 modelMatrix = (alpha >= 0.99f) ? world.worldMatrix : world.GetInterpolated(alpha);
                    res.push_back({entity, renderer.model->aabb.Transform(modelMatrix)});
                }
            }, &counter);
        }
        JobSystem::Instance().Wait(&counter);

        std::vector<OctreeElement> elements;
        elements.reserve(totalEntities);
        for (const auto& res : threadElements) elements.insert(elements.end(), res.begin(), res.end());
        
        scene.GetOctree()->Rebuild(elements);
        
        if (frustumCullEnabled) {
            scene.GetOctree()->Query(m_Frustum, outVisibleEntities);
        } else {
            for (const auto& el : elements) outVisibleEntities.push_back(el.entity);
        }
    } else {
        // Direct Parallel Culling path
        std::vector<entt::entity> entities;
        entities.reserve(totalEntities);
        for (auto entity : renderView) entities.push_back(entity);

        uint32_t numThreads = JobSystem::Instance().GetThreadCount();
        uint32_t chunkSize = (totalEntities + numThreads - 1) / numThreads;
        std::vector<std::vector<entt::entity>> threadVisible(numThreads);
        JobSystem::JobCounter counter(0);

        for (uint32_t i = 0; i < numThreads; ++i) {
            uint32_t startIdx = i * chunkSize;
            if (startIdx >= totalEntities) break;
            uint32_t endIdx = (std::min)(startIdx + chunkSize, totalEntities);

            JobSystem::Instance().Execute([this, &scene, &entities, startIdx, endIdx, &res = threadVisible[i], alpha, frustumCullEnabled]() {
                for (uint32_t k = startIdx; k < endIdx; ++k) {
                    entt::entity entity = entities[k];
                    auto& renderer = scene.registry.get<MeshRendererComponent>(entity);
                    
                    if (!renderer.model || !frustumCullEnabled) {
                        res.push_back(entity);
                        continue;
                    }

                    auto& world = scene.registry.get<WorldTransformComponent>(entity);
                    glm::mat4 modelMatrix = (alpha >= 0.99f) ? world.worldMatrix : world.GetInterpolated(alpha);
                    AABB transformedAABB = renderer.model->aabb.Transform(modelMatrix);
                    
                    if (IsVisible(transformedAABB.minBound, transformedAABB.maxBound)) {
                        res.push_back(entity);
                    }
                }
            }, &counter);
        }
        JobSystem::Instance().Wait(&counter);

        for (const auto& res : threadVisible) {
            outVisibleEntities.insert(outVisibleEntities.end(), res.begin(), res.end());
        }
    }
}
