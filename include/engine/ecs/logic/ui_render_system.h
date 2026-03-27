#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_ui_service.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <platform/logic/input_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <scene/logic/scene.h>

class UIRenderSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem, public IUIService
{
public:

    void Initialize() override;
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 90; }
    SystemCategory GetCategory() const override { return SystemCategory::RenderUI | SystemCategory::Update; }
    std::string GetName() const override { return "UIRenderSystem"; }
    void Update(Scene &scene, float dt) override;
    void Render(Scene &scene) override;
    void UpdateLayout(Scene &scene, float screenWidth, float screenHeight);
    void RenderUIPass(Scene &scene, float screenWidth, float screenHeight, IRenderStateManager& renderState) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
};