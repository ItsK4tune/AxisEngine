#pragma once

#include <entt/entt.hpp>
#include <map>
#include <string>
#include <vector>

class IPhysicsWorld;
struct Scene;
struct SceneLoadResult;

namespace SceneHandlers
{
struct SceneLoadFinalizeOptions
{
    bool ensureFallbackCamera = true;
    bool initializePhysicsBindings = true;
    bool deepValidation = false;
};

class SceneLoadFinalizer
{
public:
    static bool Finalize(Scene& scene, SceneLoadResult& result, IPhysicsWorld* physics,
                         const std::map<entt::entity, std::vector<std::string>>& deferredChildren = {},
                         const SceneLoadFinalizeOptions& options = {});
};
}  // namespace SceneHandlers
