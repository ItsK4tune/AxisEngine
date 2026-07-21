#pragma once

#include <navigation/unit/pathfollower_component.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Scene;

class INavigationService
{
public:
    virtual ~INavigationService() = default;
    virtual void AddWalkableTag(const std::string& tag) = 0;
    virtual void ClearWalkableTags() = 0;
    virtual std::vector<std::string> GetWalkableTags() const = 0;
    virtual void AddCarveTag(const std::string& tag) = 0;
    virtual void ClearCarveTags() = 0;
    virtual std::vector<std::string> GetCarveTags() const = 0;
    virtual void SetShowDebug(bool show) = 0;
    virtual bool IsShowDebug() const = 0;
    virtual void StopMoving(Scene& scene, entt::entity entity) = 0;
    virtual bool IsMoving(Scene& scene, entt::entity entity) = 0;
    virtual void SetMoveSpeed(Scene& scene, entt::entity entity, float speed) = 0;
    virtual float GetRemainingDistance(Scene& scene, entt::entity entity) = 0;
    virtual void MoveTo(Scene& scene, entt::entity entity, const glm::vec3& position) = 0;
    virtual bool HasTarget(Scene& scene, entt::entity entity) = 0;
    virtual void SetPathfindingCriteria(Scene& scene, entt::entity entity, PathfindingCriteria criteria) = 0;
    virtual void SetPreferredTags(Scene& scene, entt::entity entity, const std::vector<std::string>& tags) = 0;
    virtual void SetNavigationProviderEntity(Scene& scene, entt::entity entity, entt::entity provider,
                                             NavigationProvider providerType = NavigationProvider::Auto) = 0;
    // Marks a world-space XZ region for incremental rebuilding. The default
    // preserves source compatibility for custom providers that do not expose
    // a tiled nav-mesh implementation.
    virtual bool MarkNavMeshDirty(Scene&, entt::entity, const glm::vec3&, const glm::vec3&)
    {
        return false;
    }
};
