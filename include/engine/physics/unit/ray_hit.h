#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

struct RayHit
{
    bool hasHit = false;
    glm::vec3 hitPoint = glm::vec3(0.0f);
    glm::vec3 hitNormal = glm::vec3(0.0f);
    float distance = 0.0f;
    entt::entity entity = entt::null;
};
