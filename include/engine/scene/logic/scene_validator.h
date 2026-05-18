#pragma once

#include <entt/entt.hpp>
#include <map>
#include <string>
#include <vector>

class Application;
class IPhysicsWorld;
struct Scene;

namespace SceneHandlers
{
class SceneValidator
{
public:
    static void ValidateLights(Scene& scene);
    static void ValidateCamera(Scene& scene);
    static void ValidatePhysicsSync(Scene& scene, IPhysicsWorld* phys);
    static void ValidateParentChildRelationships(
        Scene& scene, const std::map<entt::entity, std::vector<std::string>>& deferredChildren);
};
}  // namespace SceneHandlers
