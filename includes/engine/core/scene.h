#pragma once

#include <entt/entt.hpp>
#include <engine/ecs/component.h> 

struct Scene
{
    entt::registry registry;

    entt::entity createEntity();
    void destroyEntity(entt::entity entity);
    
    entt::entity GetActiveCamera();
};