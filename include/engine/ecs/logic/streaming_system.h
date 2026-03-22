#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_ecs_system.h>


#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>

class StreamingSystem : public IUpdateSystem, public IECSSystem
{
public:
    StreamingSystem() : IBaseSystem() {}
    virtual void Initialize() override {}
    virtual void Update(Scene& scene, float dt) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

    virtual bool IsEnabled() const override { return m_Enabled; }
    virtual void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    virtual int GetPriority() const override { return 12; }
    virtual std::string GetName() const override { return "StreamingSystem"; }

private:
    bool m_Enabled = true;
    float m_CheckInterval = 1.0f;
    float m_Timer = 0.0f;
};
