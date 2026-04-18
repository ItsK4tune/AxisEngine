#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class ResourceBrowserPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Resources"; }
    PanelGroup GetGroup() const override { return PanelGroup::Scene; }

private:
    int m_ActiveTab = 0;
};
#endif
