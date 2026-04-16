#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class StatsPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void SetStats(float fps, float frameTimeMs)
    {
        m_Fps       = fps;
        m_FrameTime = frameTimeMs;
    }
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Stats"; }

private:
    float m_Fps       = 0.0f;
    float m_FrameTime = 0.0f;
};
#endif
