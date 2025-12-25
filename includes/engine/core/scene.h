#pragma once

#include <entt/entt.hpp>
#include <engine/ecs/component.h> 

class SceneManager;

struct Scene
{
    entt::registry registry;

    entt::entity createEntity();
    void destroyEntity(entt::entity entity, SceneManager* manager);
    
    entt::entity GetActiveCamera();
};