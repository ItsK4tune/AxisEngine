#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <render/logic/material_renderer.h>
#include <memory>

class TransparentSystem : public IRenderSystem, public IECSSystem
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
        return 85;
    }
    std::string GetName() const override
    {
        return "TransparentSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderAlpha | SystemCategory::RenderTransparent;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    void RenderAlphaPass(Scene& scene, int width, int height, float alpha) override;
    void RenderTransparentPass(Scene& scene, int width, int height, float alpha) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
};
