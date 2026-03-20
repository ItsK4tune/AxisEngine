#pragma once

#include <string>
#include <entt/entt.hpp>

class Scene;

struct SceneLoadedEvent { std::string path; };
struct SceneUnloadedEvent { std::string path; };
struct SceneChangedEvent { entt::registry* registry; Scene* scene; };
