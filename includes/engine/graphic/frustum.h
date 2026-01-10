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

struct AABB
{
    glm::vec3 min;
    glm::vec3 max;
};

class Frustum
{
public:
    void Update(const glm::mat4 &viewProjection);
    bool IsBoxVisible(const glm::vec3 &min, const glm::vec3 &max) const;

private:
    Plane planes[6];
};
