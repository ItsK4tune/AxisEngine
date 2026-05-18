#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_update_system.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>

class VideoSystem : public IUpdateSystem, public IECSSystem
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
        return 40;
    }
    std::string GetName() const override
    {
        return "VideoSystem";
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    void Update(Scene& scene, float dt) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    void OnVideoPlayerDestroyed(entt::registry& registry, entt::entity entity);

    bool m_Enabled = true;
};
