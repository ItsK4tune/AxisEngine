#include <graphics/renderer/frustum_culler.h>
#include <ecs/component.h>

void FrustumCuller::BuildFrustum(const glm::mat4& viewProj) {
    m_Frustum.Update(viewProj);
}

bool FrustumCuller::IsVisible(const glm::vec3& minBound, const glm::vec3& maxBound) const {
    return m_Frustum.IsBoxVisible(minBound, maxBound);
}

void FrustumCuller::Cull(Scene& scene, bool frustumCullEnabled, std::vector<entt::entity>& outVisibleEntities, float alpha) const {
    outVisibleEntities.clear();
    if (scene.GetOctree()) {
        std::vector<OctreeElement> elements;
        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : view) {
            auto [transform, renderer] = view.get<TransformComponent, MeshRendererComponent>(entity);
            if (!renderer.model) continue;
            glm::mat4 modelMatrix = transform.GetInterpolatedWorldMatrix(scene.registry, alpha);
            elements.push_back({entity, renderer.model->aabb.Transform(modelMatrix)});
        }
        scene.GetOctree()->Rebuild(elements);
        
        if (frustumCullEnabled) {
            scene.GetOctree()->Query(m_Frustum, outVisibleEntities);
        } else {
            for (const auto& el : elements) outVisibleEntities.push_back(el.entity);
        }
    } else {
        auto view = scene.registry.view<TransformComponent, MeshRendererComponent>();
        for (auto entity : view) {
            outVisibleEntities.push_back(entity);
        }
    }
}
