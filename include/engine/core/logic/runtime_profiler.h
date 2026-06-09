#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

enum class ProfiledRenderPass
{
    TotalFrame = 0,
    Shadow,
    Capture,
    Geometry,
    Lighting,
    Alpha,
    Transparent,
    Bloom,
    PostProcess,
    UI,
    Count
};

struct RuntimeProfilerStats
{
    float cpuFrameMs = 0.0f;
    float gpuFrameMs = 0.0f;
    float gpuUsagePercent = 0.0f;
    bool hasGpuFrameTime = false;
    bool hasGpuUsage = false;

    uint64_t vramUsedBytes = 0;
    uint64_t vramTotalBytes = 0;
    bool hasVram = false;

    std::array<float, static_cast<size_t>(ProfiledRenderPass::Count)> passMs = {};
};

class RuntimeProfiler
{
public:
    static RuntimeProfiler& Instance()
    {
        static RuntimeProfiler profiler;
        return profiler;
    }

    void BeginFrame()
    {
        m_Stats.passMs.fill(0.0f);
        m_Stats.cpuFrameMs = 0.0f;
    }

    void SetCpuFrameTime(float ms)
    {
        m_Stats.cpuFrameMs = ms;
        if (m_Stats.hasGpuFrameTime && !m_Stats.hasGpuUsage && ms > 0.0f)
        {
            m_Stats.gpuUsagePercent = std::clamp((m_Stats.gpuFrameMs / ms) * 100.0f, 0.0f, 100.0f);
        }
    }

    void SetGpuFrameTime(float ms)
    {
        m_Stats.gpuFrameMs = ms;
        m_Stats.hasGpuFrameTime = ms > 0.0f;
        if (!m_Stats.hasGpuUsage)
        {
            const float referenceMs = m_Stats.cpuFrameMs > 0.0f ? m_Stats.cpuFrameMs : 16.67f;
            m_Stats.gpuUsagePercent = std::clamp((ms / referenceMs) * 100.0f, 0.0f, 100.0f);
        }
    }

    void SetGpuUsage(float percent)
    {
        m_Stats.gpuUsagePercent = std::clamp(percent, 0.0f, 100.0f);
        m_Stats.hasGpuUsage = true;
    }

    void SetVramUsage(uint64_t usedBytes, uint64_t totalBytes)
    {
        m_Stats.vramUsedBytes = usedBytes;
        m_Stats.vramTotalBytes = totalBytes;
        m_Stats.hasVram = totalBytes > 0;
    }

    void AddPassTime(ProfiledRenderPass pass, float ms)
    {
        m_Stats.passMs[static_cast<size_t>(pass)] += ms;
    }

    void SetPassTime(ProfiledRenderPass pass, float ms)
    {
        m_Stats.passMs[static_cast<size_t>(pass)] = ms;
    }

    const RuntimeProfilerStats& GetStats() const
    {
        return m_Stats;
    }

private:
    RuntimeProfiler() = default;

    RuntimeProfilerStats m_Stats;
};
