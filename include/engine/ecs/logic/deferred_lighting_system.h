#pragma once

#include <ecs/interface/i_system.h>
#include <memory>

class DeferredLightingSystem : public IRenderSystem
{
public:
    void Initialize() override;
    void Shutdown() override {}
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 82; } // After Decals
    std::string GetName() const override { return "DeferredLightingSystem"; }
    void RenderAlpha(Scene &scene, int width, int height, float alpha) override;

    void Render(Scene &scene) override;

private:
    bool m_Enabled = true;
};
