#include <render/logic/frustum_culler.h>
#include <core/logic/job_system.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <algorithm>

void FrustumCuller::BuildFrustum(const glm::mat4& viewProj)
{
    m_Frustum.Update(viewProj);
}

bool FrustumCuller::IsVisible(const glm::vec3& minBound, const glm::vec3& maxBound) const
{
    return m_Frustum.IsBoxVisible(minBound, maxBound);
}

bool FrustumCuller::IsVisible(Scene& scene, entt::entity entity, float alpha) const
{
    if (!scene.HasAllComponents<WorldTransformComponent, MeshRendererComponent>(entity))
        return false;
    auto [world, renderer] = scene.GetComponent<WorldTransformComponent, MeshRendererComponent>(entity);
    if (!renderer.model)
        return false;

    glm::mat4 modelMatrix = world.GetInterpolated(alpha);
    AABB transformedAABB = renderer.model->aabb.Transform(modelMatrix);
    return IsVisible(transformedAABB.minBound, transformedAABB.maxBound);
}

void FrustumCuller::Cull(Scene& scene, bool frustumCullEnabled, std::vector<entt::entity>& outVisibleEntities,
                         float alpha) const
{
    outVisibleEntities.clear();

    auto renderView = scene.View<WorldTransformComponent, MeshRendererComponent>();

    outVisibleEntities.reserve(renderView.size_hint());

    for (auto entity : renderView)
    {
        auto& renderer = renderView.get<MeshRendererComponent>(entity);
        if (!renderer.model)
        {
            outVisibleEntities.push_back(entity);
            continue;
        }

        if (!frustumCullEnabled)
        {
            outVisibleEntities.push_back(entity);
            continue;
        }

        auto& world = renderView.get<WorldTransformComponent>(entity);
        glm::mat4 modelMatrix = (alpha >= 0.99f) ? world.worldMatrix : world.GetInterpolated(alpha);

        bool isFinite = true;
        for (int c = 0; c < 4; c++)
        {
            for (int r = 0; r < 4; r++)
            {
                if (!std::isfinite(modelMatrix[c][r]))
                {
                    isFinite = false;
                    break;
                }
            }
            if (!isFinite)
                break;
        }
        if (!isFinite)
            continue;

        AABB transformedAABB = renderer.model->aabb.Transform(modelMatrix);

        if (IsVisible(transformedAABB.minBound, transformedAABB.maxBound))
        {
            outVisibleEntities.push_back(entity);
        }
    }
}
