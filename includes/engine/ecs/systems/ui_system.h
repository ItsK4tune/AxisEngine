#pragma once

#include <ecs/i_system.h>

#include <systems/input/mouse_manager.h>
#include <rendering/interfaces/i_render_state_manager.h>
#include <scene/scene.h>

class UIRenderSystem : public ISystem
{
public:

    void Init(EngineContext ctx) override { m_Ctx = ctx; }
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 90; }
    std::string GetName() const override { return "UIRenderSystem"; }
    void Render(Scene &scene) override;
    void RenderUI(Scene &scene, float screenWidth, float screenHeight, IRenderStateManager& renderState);

private:
    EngineContext m_Ctx;
    bool m_Enabled = true;
};
