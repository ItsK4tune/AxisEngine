#pragma once

#include <scene/scene.h>
class Application;
#include <ecs/component.h>

class ScriptInputHandler
{
public:
    static void HandleInput(ScriptComponent& script, Scene& scene, std::shared_ptr<Application> app, float dt, entt::entity entity);
};
