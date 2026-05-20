#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class NetworkPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Network Controller"; }
    PanelGroup GetGroup() const override { return PanelGroup::Tools; }
};
#endif
