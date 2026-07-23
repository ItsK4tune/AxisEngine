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
        ImGui::Text("Ctrl+Z    : Undo");
        ImGui::Text("Ctrl+Shift+Z : Redo");
        ImGui::Text("Ctrl+D    : Duplicate Selected Entity");
        ImGui::Text("Delete    : Delete Selected Entity");
        ImGui::Text("Ctrl+R    : Reload Active Scene");
        ImGui::Text("Ctrl+G    : Toggle Grid Snapping");
        ImGui::Text("Ctrl+H    : Toggle Grid Indicator");
        ImGui::Separator();
        ImGui::Text("PANEL TOGGLES:");
        ImGui::Text("  Ctrl+1  : Scene Hierarchy");
        ImGui::Text("  Ctrl+2  : Project / Assets");
        ImGui::Text("  Ctrl+3  : Inspector");
        ImGui::Text("  Ctrl+4  : Tools Panel");
        ImGui::Text("  Ctrl+5  : Settings Panel");
        ImGui::Text("  Ctrl+6  : Profiler & Stats");
        ImGui::Text("  Ctrl+7  : Console Panel");
        ImGui::Text("  Ctrl+8  : State Machine");
        ImGui::Text("  Ctrl+9  : Network Controller");
        ImGui::Text("  Ctrl+0  : Help Panel");
        ImGui::Text("  Ctrl+Shift+1 : Animation Graph");
        ImGui::Text("  Ctrl+Shift+2 : VFX Graph");
        ImGui::Text("  Ctrl+Shift+3 : Input Actions");
        ImGui::Text("  Ctrl+Shift+4 : Navigation");
        ImGui::Text("  Ctrl+Shift+5 : Frame Debugger");
        ImGui::Text("  Ctrl+Shift+6 : Lighting");
        ImGui::Text("  Ctrl+Shift+7 : Prefabs");
        ImGui::Text("  Ctrl+Shift+8 : Reserved");
        ImGui::Text("  Ctrl+Shift+0 : Reserved");
        ImGui::Separator();
        ImGui::Text("Viewport drag : Box-select entities (Ctrl adds to selection)");
        ImGui::Text("Gizmo toolbar : Move/Rotate/Scale, World/Local, Pivot/Center");
        ImGui::Text("Alt + Arrows / PgUp/PgDn            : Nudge Position (X/Z, Y)");
        ImGui::Text("Ctrl + Alt + Arrows / PgUp/PgDn     : Nudge Rotation (Yaw/Pitch, Roll)");
        ImGui::Text("Shift + Alt + Arrows / PgUp/PgDn    : Nudge Scale (X/Z, Y)");
    }

    if (ImGui::CollapsingHeader("CAMERA NAVIGATION", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("Use these gestures over the game viewport, outside editor panels.");
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
        ImGui::Text("F4        : Toggle Skybox");
        ImGui::Text("F5        : Toggle Shadows");
        ImGui::Text("F6        : Toggle Post Process System");
    }

    if (ImGui::CollapsingHeader("DEBUG OVERLAYS", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("F7        : Toggle Physics Debug");
        ImGui::Text("F8        : Toggle Audio Debug");
        ImGui::Text("F9        : Toggle Particle Debug");
    }

    if (ImGui::CollapsingHeader("GAME CONTROL", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::Text("F10       : Toggle Editor Cursor (restore previous mode on exit)");
        ImGui::Text("Shift+F10 : Toggle Debug Camera (Free Cam)");
        ImGui::Text("F11       : Pause/Resume Game");
        ImGui::Text("F12       : Cycle Time Scale (0.25x -> 2x)");
    }

    ImGui::End();
}
#endif
