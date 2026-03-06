#include <rendering/renderer/frustum_culler.h>
#include <core/job_system.h>
#include <ecs/component.h>
#include <ecs/components/render_components.h>
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

    glm::mat4 modelMatrix = glm::mat4(1.0f);
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            modelMatrix[r][c] = glm::mix(world.prevWorldMatrix[r][c], world.worldMatrix[r][c], alpha);
        }
    }
    AABB transformedAABB = renderer.model->aabb.Transform(modelMatrix);
    return IsVisible(transformedAABB.minBound, transformedAABB.maxBound);
}

void FrustumCuller::Cull(Scene& scene, bool frustumCullEnabled, std::vector<entt::entity>& outVisibleEntities, float alpha) const {
    outVisibleEntities.clear();
    if (scene.GetOctree()) {
        auto view = scene.registry.view<WorldTransformComponent, MeshRendererComponent>();
        std::vector<entt::entity> entities;
        for (auto entity : view) entities.push_back(entity);
        uint32_t totalEntities = (uint32_t)entities.size();

        if (totalEntities == 0) return;

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
                    if (!scene.registry.all_of<WorldTransformComponent, MeshRendererComponent>(entity)) continue;
                    auto [world, renderer] = scene.registry.get<WorldTransformComponent, MeshRendererComponent>(entity);
                    if (!renderer.model) continue;

                    glm::mat4 modelMatrix = glm::mat4(1.0f);
                    for (int r = 0; r < 4; ++r) {
                        for (int c = 0; c < 4; ++c) {
                            modelMatrix[r][c] = glm::mix(world.prevWorldMatrix[r][c], world.worldMatrix[r][c], alpha);
                        }
                    }

                    res.push_back({entity, renderer.model->aabb.Transform(modelMatrix)});
                }
            }, &counter);
        }
        JobSystem::Instance().Wait(&counter);

        std::vector<OctreeElement> elements;
        for (const auto& res : threadElements) elements.insert(elements.end(), res.begin(), res.end());
        
        scene.GetOctree()->Rebuild(elements);
        
        if (frustumCullEnabled) {
            scene.GetOctree()->Query(m_Frustum, outVisibleEntities);
        } else {
            for (const auto& el : elements) outVisibleEntities.push_back(el.entity);
        }
    } else {
        auto view = scene.registry.view<WorldTransformComponent, MeshRendererComponent>();
        for (auto entity : view) {
            outVisibleEntities.push_back(entity);
        }
    }
}
