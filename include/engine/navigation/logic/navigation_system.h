#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_update_system.h>
#include <core/interface/i_optimization_configurable.h>
#include <navigation/interface/i_navigation_service.h>
#include <navigation/unit/navmesh_component.h>
#include <navigation/unit/pathfollower_component.h>
#include <scene/logic/scene.h>
#include <algorithm>
#include <future>
#include <deque>
#include <unordered_map>

class NavigationSystem : public IUpdateSystem,
                         public IRenderSystem,
                         public IECSSystem,
                         public INavigationService,
                         public IOptimizationConfigurable
{
public:
    void Initialize() override;
    void Shutdown() override;
    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    void SetSpatialHashConfig(bool enabled, float agentCellSize)
    {
        m_SpatialHashEnabled = enabled;
        m_AgentCellSize = (std::max)(0.01f, agentCellSize);
    }
    void SetAsyncPathfindingConfig(bool enabled, int maxRequestsPerFrame)
    {
        m_AsyncPathfindingEnabled = enabled;
        m_MaxPathRequestsPerFrame = (std::max)(1, maxRequestsPerFrame);
    }
    int GetPriority() const override
    {
        return 5;
    }
    std::string GetName() const override
    {
        return "NavigationSystem";
    }

    NavigationSystem() = default;
    ~NavigationSystem() = default;

    void Update(Scene& scene, float dt) override;
    void Render(Scene& scene) override;
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    void AddWalkableTag(const std::string& tag) override;
    void ClearWalkableTags() override;
    std::vector<std::string> GetWalkableTags() const override
    {
        return m_WalkableTags;
    }

    void SetShowDebug(bool show) override
    {
        m_ShowDebug = show;
    }
    bool IsShowDebug() const override
    {
        return m_ShowDebug;
    }

    void AddCarveTag(const std::string& tag) override;
    void ClearCarveTags() override;
    std::vector<std::string> GetCarveTags() const override
    {
        return m_CarveTags;
    }

    void StopMoving(Scene& scene, entt::entity entity) override;
    bool IsMoving(Scene& scene, entt::entity entity) override;
    void SetMoveSpeed(Scene& scene, entt::entity entity, float speed) override;
    float GetRemainingDistance(Scene& scene, entt::entity entity) override;
    void MoveTo(Scene& scene, entt::entity entity, const glm::vec3& position) override;
    bool HasTarget(Scene& scene, entt::entity entity) override;

    void SetPathfindingCriteria(Scene& scene, entt::entity entity, PathfindingCriteria criteria) override;
    void SetPreferredTags(Scene& scene, entt::entity entity, const std::vector<std::string>& tags) override;
    void SetCustomCostFunction(Scene& scene, entt::entity entity,
                               std::function<float(uint32_t, uint32_t, const NavMeshComponent&)> func);
    void SetCustomGridCostFunction(Scene& scene, entt::entity entity,
                                   std::function<float(uint32_t, uint32_t, const NavigationGridComponent&)> func);
    void SetNavigationProviderEntity(Scene& scene, entt::entity entity, entt::entity provider,
                                     NavigationProvider providerType = NavigationProvider::Auto) override;
    bool MarkNavMeshDirty(Scene& scene, entt::entity provider, const glm::vec3& minimum,
                          const glm::vec3& maximum) override;

private:
    void UpdatePathFollowing(Scene& scene, float dt);
    void UpdateNavMesh(Scene& scene, float dt);

    bool m_Enabled = true;
    bool m_ShowDebug = false;
    bool m_SpatialHashEnabled = true;
    float m_AgentCellSize = 2.0f;
    bool m_AsyncPathfindingEnabled = true;
    int m_MaxPathRequestsPerFrame = 4;
    bool m_NavMeshRebuildBudgetEnabled = true;
    int m_MaxNavMeshRebuildsPerFrame = 1;
    bool m_NavMeshDirtyTilesEnabled = true;
    float m_NavMeshTileSize = 8.0f;
    int m_MaxNavMeshDirtyTilesPerFrame = 4;
    struct DirtyBounds
    {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};
    };
    struct NavMeshRuntime
    {
        std::vector<NavMeshTriangle> uncarvedTriangles;
        std::unordered_map<entt::entity, DirtyBounds> obstacles;
        std::deque<DirtyBounds> dirtyTiles;
    };
    Scene* m_RuntimeScene = nullptr;
    std::unordered_map<entt::entity, NavMeshRuntime> m_NavMeshRuntime;
    bool QueueDirtyBounds(NavMeshRuntime& runtime, const DirtyBounds& bounds);
    struct PendingPathRequest
    {
        uint64_t generation = 0;
        entt::entity navMeshProvider = entt::null;
        uint64_t navMeshRevision = 0;
        std::future<std::vector<glm::vec3>> result;
    };
    std::unordered_map<entt::entity, PendingPathRequest> m_PendingPaths;
    std::vector<std::string> m_WalkableTags = {"walkable"};
    std::vector<std::string> m_CarveTags = {"obstacle"};
};
