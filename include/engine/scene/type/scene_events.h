#pragma once

#include <entt/entt.hpp>
#include <string>

struct Scene;

struct SceneLoadedEvent
{
    std::string path;
};
struct SceneUnloadedEvent
{
    std::string path;
};
struct SceneChangedEvent
{
    entt::registry* registry;
    Scene* scene;
};
