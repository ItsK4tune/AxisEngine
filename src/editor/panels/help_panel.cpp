#include <editor/panels/help_panel.h>

#ifdef ENABLE_EDITOR
#include <imgui.h>

void HelpPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    ImGui::Text("========== EDITOR CONTROLS ==========");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("GENERAL", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Ctrl+S    : Save All Scenes");
        ImGui::Text("Ctrl+D    : Duplicate Selected Entity");
        ImGui::Text("Ctrl+R    : Reload Active Scene");
        ImGui::Text("Ctrl+G    : Toggle Grid Snapping");
        ImGui::Text("Shift+G   : Toggle Grid Indicator");
        ImGui::Text("Ctrl+1..9 : Toggle Panels 1-9");
        ImGui::Separator();
        ImGui::Text("Alt + Arrows / PgUp/PgDn            : Nudge Position (X/Z, Y)");
        ImGui::Text("Ctrl + Alt + Arrows / PgUp/PgDn     : Nudge Rotation (Yaw/Pitch, Roll)");
        ImGui::Text("Shift + Alt + Arrows / PgUp/PgDn    : Nudge Scale (X/Z, Y)");
    }

    if (ImGui::CollapsingHeader("CAMERA NAVIGATION", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("RMB        : Look Around");
        ImGui::Text("RMB + WASD : Fly Movement (Q/E for Up/Down)");
        ImGui::Text("MMB Drag   : Pan Camera");
        ImGui::Text("Alt + LMB  : Orbit Camera");
        ImGui::Text("Scroll     : Dolly Zoom");
        ImGui::Text("F          : Frame Selected Entity");
        ImGui::Text("Shift/Ctrl : Fast/Slow Fly Speed Modifier");
        ImGui::Text("Scroll+RMB : Adjust Base Fly Speed");
    }

    if (ImGui::CollapsingHeader("VISUAL TOGGLES", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("F1        : Toggle Entity Names");
        ImGui::Text("F2        : Toggle Gizmos");
        ImGui::Text("F3        : Toggle Light Gizmos");
        ImGui::Text("Shift+F6  : Toggle Skybox");
        ImGui::Text("Shift+F7  : Toggle Shadows");
    }

    if (ImGui::CollapsingHeader("SYSTEM TOGGLES", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("F4        : Toggle Audio System (Mute/Unmute)");
        ImGui::Text("F5        : Toggle Post Process System");
        ImGui::Text("F8        : Toggle Physics Debug");
        ImGui::Text("Shift+F8  : Toggle Audio Debug");
        ImGui::Text("F9        : Toggle UI System");
        ImGui::Text("Shift+F9  : Toggle Particle Debug");
    }

    if (ImGui::CollapsingHeader("GAME CONTROL", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("F11       : Pause/Resume Game");
        ImGui::Text("Shift+F11 : Toggle Debug Camera (Free Cam)");
        ImGui::Text("F12       : Cycle Time Scale (0.25x -> 2x)");
        ImGui::Text("Shift+F12 : Cycle Cursor Mode");
    }

    ImGui::End();
}
#endif
