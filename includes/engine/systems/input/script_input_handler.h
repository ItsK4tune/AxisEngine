#pragma once

#include <core/engine_context.h>
#include <ecs/component.h>
#include <scene/scene.h>

class ScriptInputHandler
{
public:
    static void HandleInput(ScriptComponent& script, Scene& scene, EngineContext ctx, float dt, entt::entity entity);
};
