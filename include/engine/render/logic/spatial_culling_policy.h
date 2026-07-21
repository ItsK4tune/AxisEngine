#pragma once

#include <core/type/spatial_culling_mode.h>
#include <cstddef>
#include <cstdint>

struct SpatialCullingPolicyMetrics
{
    SpatialCullingMode configuredMode = SpatialCullingMode::Auto;
    SpatialCullingMode autoMode = SpatialCullingMode::Linear;
    float linearCostMs = 0.0f;
    float octreeCostMs = 0.0f;
    float octreeCandidateRatio = 1.0f;
    float dirtyRatio = 0.0f;
    uint32_t linearSamples = 0;
    uint32_t octreeSamples = 0;
};

// Chooses the render-candidate backend. Auto mode periodically samples the
// inactive path, compares moving CPU costs, and uses workload signals only as
// tie-breakers. It deliberately has no hard entity-count threshold.
class SpatialCullingPolicy
{
public:
    void SetMode(SpatialCullingMode mode);
    SpatialCullingMode GetMode() const;
    SpatialCullingMode GetAutoMode() const;
    SpatialCullingMode Select(bool octreeAvailable, bool allowProbe = true) const;
    void RecordSample(SpatialCullingMode executedMode, float costMs, size_t entityCount, size_t candidateCount,
                      size_t dirtyEvents, bool ignoreTiming = false);
    void ResetMeasurements();
    SpatialCullingPolicyMetrics GetMetrics() const;

private:
    static void UpdateAverage(float& average, uint32_t& sampleCount, float value);
    void ReevaluateAutoMode();

    SpatialCullingMode m_Mode = SpatialCullingMode::Auto;
    SpatialCullingMode m_AutoMode = SpatialCullingMode::Linear;
    float m_LinearCostMs = 0.0f;
    float m_OctreeCostMs = 0.0f;
    float m_OctreeCandidateRatio = 1.0f;
    float m_DirtyRatio = 0.0f;
    uint32_t m_LinearSamples = 0;
    uint32_t m_OctreeSamples = 0;
    uint32_t m_OctreeWorkloadSamples = 0;
    uint32_t m_FramesSinceProbe = 0;
};
