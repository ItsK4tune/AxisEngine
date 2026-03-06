#pragma once

#include <entt/entt.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

struct ModelInstance
{
    glm::mat4 transform;
    entt::entity entity;
};
