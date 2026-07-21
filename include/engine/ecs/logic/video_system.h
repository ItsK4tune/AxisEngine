#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_update_system.h>
#include <core/interface/i_optimization_configurable.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>

class VideoSystem : public IUpdateSystem, public IECSSystem, public IOptimizationConfigurable
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
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    void OnVideoPlayerDestroyed(entt::registry& registry, entt::entity entity);

    bool m_Enabled = true;
    Scene* m_BoundScene = nullptr;
    bool m_AsyncDecodeEnabled = true;
};
