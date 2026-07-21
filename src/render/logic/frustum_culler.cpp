#include <render/logic/frustum_culler.h>

void FrustumCuller::BuildFrustum(const glm::mat4& viewProj)
{
    m_Frustum.Update(viewProj);
}

bool FrustumCuller::IsVisible(const glm::vec3& minBound, const glm::vec3& maxBound) const
{
    return m_Frustum.IsBoxVisible(minBound, maxBound);
}
