#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_update_system.h>
#include <scene/logic/scene.h>

class AnimationSystem : public IUpdateSystem, public IECSSystem
{
public:
    void Initialize() override;
    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    int GetPriority() const override
    {
        return 50;
    }
    std::string GetName() const override
    {
        return "AnimationSystem";
    }
    void Update(Scene& scene, float dt) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    std::vector<entt::entity> m_Entities;
};
