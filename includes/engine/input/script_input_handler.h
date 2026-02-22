#pragma once

#include <scene/scene.h>
#include <app/application.h>
#include <ecs/component.h>

class ScriptInputHandler
{
public:
    static void HandleInput(ScriptComponent& script, Scene& scene, std::shared_ptr<Application> app, float dt, entt::entity entity);
};
