#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL

namespace RaycastUtils
{
    struct Ray
    {
        glm::vec3 origin;
        glm::vec3 direction;
    };

    /**
     * @brief Calculates a world-space ray from screen-space coordinates.
     */
    Ray CalculateRay(const glm::vec2& screenPos, const glm::vec2& viewportSize, const glm::mat4& view, const glm::mat4& proj);

    /**
     * @brief Converts yaw and pitch angles into a normalized direction vector.
     */
    glm::vec3 AngleToDirection(float yaw, float pitch);
}