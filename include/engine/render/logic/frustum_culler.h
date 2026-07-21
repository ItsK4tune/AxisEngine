#pragma once

#include <render/unit/frustum.h>

class FrustumCuller
{
public:
    void BuildFrustum(const glm::mat4& viewProj);
    bool IsVisible(const glm::vec3& minBound, const glm::vec3& maxBound) const;
    const Frustum& GetFrustum() const { return m_Frustum; }

private:
    Frustum m_Frustum;
};
