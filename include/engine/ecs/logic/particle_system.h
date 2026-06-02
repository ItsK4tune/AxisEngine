#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_update_system.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>

class IGraphicsContext;

class ParticleSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
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
        return "ParticleSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::Update;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    void Update(Scene& scene, float dt) override;
    void Render(Scene& scene) override
    {
    }
    void RenderTransparentPass(Scene& scene, int width, int height, float alpha) override;
    void RenderParticles(Scene& scene, int width, int height, float alpha);

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    IGraphicsContext* m_Context = nullptr;
    unsigned int m_DefaultTexture = 0;
    bool m_Enabled = true;
};
