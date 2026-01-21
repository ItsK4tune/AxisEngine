#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <entt/entt.hpp>

class Scene;

namespace SceneHandlers
{
    class EntityCommandHandler
    {
    public:
        static entt::entity HandleNewEntity(std::stringstream &ss, Scene &scene);
        static void HandleTransform(std::stringstream &ss, Scene &scene, entt::entity entity);
        static void HandleParent(std::stringstream &ss, Scene &scene, entt::entity entity);
        static void HandleChildren(
            std::stringstream &ss,
            entt::entity entity,
            std::map<entt::entity, std::vector<std::string>> &deferredChildren);
    };
}
