#pragma once

#include <core/engine_context.h>
#include <entt/entt.hpp>
#include <map>
#include <string>
#include <vector>

class Scene;
class IPhysicsWorld;
class Application;

namespace SceneHandlers
{
    class SceneValidator
    {
    public:
        static void ValidateLights(Scene &scene);
        static void ValidateCamera(Scene &scene, EngineContext ctx);
        static void ValidatePhysicsSync(Scene &scene, IPhysicsWorld &phys);
        static void ValidateParentChildRelationships(
            Scene &scene,
            const std::map<entt::entity, std::vector<std::string>> &deferredChildren);
    };
}
