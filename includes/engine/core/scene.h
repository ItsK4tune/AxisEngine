#pragma once

#include <entt/entt.hpp>
#include <ecs/component.h> 

class SceneManager;

struct Scene
{
    entt::registry registry;

    entt::entity createEntity();
    void destroyEntity(entt::entity entity, class SceneManager* manager = nullptr);
    
    entt::entity GetActiveCamera();
};