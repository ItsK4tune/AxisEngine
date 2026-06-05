#pragma once

#include <core/logic/event_manager.h>
#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_shadow_service.h>
#include <render/logic/shadow_renderer.h>
#include <memory>

class ShadowSystem : public IRenderSystem, public IECSSystem, public IShadowService
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
        return 75;
    }
    std::string GetName() const override
    {
        return "ShadowSystem";
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }

    void Render(Scene& scene) override;
    void PrepareShadowLights(Scene& scene) override;

    Shadow& GetShadow()
    {
        return m_ShadowRenderer.GetShadow();
    }
    ShadowRenderer& GetRenderer()
    {
        return m_ShadowRenderer;
    }

    void SetEnableShadows(bool enable)
    {
        m_ShadowRenderer.SetEnableShadows(enable);
    }
    void SetShadowMode(int mode)
    {
        m_ShadowRenderer.SetShadowMode(mode);
    }
    bool IsShadowsEnabled() const
    {
        return m_ShadowRenderer.IsShadowsEnabled();
    }
    int GetShadowMode() const
    {
        return m_ShadowRenderer.GetShadowMode();
    }

    void SetShadowBias(float bias)
    {
        m_ShadowRenderer.SetShadowBias(bias);
    }
    void SetShadowSoftness(int softness)
    {
        m_ShadowRenderer.SetShadowSoftness(softness);
    }

    void SetShadowProjectionSize(float size)
    {
        m_ShadowRenderer.SetShadowProjectionSize(size);
    }
    void SetShadowFrustumCulling(bool enable)
    {
        m_ShadowRenderer.SetShadowFrustumCulling(enable);
    }
    void SetShadowDistanceCulling(float distance)
    {
        m_ShadowRenderer.SetShadowDistanceCulling(distance);
    }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    ShadowRenderer m_ShadowRenderer;
    EventSubscriptionList m_EventSubscriptions;
};
