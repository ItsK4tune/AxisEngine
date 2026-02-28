#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>

struct RayHit
{
    bool hasHit = false;
    glm::vec3 hitPoint = glm::vec3(0.0f);
    glm::vec3 hitNormal = glm::vec3(0.0f);
    float distance = 0.0f;
    entt::entity entity = entt::null;
};
