#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_render_system.h>
#include <core/interface/i_optimization_configurable.h>
#include <core/type/app_config.h>
#include <platform/logic/input_manager.h>
#include <render/interface/i_render_state_manager.h>
#include <scene/logic/scene.h>
#include <vector>
#include <unordered_map>
#include <cstdint>

class UIRenderSystem : public IRenderSystem, public IECSSystem, public IOptimizationConfigurable
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
        return 90;
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::RenderUI;
    }
    SystemRequirement GetRequirements() const override
    {
        return SystemRequirement::Graphics;
    }
    std::string GetName() const override
    {
        return "UIRenderSystem";
    }
    void UpdateLayout(Scene& scene, float screenWidth, float screenHeight);
    void RenderUIPass(Scene& scene, float screenWidth, float screenHeight, IRenderStateManager& renderState) override;
    void ApplyOptimizationConfig(const OptimizationConfig& config) override
    {
        m_LayoutCacheEnabled = config.uiLayoutCacheEnabled;
        if (!m_LayoutCacheEnabled)
            m_UIOrderSignature = 0;
    }

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    std::vector<entt::entity> m_SortedEntities;
    std::vector<float> m_TextVertices;
    std::vector<uint32_t> m_TextCodepoints;
    std::vector<std::vector<uint32_t>> m_TextLines;
    std::unordered_map<entt::entity, glm::vec4> m_RectCache;
    uint64_t m_UIOrderSignature = 0;
    bool m_LayoutCacheEnabled = true;
};
