#pragma once

#include <ecs/interface/i_render_system.h>
#include <core/interface/i_optimization_configurable.h>
#include <scene/logic/scene.h>
#include <cstddef>
#include <algorithm>
#include <vector>
#include <unordered_set>

class IGraphicsContext;
class IRenderService;

class PlanarReflectionSystem : public IRenderSystem, public IOptimizationConfigurable
{
public:
    void Initialize() override;
    void Shutdown() override;
    void Reset() override;
    void Render(Scene& scene) override;
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;

    bool IsEnabled() const override
    {
        return m_Enabled;
    }
    void SetEnabled(bool enable) override
    {
        m_Enabled = enable;
    }
    void SetCaptureBudget(bool enabled, size_t maxCapturesPerFrame)
    {
        m_CaptureBudgetEnabled = enabled;
        m_MaxCapturesPerFrame = (std::max)(size_t{1}, maxCapturesPerFrame);
    }
    std::string GetName() const override
    {
        return "PlanarReflectionSystem";
    }
    int GetPriority() const override
    {
        return 45;
    }  // Before Lighting/Main render
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderMain;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }

private:
    void ReleaseSceneResources();

    bool m_Enabled = true;
    IGraphicsContext* m_Context = nullptr;
    IRenderService* m_RenderService = nullptr;
    size_t m_NextReflectionIndex = 0;
    size_t m_MaxCapturesPerFrame = 1;
    bool m_CaptureBudgetEnabled = true;
    std::vector<entt::entity> m_Candidates;
    std::unordered_set<uint32_t> m_MainViewVisibleEntities;
};
