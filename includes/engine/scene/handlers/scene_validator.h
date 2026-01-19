#pragma once

#include <map>
#include <vector>
#include <string>
#include <entt/entt.hpp>

// Forward declarations
class Scene;
class PhysicsWorld;
class Application;

namespace SceneHandlers
{
    class SceneValidator
    {
    public:
        // Validate and auto-fix scene after loading
        static void ValidateLights(Scene& scene);
        static void ValidateCamera(Scene& scene, Application* app);
        static void ValidatePhysicsSync(Scene& scene, PhysicsWorld& phys);
        static void ValidateParentChildRelationships(
            Scene& scene,
            const std::map<entt::entity, std::vector<std::string>>& deferredChildren
        );
    };
}
