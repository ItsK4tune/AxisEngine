#pragma once

#include <editor/i_editor_panel.h>
#include <array>
#include <vector>

class InputActionsPanel final : public IEditorPanel
{
public:
    void OnImGui(Scene& scene) override;
    std::string GetTitle() const override { return "Input Actions [Ctrl+Shift+3]"; }
    PanelGroup GetGroup() const override { return PanelGroup::Tools; }

private:
    std::array<char, 128> m_ActionName{};
    std::array<char, 128> m_Filter{};
    std::array<char, 260> m_SavePath{"input_bindings.axs"};
    std::array<char, 260> m_DraftPath{"assets/input/new_input.axs"};
    std::vector<std::string> m_DiscoveredFiles;
    int m_InputType = 0;
    int m_Code = 0;
    bool m_AdvancedCode = false;
    bool m_AllowOverwrite = false;
    std::string m_Status;
};
