#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Ray
{
    glm::vec3 origin;
    glm::vec3 direction;
};

namespace RaycastUtils
{
    
    Ray CalculateRay(const glm::vec2& screenPos, const glm::vec2& viewportSize, const glm::mat4& view, const glm::mat4& proj);

    
    glm::vec3 AngleToDirection(float yaw, float pitch);
}