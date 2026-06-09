#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class StatePanel : public IEditorPanel
{
public:
    void Initialize() override
    {
    }
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override
    {
        return "State Machine [Ctrl+8]";
    }
    PanelGroup GetGroup() const override
    {
        return PanelGroup::Debug;
    }
};
#endif
