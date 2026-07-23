#pragma once
#ifdef ENABLE_EDITOR
#include <core/logic/runtime_profiler.h>
#include <editor/i_editor_panel.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

struct Scene;

class ProfilerPanel : public IEditorPanel
{
public:
    void Initialize() override
    {
    }
    void OnUpdate(float dt) override;
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override
    {
        return "Profiler & Stats [Ctrl+6]";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Debug;
    }

private:
    static constexpr int FRAME_HISTORY_SIZE = 2048;
    static constexpr float SMOOTHING_WINDOW_SECONDS = 3.0f;

    struct CpuTimes
    {
        uint64_t total = 0;
        uint64_t idle = 0;
    };

    void RecalculateFrameWindowStats();
    void SmoothRuntimeStats(float dt);
    void UpdateSystemStats(float dt);

    float m_FrameTimeHistory[FRAME_HISTORY_SIZE] = {};
    float m_FrameDeltaHistory[FRAME_HISTORY_SIZE] = {};
    int m_HistoryOffset = 0;
    float m_FrameTime = 0.0f;
    float m_CpuFrameTime = 0.0f;
    float m_Fps = 0.0f;
    float m_RawFps = 0.0f;
    float m_MinFrameTime = 999.0f;
    float m_MaxFrameTime = 0.0f;
    float m_AvgFrameTime = 0.0f;
    float m_GraphMaxFrameTime = 0.0f;

    bool m_HasRuntimeSmoothSample = false;
    float m_SmoothedCpuFrameTime = 0.0f;
    float m_SmoothedGpuFrameTime = 0.0f;
    float m_SmoothedGpuUsage = 0.0f;
    std::array<float, static_cast<size_t>(ProfiledRenderPass::Count)> m_SmoothedPassMs = {};

    struct RuntimeWindowSample
    {
        float duration = 0.0f;
        float cpuFrameMs = 0.0f;
        float gpuFrameMs = 0.0f;
        float gpuUsage = 0.0f;
        std::array<float, static_cast<size_t>(ProfiledRenderPass::Count)> passMs = {};
    };
    std::deque<RuntimeWindowSample> m_RuntimeWindow;
    float m_RuntimeWindowDuration = 0.0f;
    float m_RuntimeCpuWeightedSum = 0.0f;
    float m_RuntimeGpuWeightedSum = 0.0f;
    float m_RuntimeGpuUsageWeightedSum = 0.0f;
    std::array<float, static_cast<size_t>(ProfiledRenderPass::Count)> m_RuntimePassWeightedSums = {};

    float m_SystemStatsTimer = 0.0f;
    bool m_HasCpuSample = false;
    bool m_HasCpuUsageSmoothSample = false;
    std::vector<CpuTimes> m_LastCpuTimes;
    float m_CpuUsageTotal = 0.0f;
    std::vector<float> m_CpuUsagePerCore;
    uint64_t m_RamUsedBytes = 0;
    uint64_t m_RamTotalBytes = 0;
    uint64_t m_ProcessRamBytes = 0;
};
#endif
