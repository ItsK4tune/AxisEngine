#pragma once

#include <glm/glm.hpp>

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

#include <math/aabb.h>

class Frustum
{
public:
    void Update(const glm::mat4 &viewProjection);
    bool IsBoxVisible(const glm::vec3 &minBound, const glm::vec3 &maxBound) const;

private:
    Plane planes[6];
};
