#pragma once

#include <ecs/interface/i_system.h>
#include <platform/logic/input_system.h>
#include <render/interface/i_render_state_manager.h>
#include <scene/logic/scene.h>

class UIRenderSystem : public ISystem
{
public:

    void Initialize(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 90; }
    std::string GetName() const override { return "UIRenderSystem"; }
    void Render(Scene &scene) override;
    void RenderUI(Scene &scene, float screenWidth, float screenHeight, IRenderStateManager& renderState);

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
};