#pragma once

#include <engine/core/scene.h>

class SceneHelper
{
public:
    template <typename T>
    static T *GetScriptInstance(Scene *scene, entt::entity entity)
    {
        if (!scene->registry.valid(entity))
            return nullptr;
        if (scene->registry.all_of<ScriptComponent>(entity))
        {
            auto &sc = scene->registry.get<ScriptComponent>(entity);
            return dynamic_cast<T *>(sc.instance);
        }
        return nullptr;
    }
};