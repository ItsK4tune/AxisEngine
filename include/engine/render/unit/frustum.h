#pragma once

#include <core/unit/aabb.h>
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

class Frustum
{
public:
    void Update(const glm::mat4& viewProjection);
    bool IsBoxVisible(const glm::vec3& minBound, const glm::vec3& maxBound) const;
    bool IsBoxVisible(const AABB& aabb) const
    {
        return IsBoxVisible(aabb.minBound, aabb.maxBound);
    }
    int ContainsBoxState(const glm::vec3& minBound, const glm::vec3& maxBound) const;
    int ContainsBoxState(const AABB& aabb) const
    {
        return ContainsBoxState(aabb.minBound, aabb.maxBound);
    }

private:
    Plane planes[6];
};
