#include <graphic/renderer/frustum.h>

void Frustum::Update(const glm::mat4 &vp)
{
    // Left
    planes[0].normal.x = vp[0][3] + vp[0][0];
    planes[0].normal.y = vp[1][3] + vp[1][0];
    planes[0].normal.z = vp[2][3] + vp[2][0];
    planes[0].distance = vp[3][3] + vp[3][0];

    // Right
    planes[1].normal.x = vp[0][3] - vp[0][0];
    planes[1].normal.y = vp[1][3] - vp[1][0];
    planes[1].normal.z = vp[2][3] - vp[2][0];
    planes[1].distance = vp[3][3] - vp[3][0];

    // Bottom
    planes[2].normal.x = vp[0][3] + vp[0][1];
    planes[2].normal.y = vp[1][3] + vp[1][1];
    planes[2].normal.z = vp[2][3] + vp[2][1];
    planes[2].distance = vp[3][3] + vp[3][1];

    // Top
    planes[3].normal.x = vp[0][3] - vp[0][1];
    planes[3].normal.y = vp[1][3] - vp[1][1];
    planes[3].normal.z = vp[2][3] - vp[2][1];
    planes[3].distance = vp[3][3] - vp[3][1];

    // Near
    planes[4].normal.x = vp[0][3] + vp[0][2];
    planes[4].normal.y = vp[1][3] + vp[1][2];
    planes[4].normal.z = vp[2][3] + vp[2][2];
    planes[4].distance = vp[3][3] + vp[3][2];

    // Far
    planes[5].normal.x = vp[0][3] - vp[0][2];
    planes[5].normal.y = vp[1][3] - vp[1][2];
    planes[5].normal.z = vp[2][3] - vp[2][2];
    planes[5].distance = vp[3][3] - vp[3][2];

    for (int i = 0; i < 6; ++i)
    {
        planes[i].Normalize();
    }
}

bool Frustum::IsBoxVisible(const glm::vec3 &min, const glm::vec3 &max) const
{
    for (int i = 0; i < 6; ++i)
    {
        glm::vec3 p = min;
        if (planes[i].normal.x >= 0)
            p.x = max.x;
        if (planes[i].normal.y >= 0)
            p.y = max.y;
        if (planes[i].normal.z >= 0)
            p.z = max.z;

        if (glm::dot(planes[i].normal, p) + planes[i].distance < 0)
        {
            return false;
        }
    }
    return true;
}
