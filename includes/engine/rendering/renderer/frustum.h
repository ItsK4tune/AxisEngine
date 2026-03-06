#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <math/aabb.h>

struct Plane
{
    glm::vec3 normal;
    float distance;

    void Normalize()
    {
        float length = glm::length(normal);
        normal /= length;
        distance /= length;
    }
};

class Frustum
{
public:
    void Update(const glm::mat4 &viewProjection);
    bool IsBoxVisible(const glm::vec3 &minBound, const glm::vec3 &maxBound) const;

private:
    Plane planes[6];
};
