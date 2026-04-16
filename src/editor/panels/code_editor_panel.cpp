#include <editor/panels/code_editor_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>

void CodeEditorPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    ImGui::Text("Code Editor Placeholder");
    ImGui::TextWrapped("Currently does not load anything. Will allow runtime file browsing inside AxisEngine.");
    
    if (ImGui::Button("Browse File")) {
        // Simple file browse trigger in follow-up
    }

    ImGui::End();
}

void CodeEditorPanel::BrowseForFile() {}
void CodeEditorPanel::LoadFile(const std::string& path) {}
void CodeEditorPanel::SaveFile() {}
#endif
