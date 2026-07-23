#pragma once

#include <core/logic/runtime_profiler.h>
#include <editor/i_editor_panel.h>
#include <render/interface/i_post_process_registry.h>
#include <ecs/interface/i_render_runtime_control.h>
#include <array>
#include <deque>
#include <string>
#include <vector>

struct FrameDebuggerTimedSample
{
    RuntimeProfilerStats stats;
    float seconds = 0.0f;
};

class FrameDebuggerPanel final : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Frame Debugger [Ctrl+Shift+5]"; }
    PanelGroup GetGroup() const override { return PanelGroup::Debug; }

private:
    bool m_Frozen = false;
    RuntimeProfilerStats m_CapturedStats;
    std::vector<RegisteredPostProcessEffect> m_CapturedEffects;
    std::vector<FrameDebugDrawCall> m_CapturedDrawCalls;
    int m_DrawLimit = 0;
    int m_SelectedDraw = -1;
    std::deque<FrameDebuggerTimedSample> m_Samples;
    float m_SampleSeconds = 0.0f;
    RuntimeProfilerStats m_SmoothedStats;
    std::array<char, 128> m_LogFolder{"frame_debugger"};
    std::array<char, 128> m_LogName{"capture"};
    std::string m_LogStatus;
};
