#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class SettingsPanel : public IEditorPanel
{
public:
    void Initialize() override;
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Settings [Ctrl+4]"; }
    PanelGroup GetGroup() const override { return PanelGroup::Debug; }

private:
    std::string m_CpuName = "Unknown CPU";
    std::string m_GpuName = "Unknown GPU";
};
#endif
