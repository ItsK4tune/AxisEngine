#include <editor/panels/tools_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>

#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <platform/logic/io_handler.h>
#include <platform/logic/mouse_manager.h>

void ToolsPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    if (!cm) { ImGui::End(); return; }
    
    auto conf = cm->GetConfig();
    bool changed = false;
    if (auto* io = ServiceLocator::Instance().Resolve<IOHandler>()) {
        auto& mouse = io->GetMouse();
        CursorMode mode = mouse.GetCursorMode();
        const char* modeStr = (mode == CursorMode::Locked)   ? "Locked (FPS)" :
                              (mode == CursorMode::Disabled)  ? "Disabled (Panel)" :
                              (mode == CursorMode::Normal)    ? "Normal" :
                              (mode == CursorMode::Hidden)    ? "Hidden" : "LockedHidden";
        ImGui::Text("Cursor: %s", modeStr);
        ImGui::SameLine();
        if (mode == CursorMode::Locked || mode == CursorMode::LockedHidden) {
            if (ImGui::SmallButton("Free")) mouse.SetCursorMode(CursorMode::Normal);
        } else {
            if (ImGui::SmallButton("Lock (FPS)")) mouse.SetCursorMode(CursorMode::Locked);
        }
    }
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Checkbox("Wireframe Mode", &conf.debug.wireframeMode)) changed = true;
        if (ImGui::Checkbox("No Textures", &conf.debug.noTexture)) changed = true;
        if (ImGui::Checkbox("Enable Shadows", &conf.shadow.shadowsEnabled)) changed = true;
        if (ImGui::SliderFloat("Skybox Intensity", &conf.render.skyboxIntensity, 0.0f, 2.0f)) changed = true;
    }
    
    if (ImGui::CollapsingHeader("Debug Vis", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::Checkbox("Physics Debug", &conf.debug.physicsDebug)) changed = true;
        if (ImGui::Checkbox("Entity Names", &conf.debug.entityNames)) changed = true;
        if (ImGui::Checkbox("Gizmos", &conf.debug.gizmos)) changed = true;
        if (ImGui::Checkbox("Light Gizmos", &conf.debug.lightGizmos)) changed = true;
        if (ImGui::Checkbox("UI Enabled", &conf.debug.uiEnabled)) changed = true;
    }

    if (changed) {
        cm->UpdateConfig(conf, ConfigChangedEvent::All);
    }

    ImGui::End();
}
#endif
