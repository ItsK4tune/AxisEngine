#pragma once

#include <ecs/interface/i_system.h>
#include <memory>

class DeferredLightingSystem : public ISystem
{
public:
    void Initialize(EngineContext ctx) override;
    void Shutdown() override {}
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 85; } // After Decals
    std::string GetName() const override { return "DeferredLightingSystem"; }

    void Render(Scene &scene) override;

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
};
