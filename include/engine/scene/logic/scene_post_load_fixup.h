#pragma once

#include <entt/entt.hpp>
#include <map>
#include <string>
#include <vector>

class IPhysicsWorld;
struct Scene;

namespace SceneHandlers
{
class ScenePostLoadFixup
{
public:
    static void ResolveParentChildRelationships(
        Scene& scene, const std::map<entt::entity, std::vector<std::string>>& deferredChildren);
    static bool EnsureFallbackCamera(Scene& scene);
    static void InitializePhysicsBindings(Scene& scene, IPhysicsWorld* physics,
                                          const std::vector<entt::entity>& entityScope = {});
};
}  // namespace SceneHandlers
