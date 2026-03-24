#pragma once

#include <glm/glm.hpp>
#include <physics/unit/ray.h>


Ray CalculateRay(const glm::vec2& screenPos, const glm::vec2& viewportSize, const glm::mat4& view, const glm::mat4& proj);


glm::vec3 AngleToDirection(float yaw, float pitch);