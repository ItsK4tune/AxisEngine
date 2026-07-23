#pragma once

#include <editor/i_editor_panel.h>
#include <array>
#include <string>

class LightingPanel final : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Lighting [Ctrl+Shift+6]"; }
    PanelGroup GetGroup() const override { return PanelGroup::Tools; }

private:
    bool BakeLightmaps(Scene& scene);
    std::string m_Status;
    std::array<char, 260> m_OutputPath{"assets/generated/lightmaps/{entity}.ppm"};
    int m_LightmapResolution = 256;
    float m_Ambient = 0.08f;
};
