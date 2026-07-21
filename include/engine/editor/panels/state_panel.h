#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class StatePanel : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override
    {
        return "State Machine [Ctrl+8]";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Debug;
    }

private:
    float m_ScrollX = 0.0f;
    float m_ScrollY = 0.0f;
    float m_Zoom = 0.9f;
    bool m_FirstFrame = true;
};
#endif
