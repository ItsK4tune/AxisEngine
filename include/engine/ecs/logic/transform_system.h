#pragma once

#include <ecs/interface/i_system.h>


#include <ecs/unit/core_components.h>
#include <scene/logic/scene.h>

class TransformSystem : public IUpdateSystem
{
public:
    TransformSystem() : IBaseSystem() {}
    virtual ~TransformSystem() = default;

    void Initialize() override;
    void Update(Scene& scene, float dt) override;
    void FixedUpdate(Scene& scene, float fixedDt) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enabled) override { m_Enabled = enabled; }

    std::string GetName() const override { return "TransformSystem"; }
    int GetPriority() const override { return 10; }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
};
