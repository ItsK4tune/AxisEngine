#pragma once

#include <ecs/interface/i_system.h>


#include <resource/manager/resource_manager.h>
#include <scene/logic/scene.h>

class StreamingSystem : public ISystem
{
public:
    StreamingSystem() : ISystem() {}
    virtual void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
    virtual void Update(Scene& scene, float dt) override;

    virtual bool IsEnabled() const override { return m_Enabled; }
    virtual void SetEnabled(bool enabled) override { m_Enabled = enabled; }
    virtual std::string GetName() const override { return "StreamingSystem"; }

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
    float m_CheckInterval = 1.0f;
    float m_Timer = 0.0f;
};
