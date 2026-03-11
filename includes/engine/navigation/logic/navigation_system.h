#pragma once

#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>
#include <navigation/unit/navmesh_component.h>
#include <navigation/unit/pathfollower_component.h>

class NavigationSystem : public ISystem
{
public:
    void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 5; } // Before transform
    std::string GetName() const override { return "NavigationSystem"; }

    NavigationSystem() = default;
    ~NavigationSystem() = default;

    void Update(Scene& scene, float dt) override;
    void Render(Scene& scene) override;

    void SetWalkableTag(const std::string& tag) { m_WalkableTag = tag; }
    const std::string& GetWalkableTag() const { return m_WalkableTag; }

    void SetShowDebug(bool show) { m_ShowDebug = show; }
    bool IsShowDebug() const { return m_ShowDebug; }

private:
    void UpdatePathFollowing(Scene& scene, float dt);
    void UpdateNavMesh(Scene& scene);
    
    EngineContext m_Ctx;
    bool m_Enabled = true;
    bool m_ShowDebug = false;
    std::string m_WalkableTag = "Walkable";
};
