#include <editor/panels/profiler_panel.h>
#ifdef ENABLE_EDITOR
#include <imgui.h>
#include <scene/logic/scene.h>
#include <core/logic/service_locator.h>
#include <ecs/interface/i_render_service.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/render_components.h>
#include <ecs/unit/physics_components.h>
#include <ecs/unit/ui_components.h>
#include <algorithm>
#include <cmath>

void ProfilerPanel::OnUpdate(float dt)
{
    m_FrameTime = dt * 1000.0f;
    m_Fps = dt > 0.0f ? 1.0f / dt : 0.0f;

    m_FrameTimeHistory[m_HistoryOffset] = m_FrameTime;
    m_FpsHistory[m_HistoryOffset] = m_Fps;
    m_HistoryOffset = (m_HistoryOffset + 1) % HISTORY_SIZE;

    // Running stats
    m_MinFrameTime = 999.0f;
    m_MaxFrameTime = 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < HISTORY_SIZE; ++i) {
        float ft = m_FrameTimeHistory[i];
        if (ft > 0.01f) {
            m_MinFrameTime = std::min(m_MinFrameTime, ft);
            m_MaxFrameTime = std::max(m_MaxFrameTime, ft);
            sum += ft;
        }
    }
    m_AvgFrameTime = sum / HISTORY_SIZE;
}

void ProfilerPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    // Frame time graph
    ImGui::Text("Frame Time (ms)");
    char overlay[64];
    snprintf(overlay, sizeof(overlay), "%.2f ms (%.0f FPS)", m_FrameTime, m_Fps);
    ImGui::PlotLines("##FrameTime", m_FrameTimeHistory, HISTORY_SIZE, m_HistoryOffset, overlay,
                     0.0f, m_MaxFrameTime * 1.5f, ImVec2(-1, 80));

    // FPS graph
    ImGui::Text("FPS");
    char fpsOverlay[32];
    snprintf(fpsOverlay, sizeof(fpsOverlay), "%.0f", m_Fps);
    ImGui::PlotLines("##FPS", m_FpsHistory, HISTORY_SIZE, m_HistoryOffset, fpsOverlay,
                     0.0f, 0.0f, ImVec2(-1, 60));

    ImGui::Separator();

    // Stats table
    ImGui::Columns(2, "ProfilerStats", false);
    ImGui::SetColumnWidth(0, 150.0f);

    ImGui::Text("Avg Frame Time"); ImGui::NextColumn();
    ImGui::Text("%.2f ms", m_AvgFrameTime); ImGui::NextColumn();

    ImGui::Text("Min Frame Time"); ImGui::NextColumn();
    ImGui::Text("%.2f ms", m_MinFrameTime < 999.0f ? m_MinFrameTime : 0.0f); ImGui::NextColumn();

    ImGui::Text("Max Frame Time"); ImGui::NextColumn();
    ImGui::Text("%.2f ms", m_MaxFrameTime); ImGui::NextColumn();

    ImGui::Text("Avg FPS"); ImGui::NextColumn();
    ImGui::Text("%.0f", m_AvgFrameTime > 0.01f ? 1000.0f / m_AvgFrameTime : 0.0f); ImGui::NextColumn();

    ImGui::Columns(1);

    ImGui::Separator();

    // Entity/render stats
    auto& reg = scene.registry;
    ImGui::Text("Entities: %zu", reg.storage<entt::entity>().size());

    size_t meshCount = reg.view<MeshRendererComponent>().size();
    size_t physicsCount = reg.view<RigidBodyComponent>().size();
    size_t uiCount = reg.view<UITransformComponent>().size();
    ImGui::Text("Mesh Renderers: %zu", meshCount);
    ImGui::Text("Physics Bodies: %zu", physicsCount);
    ImGui::Text("UI Elements: %zu", uiCount);

    auto rs = ServiceLocator::Instance().Resolve<IRenderService>();
    if (rs) {
        ImGui::Text("Rendered: %d", rs->GetRenderedCount());
    }

    // Budget indicator
    ImGui::Separator();
    float budget = 16.67f; // 60fps target
    float ratio = m_AvgFrameTime / budget;
    ImVec4 budgetColor = ratio < 0.8f ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f)
                       : ratio < 1.0f ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f)
                                      : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);

    ImGui::TextColored(budgetColor, "Frame Budget: %.0f%% (target 60fps)", ratio * 100.0f);
    ImGui::ProgressBar(std::min(ratio, 2.0f) / 2.0f, ImVec2(-1, 0),
                       ratio < 1.0f ? "Under Budget" : "OVER BUDGET");

    ImGui::End();
}
#endif
