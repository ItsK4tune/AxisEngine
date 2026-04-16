#pragma once
#ifdef ENABLE_EDITOR
#include <editor/i_editor_panel.h>
#include <string>

struct Scene;

class SettingsPanel : public IEditorPanel
{
public:
    void Initialize() override {}
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Settings"; }

private:
    float m_TimeScale    = 1.0f;
    bool  m_VsyncEnabled = true;
    int   m_FrameLimit   = 0;
    bool  m_IsPaused     = false;
};
#endif
