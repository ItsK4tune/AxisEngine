#pragma once
#include <editor/i_editor_panel.h>

#ifdef ENABLE_EDITOR
class HelpPanel : public IEditorPanel
{
public:
    HelpPanel() { m_Open = false; }
    ~HelpPanel() override = default;

    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Help [Ctrl+6]"; }
    PanelGroup GetGroup() const override { return PanelGroup::Help; }
};
#endif
