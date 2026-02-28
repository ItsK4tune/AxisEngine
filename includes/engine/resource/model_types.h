#pragma once

#include <glm/glm.hpp>
#include <entt/entt.hpp>

struct ModelInstance
{
    glm::mat4 transform;
    entt::entity entity;
};
