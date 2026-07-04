#include <render/unit/frustum.h>

void Frustum::Update(const glm::mat4& vp)
{
    planes[0].normal.x = vp[0][3] + vp[0][0];
    planes[0].normal.y = vp[1][3] + vp[1][0];
    planes[0].normal.z = vp[2][3] + vp[2][0];
    planes[0].distance = vp[3][3] + vp[3][0];

    planes[1].normal.x = vp[0][3] - vp[0][0];
    planes[1].normal.y = vp[1][3] - vp[1][0];
    planes[1].normal.z = vp[2][3] - vp[2][0];
    planes[1].distance = vp[3][3] - vp[3][0];

    planes[2].normal.x = vp[0][3] + vp[0][1];
    planes[2].normal.y = vp[1][3] + vp[1][1];
    planes[2].normal.z = vp[2][3] + vp[2][1];
    planes[2].distance = vp[3][3] + vp[3][1];

    planes[3].normal.x = vp[0][3] - vp[0][1];
    planes[3].normal.y = vp[1][3] - vp[1][1];
    planes[3].normal.z = vp[2][3] - vp[2][1];
    planes[3].distance = vp[3][3] - vp[3][1];

    planes[4].normal.x = vp[0][3] + vp[0][2];
    planes[4].normal.y = vp[1][3] + vp[1][2];
    planes[4].normal.z = vp[2][3] + vp[2][2];
    planes[4].distance = vp[3][3] + vp[3][2];

    planes[5].normal.x = vp[0][3] - vp[0][2];
    planes[5].normal.y = vp[1][3] - vp[1][2];
    planes[5].normal.z = vp[2][3] - vp[2][2];
    planes[5].distance = vp[3][3] - vp[3][2];

    for (int i = 0; i < 6; ++i)
    {
        planes[i].Normalize();
    }
}

bool Frustum::IsBoxVisible(const glm::vec3& minBound, const glm::vec3& maxBound) const
{
    for (int i = 0; i < 6; ++i)
    {
        glm::vec3 pMax(
            planes[i].normal.x >= 0.0f ? maxBound.x : minBound.x,
            planes[i].normal.y >= 0.0f ? maxBound.y : minBound.y,
            planes[i].normal.z >= 0.0f ? maxBound.z : minBound.z
        );

        if (glm::dot(planes[i].normal, pMax) + planes[i].distance < 0.0f)
        {
            return false;
        }
    }
    return true;
}

int Frustum::ContainsBoxState(const glm::vec3& minBound, const glm::vec3& maxBound) const
{
    bool fullyInside = true;
    for (int i = 0; i < 6; ++i)
    {
        glm::vec3 pMax(
            planes[i].normal.x >= 0.0f ? maxBound.x : minBound.x,
            planes[i].normal.y >= 0.0f ? maxBound.y : minBound.y,
            planes[i].normal.z >= 0.0f ? maxBound.z : minBound.z
        );

        if (glm::dot(planes[i].normal, pMax) + planes[i].distance < 0.0f)
        {
            return 0; // Outside
        }

        glm::vec3 pMin(
            planes[i].normal.x >= 0.0f ? minBound.x : maxBound.x,
            planes[i].normal.y >= 0.0f ? minBound.y : maxBound.y,
            planes[i].normal.z >= 0.0f ? minBound.z : maxBound.z
        );

        if (glm::dot(planes[i].normal, pMin) + planes[i].distance < 0.0f)
        {
            fullyInside = false; // Intersects plane
        }
    }
    return fullyInside ? 2 : 1;
}
