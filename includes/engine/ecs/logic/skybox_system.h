#pragma once

#include <ecs/interface/i_system.h>
#include <scene/logic/scene.h>

class IGraphicsContext;
struct AppConfig;

class SkyboxRenderSystem : public ISystem
{
public:

    void Initialize(EngineContext ctx) override;
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 81; }
    std::string GetName() const override { return "SkyboxRenderSystem"; }
    void Render(Scene &scene) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    EngineContext m_Ctx;
    IGraphicsContext* m_Context = nullptr;
    bool m_Enabled = true;
    const AppConfig* m_Config = nullptr;
};