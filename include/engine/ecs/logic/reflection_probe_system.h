#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <core/interface/i_optimization_configurable.h>
#include <render/interface/i_graphics_context.h>
#include <resource/unit/shader.h>
#include <memory>
#include <algorithm>
#include <vector>

class ReflectionProbeSystem : public IRenderSystem, public IECSSystem, public IOptimizationConfigurable
{
public:
    void Initialize() override;
    void Shutdown() override;
    void Reset() override;
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;

    // IRenderSystem implementation
    void RenderCapturePass(Scene& scene, int width, int height) override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    void SetCaptureBudget(bool enabled, size_t maxFacesPerFrame)
    {
        m_CaptureBudgetEnabled = enabled;
        m_MaxFacesPerFrame = (std::max)(size_t{1}, maxFacesPerFrame);
    }
    int GetPriority() const override
    {
        return 70;
    }
    std::string GetName() const override
    {
        return "ReflectionProbeSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderCapture;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }

private:
    bool CaptureProbe(Scene& scene, entt::entity entity, int faceIndex, int viewportWidth, int viewportHeight);
    unsigned int CreateCubemap(int resolution);

    bool m_Enabled = true;
    uint32_t m_CaptureFBO = 0;
    uint32_t m_DepthRB = 0;
    int m_DepthResolution = 0;
    size_t m_NextProbeIndex = 0;
    size_t m_MaxFacesPerFrame = 2;
    bool m_CaptureBudgetEnabled = true;
    std::vector<entt::entity> m_Candidates;
};
