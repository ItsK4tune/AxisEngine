#pragma once

#include <ecs/interface/i_ecs_system.h>
#include <ecs/interface/i_parallel_update_system.h>
#include <core/interface/i_optimization_configurable.h>
#include <scene/logic/scene.h>
#include <algorithm>
#include <cstddef>

class AnimationSystem : public IParallelUpdateSystem, public IECSSystem, public IOptimizationConfigurable
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
    void SetParallelEvaluationConfig(bool enabled, size_t threshold)
    {
        m_ParallelEvaluationEnabled = enabled;
        m_ParallelThreshold = (std::max)(size_t{1}, threshold);
    }
    int GetPriority() const override
    {
        return 50;
    }
    std::string GetName() const override
    {
        return "AnimationSystem";
    }
    void Update(Scene& scene, float dt) override;
    void CaptureSnapshot(Scene& scene, FrameSnapshot& snapshot) override;
    void UpdateParallel(const FrameSnapshot& snapshot, ECSCommandBuffer& commands, float dt) override;
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;

    std::vector<entt::id_type> GetReadComponents() const override;
    std::vector<entt::id_type> GetWriteComponents() const override;

private:
    bool m_Enabled = true;
    bool m_ParallelEvaluationEnabled = true;
    size_t m_ParallelThreshold = 64;
};
