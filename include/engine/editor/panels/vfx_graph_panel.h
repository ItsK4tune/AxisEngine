#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <entt/entity/entity.hpp>
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
    uint32_t m_LinkFrom = 0;
    uint32_t m_LinkTo = 0;
    entt::entity m_ContextEntity = entt::null;
    int m_NewNodeType = 0;
    float m_InspectorWidth = 320.0f;
    float m_CanvasZoom = 1.0f;
    float m_CanvasPanX = 0.0f;
    float m_CanvasPanY = 0.0f;
};
#endif
