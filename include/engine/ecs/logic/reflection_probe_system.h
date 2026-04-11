#pragma once

#include <ecs/interface/i_update_system.h>
#include <ecs/interface/i_render_system.h>
#include <ecs/interface/i_ecs_system.h>
#include <render/interface/i_graphics_context.h>
#include <resource/unit/shader.h>
#include <memory>
#include <vector>

class ReflectionProbeSystem : public IUpdateSystem, public IRenderSystem, public IECSSystem
{
public:
    void Initialize() override;
    void Shutdown() override;
    void Update(Scene& scene, float dt) override;
    
    // IRenderSystem implementation
    void Render(Scene& scene) override {}
    void RenderCapturePass(Scene& scene, int width, int height) override;
    
    bool IsEnabled() const override { return m_Enabled; }
    void SetEnabled(bool enable) override { m_Enabled = enable; }
    int GetPriority() const override { return 70; } 
    std::string GetName() const override { return "ReflectionProbeSystem"; }
    SystemCategory GetCategory() const override { return SystemCategory::Update | SystemCategory::RenderCapture; }
    SystemRequirement GetRequirements() const override { return SystemRequirement::Graphics; }

private:
    void CaptureProbe(Scene& scene, entt::entity entity, int faceIndex);
    unsigned int CreateCubemap(int resolution);

    bool m_Enabled = true;
    uint32_t m_CaptureFBO = 0;
    uint32_t m_DepthRB = 0;
    std::shared_ptr<Shader> m_ProbeShader;
};
