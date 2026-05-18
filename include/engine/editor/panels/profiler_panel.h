#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <chrono>
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
        return "Profiler & Stats";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Debug;
    }

private:
    static constexpr int HISTORY_SIZE = 120;
    float m_FrameTimeHistory[HISTORY_SIZE] = {};
    float m_FpsHistory[HISTORY_SIZE] = {};
    int m_HistoryOffset = 0;
    float m_FrameTime = 0.0f;
    float m_Fps = 0.0f;
    float m_MinFrameTime = 999.0f;
    float m_MaxFrameTime = 0.0f;
    float m_AvgFrameTime = 0.0f;
};
#endif
