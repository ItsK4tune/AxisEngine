#pragma once

#include <core/logic/event_manager.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_skybox_service.h>
#include <scene/logic/scene.h>

class IGraphicsContext;

class SkyboxRenderSystem : public IRenderSystem, public IECSSystem, public ISkyboxService
{
public:
    void Initialize() override;
    void Shutdown() override;
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
        return 84;
    }
    std::string GetName() const override
    {
        return "SkyboxRenderSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderAlpha;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    void RenderAlphaPass(Scene& scene, int width, int height, float alpha) override;
    void RenderAlphaPassWithCamera(Scene& scene, const glm::mat4& view, const glm::mat4& proj, int width, int height,
                                   uint32_t targetFBO = 0) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    IGraphicsContext* m_Context = nullptr;
    float m_Intensity = 1.0f;
    EventSubscriptionList m_EventSubscriptions;
    bool m_Enabled = true;

    struct
    {
        uint32_t mainFBO = 0;
    } m_LastFrameData;

    class IRenderService* m_RenderService = nullptr;
};
