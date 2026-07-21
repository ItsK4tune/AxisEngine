#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_update_system.h>
#include <core/interface/i_optimization_configurable.h>
#include <core/logic/event_manager.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

class StreamingSystem : public IUpdateSystem, public IECSSystem, public IOptimizationConfigurable
{
public:
    StreamingSystem() : IBaseSystem()
    {
    }
    virtual void Initialize() override;
    virtual void Shutdown() override;
    virtual void Update(Scene& scene, float dt) override;
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    virtual bool IsEnabled() const override
    {
        return m_Enabled;
    }
    virtual void SetEnabled(bool enabled) override
    {
        m_Enabled = enabled;
    }
    void SetUpdateThrottling(bool enabled, float intervalSeconds)
    {
        m_UpdateThrottlingEnabled = enabled;
        m_CheckInterval = (std::max)(0.0f, intervalSeconds);
        m_Timer = 0.0f;
    }
    virtual int GetPriority() const override
    {
        return 12;
    }
    virtual std::string GetName() const override
    {
        return "StreamingSystem";
    }

private:
    struct Residency
    {
        size_t references = 0;
    };

    static std::string MakeResourceName(const std::string& modelPath, bool isStatic);
    void Release(entt::entity entity, ResourceManager& resources);

    bool m_Enabled = true;
    bool m_UpdateThrottlingEnabled = true;
    float m_CheckInterval = 1.0f;
    float m_Timer = 0.0f;
    std::unordered_map<std::string, Residency> m_Residencies;
    std::unordered_map<entt::entity, std::string> m_TrackedEntities;
    std::unordered_set<std::string> m_FailedResources;
    EventSubscriptionList m_EventSubscriptions;
};
