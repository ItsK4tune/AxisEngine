#pragma once

#include <ecs/interface/i_render_system.h>
#include <scene/logic/scene.h>

class IGraphicsContext;
struct AppConfig;

class SkyboxRenderSystem : public IRenderSystem
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
    const struct AppConfig* m_Config = nullptr;
    bool m_Enabled = true;
};