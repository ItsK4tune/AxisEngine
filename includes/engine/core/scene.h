#pragma once

#include <entt/entt.hpp>
#include <engine/ecs/component.h> 

class Application;

struct Scene
{
    entt::registry registry;

    entt::entity createEntity();
    void destroyEntity(entt::entity entity, Application* app = nullptr);
    
    entt::entity GetActiveCamera();
};