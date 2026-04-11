#pragma once

#include <ecs/interface/i_render_system.h>
#include <scene/logic/scene.h>

class IGraphicsContext;
class IRenderService;

class PlanarReflectionSystem : public IRenderSystem
{
public:
    void Initialize() override;
    void Shutdown() override;
    void Render(Scene& scene) override;

    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    std::string GetName() const override { return "PlanarReflectionSystem"; }
    int GetPriority() const override { return 45; } // Before Lighting/Main render
    SystemCategory GetCategory() const override { return SystemCategory::RenderMain; }
    SystemRequirement GetRequirements() const override { return SystemRequirement::Graphics; }

private:
    bool m_Enabled = true;
    IGraphicsContext* m_Context = nullptr;
    IRenderService* m_RenderService = nullptr;
};
