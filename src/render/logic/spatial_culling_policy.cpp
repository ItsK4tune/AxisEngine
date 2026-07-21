#include <render/logic/spatial_culling_policy.h>
#include <algorithm>
#include <cmath>

namespace
{
constexpr uint32_t LinearWarmupSamples = 8;
constexpr uint32_t ProbeIntervalFrames = 120;
constexpr float MovingAverageWeight = 0.15f;
constexpr float SwitchAdvantage = 0.90f;
constexpr float ComparableCost = 1.05f;
constexpr float HighDirtyRatio = 0.30f;
constexpr float UsefulCandidateRatio = 0.65f;
constexpr float IneffectiveCandidateRatio = 0.90f;
}  // namespace

void SpatialCullingPolicy::SetMode(SpatialCullingMode mode)
{
    if (mode < SpatialCullingMode::Auto || mode > SpatialCullingMode::Octree)
        mode = SpatialCullingMode::Auto;
    if (m_Mode != mode)
    {
        m_Mode = mode;
        ResetMeasurements();
    }
}

SpatialCullingMode SpatialCullingPolicy::GetMode() const
{
    return m_Mode;
}

SpatialCullingMode SpatialCullingPolicy::GetAutoMode() const
{
    return m_AutoMode;
}

SpatialCullingMode SpatialCullingPolicy::Select(bool octreeAvailable, bool allowProbe) const
{
    if (!octreeAvailable)
        return SpatialCullingMode::Linear;
    if (m_Mode == SpatialCullingMode::Linear || m_Mode == SpatialCullingMode::Octree)
        return m_Mode;
    if (!allowProbe)
        return m_AutoMode;

    if (m_LinearSamples < LinearWarmupSamples)
        return SpatialCullingMode::Linear;
    if (m_OctreeSamples == 0)
        return m_DirtyRatio < HighDirtyRatio ? SpatialCullingMode::Octree : SpatialCullingMode::Linear;
    if (m_FramesSinceProbe >= ProbeIntervalFrames)
        return m_AutoMode == SpatialCullingMode::Linear ? SpatialCullingMode::Octree : SpatialCullingMode::Linear;
    return m_AutoMode;
}

void SpatialCullingPolicy::RecordSample(SpatialCullingMode executedMode, float costMs, size_t entityCount,
                                        size_t candidateCount, size_t dirtyEvents, bool ignoreTiming)
{
    if (m_Mode != SpatialCullingMode::Auto)
        return;

    const float dirtyRatio =
        entityCount > 0 ? std::min(static_cast<float>(dirtyEvents) / static_cast<float>(entityCount), 1.0f) : 0.0f;
    m_DirtyRatio += (dirtyRatio - m_DirtyRatio) * MovingAverageWeight;

    if (executedMode == SpatialCullingMode::Octree && entityCount > 0)
    {
        const float candidateRatio =
            std::min(static_cast<float>(candidateCount) / static_cast<float>(entityCount), 1.0f);
        if (m_OctreeWorkloadSamples == 0)
            m_OctreeCandidateRatio = candidateRatio;
        else
            m_OctreeCandidateRatio += (candidateRatio - m_OctreeCandidateRatio) * MovingAverageWeight;
        ++m_OctreeWorkloadSamples;
    }

    const bool wasProbe = executedMode != m_AutoMode;
    if (wasProbe)
        m_FramesSinceProbe = 0;
    else if (m_FramesSinceProbe < ProbeIntervalFrames)
        ++m_FramesSinceProbe;

    if (!ignoreTiming && std::isfinite(costMs) && costMs >= 0.0f)
    {
        if (executedMode == SpatialCullingMode::Octree)
            UpdateAverage(m_OctreeCostMs, m_OctreeSamples, costMs);
        else
            UpdateAverage(m_LinearCostMs, m_LinearSamples, costMs);
    }

    ReevaluateAutoMode();
}

void SpatialCullingPolicy::ResetMeasurements()
{
    m_AutoMode = SpatialCullingMode::Linear;
    m_LinearCostMs = 0.0f;
    m_OctreeCostMs = 0.0f;
    m_OctreeCandidateRatio = 1.0f;
    m_DirtyRatio = 0.0f;
    m_LinearSamples = 0;
    m_OctreeSamples = 0;
    m_OctreeWorkloadSamples = 0;
    m_FramesSinceProbe = 0;
}

SpatialCullingPolicyMetrics SpatialCullingPolicy::GetMetrics() const
{
    return {m_Mode,       m_AutoMode,      m_LinearCostMs, m_OctreeCostMs, m_OctreeCandidateRatio,
            m_DirtyRatio, m_LinearSamples, m_OctreeSamples};
}

void SpatialCullingPolicy::UpdateAverage(float& average, uint32_t& sampleCount, float value)
{
    if (sampleCount == 0)
        average = value;
    else
        average += (value - average) * MovingAverageWeight;
    ++sampleCount;
}

void SpatialCullingPolicy::ReevaluateAutoMode()
{
    if (m_LinearSamples == 0 || m_OctreeSamples == 0)
        return;

    SpatialCullingMode preferred = m_AutoMode;
    if (m_DirtyRatio >= HighDirtyRatio)
    {
        if (m_LinearCostMs <= m_OctreeCostMs / SwitchAdvantage)
            preferred = SpatialCullingMode::Linear;
    }
    else if (m_AutoMode == SpatialCullingMode::Linear)
    {
        if (m_OctreeCostMs < m_LinearCostMs * SwitchAdvantage ||
            (m_OctreeCostMs <= m_LinearCostMs * ComparableCost && m_OctreeCandidateRatio < UsefulCandidateRatio))
            preferred = SpatialCullingMode::Octree;
    }
    else if (m_LinearCostMs < m_OctreeCostMs * SwitchAdvantage ||
             (m_LinearCostMs <= m_OctreeCostMs * ComparableCost && m_OctreeCandidateRatio > IneffectiveCandidateRatio))
    {
        preferred = SpatialCullingMode::Linear;
    }

    if (preferred != m_AutoMode)
    {
        m_AutoMode = preferred;
        m_FramesSinceProbe = 0;
    }
}
