#pragma once

#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <render/unit/command_queue.h>
#include <render/logic/material_renderer.h>
#include <memory>

class TransparentSystem : public IRenderSystem, public IECSSystem
{
public:
    void Initialize() override;
    void Shutdown() override {}
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 85; }
    std::string GetName() const override { return "TransparentSystem"; }
    void Render(Scene &scene) override {}
    void RenderTransparent(Scene &scene, int width, int height, float alpha) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    CommandQueue m_CommandQueue;
    MaterialRenderer m_MaterialRenderer;
};
