#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <cstdint>

class AnimationGraphPanel : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Animation Graph"; }
    PanelGroup GetGroup() const override { return PanelGroup::Tools; }

private:
    uint32_t m_SelectedState = 0;
    uint32_t m_SelectedTransition = 0;
    float m_InspectorWidth = 330.0f;
    float m_CanvasZoom = 1.0f;
    float m_CanvasPanX = 0.0f;
    float m_CanvasPanY = 0.0f;
};
#endif
