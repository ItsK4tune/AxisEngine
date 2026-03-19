#pragma once

#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>
#include <navigation/unit/navmesh_component.h>
#include <navigation/unit/pathfollower_component.h>

class NavigationSystem : public IUpdateSystem, public IRenderSystem
{
public:
    void Initialize() override { }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 5; } // Before transform
    std::string GetName() const override { return "NavigationSystem"; }
    
    NavigationSystem() = default;
    ~NavigationSystem() = default;

    void Update(Scene& scene, float dt) override;
    void Render(Scene& scene) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    void AddWalkableTag(const std::string& tag);
    void ClearWalkableTags();
    const std::vector<std::string>& GetWalkableTags() const { return m_WalkableTags; }

    void SetShowDebug(bool show) { m_ShowDebug = show; }
    bool IsShowDebug() const { return m_ShowDebug; }

    void AddCarveTag(const std::string& tag);
    void ClearCarveTags();
    const std::vector<std::string>& GetCarveTags() const { return m_CarveTags; }

    // Management APIs
    void StopMoving(Scene& scene, entt::entity entity);
    bool IsMoving(Scene& scene, entt::entity entity);
    void SetMoveSpeed(Scene& scene, entt::entity entity, float speed);
    float GetRemainingDistance(Scene& scene, entt::entity entity);
    void MoveTo(Scene& scene, entt::entity entity, const glm::vec3& position);
    bool HasTarget(Scene& scene, entt::entity entity);

    // Pathfinding Preference APIs
    void SetPathfindingCriteria(Scene& scene, entt::entity entity, PathfindingCriteria criteria);
    void SetPreferredTags(Scene& scene, entt::entity entity, const std::vector<std::string>& tags);
    void SetCustomCostFunction(Scene& scene, entt::entity entity, std::function<float(uint32_t, uint32_t, const NavMeshComponent&)> func);

private:
    void UpdatePathFollowing(Scene& scene, float dt);
    void UpdateNavMesh(Scene& scene);
    
    bool m_Enabled = true;
    bool m_ShowDebug = false;
    std::vector<std::string> m_WalkableTags = { "walkable" };
    std::vector<std::string> m_CarveTags = { "obstacle" };
};
