#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <vector>

struct Scene;

class CameraSystem : public IUpdateSystem, public IECSSystem
{
public:
    void Initialize() override;
    void Shutdown() override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 10; } // Early update
    std::string GetName() const override { return "CameraSystem"; }
    SystemCategory GetCategory() const override { return SystemCategory::Update; }

    void Update(Scene& scene, float dt) override;
    void FixedUpdate(Scene& scene, float dt) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
};
