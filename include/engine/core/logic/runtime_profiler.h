#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

enum class ProfiledRenderPass
{
    TotalFrame = 0,
    Input,
    ResourceUpdate,
    GameUpdate,
    FixedUpdate,
    Physics,
    Navigation,
    QueueBuild,
    Shadow,
    Capture,
    Geometry,
    Lighting,
    Alpha,
    Transparent,
    Bloom,
    PostProcess,
    UI,
    Swap,
    FrameLimiter,
    Count
};

struct RuntimeProfilerStats
{
    float cpuFrameMs = 0.0f;
    float gpuFrameMs = 0.0f;
    float gpuUsagePercent = 0.0f;
    bool hasGpuFrameTime = false;
    bool hasGpuUsage = false;

    float cpuP50Ms = 0.0f;
    float cpuP95Ms = 0.0f;
    float cpuP99Ms = 0.0f;
    float gpuP50Ms = 0.0f;
    float gpuP95Ms = 0.0f;
    float gpuP99Ms = 0.0f;
    uint64_t frameHitchCount = 0;

    uint64_t ramUsedBytes = 0;
    bool hasRam = false;

    uint64_t vramUsedBytes = 0;
    uint64_t vramTotalBytes = 0;
    bool hasVram = false;

    uint64_t queueBuildCount = 0;
    uint64_t drawCalls = 0;
    uint64_t instancedBatches = 0;
    uint64_t submittedTriangles = 0;
    uint64_t culledEntities = 0;
    uint64_t stateChanges = 0;
    uint64_t uniformUpdates = 0;
    uint64_t uploadBytes = 0;
    uint64_t transientAllocations = 0;
    float jobWaitMs = 0.0f;

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
        if (m_Stats.cpuFrameMs > 0.0f)
            m_LastCompletedStats = m_Stats;
        m_Stats.passMs.fill(0.0f);
        m_Stats.cpuFrameMs = 0.0f;
        m_Stats.queueBuildCount = 0;
        m_Stats.drawCalls = 0;
        m_Stats.instancedBatches = 0;
        m_Stats.submittedTriangles = 0;
        m_Stats.culledEntities = 0;
        m_Stats.stateChanges = 0;
        m_Stats.uniformUpdates = 0;
        m_Stats.uploadBytes = 0;
        m_Stats.transientAllocations = 0;
        m_Stats.jobWaitMs = 0.0f;
        m_JobWaitMs.store(0.0f, std::memory_order_relaxed);
    }

    void Reset()
    {
        m_Stats = {};
        m_LastCompletedStats = {};
        m_CpuHistory.fill(0.0f);
        m_GpuHistory.fill(0.0f);
        m_CpuHistoryCount = m_GpuHistoryCount = 0;
        m_CpuHistoryCursor = m_GpuHistoryCursor = 0;
        m_JobWaitMs.store(0.0f, std::memory_order_relaxed);
    }

    void SetCpuFrameTime(float ms)
    {
        m_Stats.jobWaitMs = m_JobWaitMs.load(std::memory_order_relaxed);
        m_Stats.cpuFrameMs = ms;
        RecordSample(m_CpuHistory, m_CpuHistoryCount, m_CpuHistoryCursor, ms, m_Stats.cpuP50Ms,
                     m_Stats.cpuP95Ms, m_Stats.cpuP99Ms);
        if (m_CpuHistoryCount >= 30 && m_Stats.cpuP95Ms > 0.0f && ms > (std::max)(50.0f, m_Stats.cpuP95Ms * 1.5f))
            ++m_Stats.frameHitchCount;
        if (m_Stats.hasGpuFrameTime && !m_Stats.hasGpuUsage && ms > 0.0f)
        {
            m_Stats.gpuUsagePercent = std::clamp((m_Stats.gpuFrameMs / ms) * 100.0f, 0.0f, 100.0f);
        }
    }

    void SetGpuFrameTime(float ms)
    {
        m_Stats.gpuFrameMs = ms;
        m_Stats.hasGpuFrameTime = ms > 0.0f;
        if (ms > 0.0f)
            RecordSample(m_GpuHistory, m_GpuHistoryCount, m_GpuHistoryCursor, ms, m_Stats.gpuP50Ms,
                         m_Stats.gpuP95Ms, m_Stats.gpuP99Ms);
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

    void SetRamUsage(uint64_t usedBytes)
    {
        m_Stats.ramUsedBytes = usedBytes;
        m_Stats.hasRam = usedBytes > 0;
    }

    void AddQueueBuild(uint64_t count = 1) { m_Stats.queueBuildCount += count; }
    void AddDrawCalls(uint64_t count = 1) { m_Stats.drawCalls += count; }
    void AddInstancedBatch(uint64_t count = 1) { m_Stats.instancedBatches += count; }
    void AddTriangles(uint64_t count) { m_Stats.submittedTriangles += count; }
    void AddCulledEntities(uint64_t count) { m_Stats.culledEntities += count; }
    void AddStateChanges(uint64_t count = 1) { m_Stats.stateChanges += count; }
    void AddUniformUpdates(uint64_t count = 1) { m_Stats.uniformUpdates += count; }
    void AddUploadBytes(uint64_t bytes) { m_Stats.uploadBytes += bytes; }
    void AddTransientAllocations(uint64_t count = 1) { m_Stats.transientAllocations += count; }
    void AddJobWaitTime(float ms)
    {
        m_JobWaitMs.fetch_add((std::max)(0.0f, ms), std::memory_order_relaxed);
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

    const RuntimeProfilerStats& GetLastCompletedStats() const
    {
        return m_LastCompletedStats;
    }

private:
    RuntimeProfiler() = default;

    static constexpr size_t HistoryCapacity = 240;

    static void RecordSample(std::array<float, HistoryCapacity>& history, size_t& count, size_t& cursor, float value,
                             float& p50, float& p95, float& p99)
    {
        history[cursor] = value;
        cursor = (cursor + 1) % HistoryCapacity;
        count = (std::min)(count + 1, HistoryCapacity);
        if (count < 8 || (cursor % 15) != 0)
            return;

        auto sorted = history;
        std::sort(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(count));
        const auto percentile = [&](float ratio) {
            const size_t index = (std::min)(count - 1, static_cast<size_t>(ratio * static_cast<float>(count - 1)));
            return sorted[index];
        };
        p50 = percentile(0.50f);
        p95 = percentile(0.95f);
        p99 = percentile(0.99f);
    }

    RuntimeProfilerStats m_Stats;
    RuntimeProfilerStats m_LastCompletedStats;
    std::array<float, HistoryCapacity> m_CpuHistory = {};
    std::array<float, HistoryCapacity> m_GpuHistory = {};
    size_t m_CpuHistoryCount = 0;
    size_t m_GpuHistoryCount = 0;
    size_t m_CpuHistoryCursor = 0;
    size_t m_GpuHistoryCursor = 0;
    // Wait() can execute on worker threads while the render/main thread owns
    // the rest of the frame statistics.
    std::atomic<float> m_JobWaitMs{0.0f};
};
