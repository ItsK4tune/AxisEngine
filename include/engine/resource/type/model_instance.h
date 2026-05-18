#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

struct ModelInstance
{
    glm::mat4 transform;
    entt::entity entity;
};
