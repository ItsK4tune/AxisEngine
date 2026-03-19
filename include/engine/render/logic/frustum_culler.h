#pragma once

#include <render/unit/frustum.h>
#include <scene/logic/scene.h>
#include <vector>

class FrustumCuller {
public:
    void BuildFrustum(const glm::mat4& viewProj);
    bool IsVisible(const glm::vec3& minBound, const glm::vec3& maxBound) const;
    bool IsVisible(Scene& scene, entt::entity entity, float alpha) const;
    void Cull(Scene& scene, bool frustumCullEnabled, std::vector<entt::entity>& outVisibleEntities, float alpha) const;

private:
    Frustum m_Frustum;
};