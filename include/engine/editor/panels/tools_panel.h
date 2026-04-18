#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class ToolsPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Tools"; }

};
#endif
