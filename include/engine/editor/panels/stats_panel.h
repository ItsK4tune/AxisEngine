#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class StatsPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnUpdate(float dt) override
    {
        const float alpha = 0.1f;
        m_FrameTime = (dt * 1000.0f) * alpha + m_FrameTime * (1.0f - alpha);
        m_Fps = (dt > 0.0f ? (1.0f / dt) : 0.0f) * alpha + m_Fps * (1.0f - alpha);
    }
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Stats"; }
    PanelGroup GetGroup() const override { return PanelGroup::Debug; }

private:
    float m_Fps       = 0.0f;
    float m_FrameTime = 0.0f;
};
#endif
