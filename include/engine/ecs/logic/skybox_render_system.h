#pragma once

#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <scene/logic/scene.h>

class IGraphicsContext;

class SkyboxRenderSystem : public IRenderSystem, public IECSSystem
{
public:

    void Initialize() override;
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 83; }
    std::string GetName() const override { return "SkyboxRenderSystem"; }
    void Render(Scene &scene) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    IGraphicsContext* m_Context = nullptr;
    float m_Intensity = 1.0f;
    uint32_t m_ConfigSubId = 0;
    uint32_t m_SceneSubId = 0;
    bool m_Enabled = true;
};
