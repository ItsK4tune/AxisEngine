#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <cstdint>

class VFXGraphPanel : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "VFX Graph"; }
    PanelGroup GetGroup() const override { return PanelGroup::Tools; }

private:
    uint32_t m_SelectedNode = 0;
    uint32_t m_SelectedLink = 0;
    float m_InspectorWidth = 320.0f;
    float m_CanvasZoom = 1.0f;
    float m_CanvasPanX = 0.0f;
    float m_CanvasPanY = 0.0f;
};
#endif
