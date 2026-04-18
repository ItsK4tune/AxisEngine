#include <editor/panels/stats_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>
#include <scene/logic/scene.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/ui_components.h>

void StatsPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    
    ImGui::Text("FPS: %.1f", m_Fps);
    ImGui::Text("Frame Time: %.3f ms", m_FrameTime);
    ImGui::Separator();

    auto& reg = scene.registry;
    size_t totalEntities = reg.storage<entt::entity>().size();
    size_t meshEntities = reg.view<MeshRendererComponent>().size();
    size_t physicsEntities = reg.view<RigidBodyComponent>().size();
    size_t uiEntities = reg.view<UITransformComponent>().size();

    ImGui::Text("Total Entities: %zu", totalEntities);
    ImGui::Text("Mesh Renderers: %zu", meshEntities);
    ImGui::Text("Physics Bodies: %zu", physicsEntities);
    ImGui::Text("UI Elements: %zu", uiEntities);
    
    ImGui::Separator();
    auto rs = ServiceLocator::Instance().Resolve<IRenderService>();
    if (rs) {
        ImGui::Text("Rendered Entities: %d", rs->GetRenderedCount());
    } else {
        ImGui::Text("Render Service: offline");
    }

    ImGui::Separator();
    if (ImGui::CollapsingHeader("Debug Tools", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto cm = ServiceLocator::Instance().Resolve<ConfigManager>();
        if (cm) {
            auto conf = cm->GetConfig();
            
            auto boolStr = [](bool v) { return v ? "ON" : "OFF"; };
            
            ImGui::Text("Wireframe Mode  : %s", boolStr(conf.debug.wireframeMode));
            ImGui::Text("Entity Names    : %s", boolStr(conf.debug.entityNames));
            ImGui::Text("Transform Gizmos: %s", boolStr(conf.debug.gizmos));
            ImGui::Text("Light Gizmos    : %s", boolStr(conf.debug.lightGizmos));
            ImGui::Text("Physics Debug   : %s", boolStr(conf.debug.physicsDebug));
            ImGui::Text("UI Enabled      : %s", boolStr(conf.debug.uiEnabled));
            ImGui::Separator();
            ImGui::Text("Audio Debug     : %s", boolStr(conf.debug.audioDebug));
            ImGui::Text("Particle Debug  : %s", boolStr(conf.debug.particleDebug));
        }
    }

    if (ImGui::CollapsingHeader("Scene Graph", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto view = scene.registry.view<InfoComponent>();
        for (auto entity : view) {
            const auto &info = view.get<InfoComponent>(entity);
            uint32_t sequentialId = (uint32_t)entity & 0xFFFFF; 
            
            std::string label = "[" + std::to_string(sequentialId) + "] " + info.name;
            if (!info.tag.empty()) label += " (" + info.tag + ")";
            
            if (ImGui::Selectable(label.c_str())) {
                // Potential selection logic here if shared across panels
            }
        }
    }

    ImGui::End();
}
#endif
