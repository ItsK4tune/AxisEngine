#include <editor/panels/state_panel.h>

#ifdef ENABLE_EDITOR
#include <core/app/runtime_core.h>
#include <core/logic/service_locator.h>
#include <imgui.h>
#include <cmath>
#include <string>

void StatePanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto* core = ServiceLocator::Instance().Resolve<RuntimeCore>();
    if (!core)
    {
        ImGui::Text("RuntimeCore offline");
        ImGui::End();
        return;
    }

    auto& sm = core->GetStateMachine();
    State* currentState = sm.GetCurrentState();
    std::string stateName = "None";
    if (currentState)
    {
        std::string rawName = typeid(*currentState).name();
        if (rawName.rfind("class ", 0) == 0)
            stateName = rawName.substr(6);
        else
            stateName = rawName;
    }

    ImGui::Text("Current State: %s", stateName.c_str());
    ImGui::SameLine();
    if (ImGui::Button("Reset View"))
    {
        // Handled below
    }

    ImGui::BeginChild("StateMachineCanvas", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.x < 50.0f)
        canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f)
        canvas_size.y = 50.0f;
    ImVec2 canvas_end = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);

    static ImVec2 scrolling = ImVec2(0.0f, 0.0f);
    static float zoom = 1.0f;

    if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
    {
        scrolling.x += ImGui::GetIO().MouseDelta.x;
        scrolling.y += ImGui::GetIO().MouseDelta.y;
    }

    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f)
    {
        float mouse_wheel = ImGui::GetIO().MouseWheel;
        float prev_zoom = zoom;
        zoom += mouse_wheel * 0.05f;
        if (zoom < 0.3f)
            zoom = 0.3f;
        if (zoom > 2.0f)
            zoom = 2.0f;

        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 mouse_in_canvas =
            ImVec2(mouse_pos.x - canvas_pos.x - scrolling.x, mouse_pos.y - canvas_pos.y - scrolling.y);
        scrolling.x -= mouse_in_canvas.x * (zoom / prev_zoom - 1.0f);
        scrolling.y -= mouse_in_canvas.y * (zoom / prev_zoom - 1.0f);
    }

    // Reset view implementation
    static bool first_frame = true;
    if (first_frame || ImGui::IsItemActive())  // Reset view button above or first frame
    {
        scrolling = ImVec2(canvas_size.x * 0.1f, canvas_size.y * 0.1f);
        zoom = 0.9f;
        first_frame = false;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_pos, canvas_end, IM_COL32(20, 20, 25, 255));

    // Draw Grid
    float grid_size = 32.0f * zoom;
    for (float x = std::fmod(scrolling.x, grid_size); x < canvas_size.x; x += grid_size)
        draw_list->AddLine(ImVec2(canvas_pos.x + x, canvas_pos.y), ImVec2(canvas_pos.x + x, canvas_end.y),
                           IM_COL32(35, 35, 45, 255));
    for (float y = std::fmod(scrolling.y, grid_size); y < canvas_size.y; y += grid_size)
        draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + y), ImVec2(canvas_end.x, canvas_pos.y + y),
                           IM_COL32(35, 35, 45, 255));

    auto local_to_screen = [&](ImVec2 local) -> ImVec2 {
        return ImVec2(canvas_pos.x + scrolling.x + local.x * zoom, canvas_pos.y + scrolling.y + local.y * zoom);
    };

    auto draw_node = [&](const std::string& name, ImVec2 local_pos, ImVec2 local_size, bool active) {
        ImVec2 p_min = local_to_screen(local_pos);
        ImVec2 p_max = local_to_screen(ImVec2(local_pos.x + local_size.x, local_pos.y + local_size.y));

        ImU32 bg_color = active ? IM_COL32(40, 60, 100, 255) : IM_COL32(30, 30, 35, 255);
        ImU32 border_color = active ? IM_COL32(255, 200, 50, 255) : IM_COL32(65, 65, 75, 255);
        float thickness = active ? 2.5f : 1.5f;
        float rounding = 6.0f;

        if (active)
        {
            float t = (float)ImGui::GetTime();
            float glow = 0.5f + 0.5f * std::sin(t * 6.0f);
            ImU32 glow_color = IM_COL32(255, 200, 50, (int)(glow * 150));
            ImVec2 glow_min = ImVec2(p_min.x - 2.0f * zoom, p_min.y - 2.0f * zoom);
            ImVec2 glow_max = ImVec2(p_max.x + 2.0f * zoom, p_max.y + 2.0f * zoom);
            draw_list->AddRect(glow_min, glow_max, glow_color, rounding, 0, (thickness + 2.0f) * zoom);
        }

        draw_list->AddRectFilled(p_min, p_max, bg_color, rounding);
        draw_list->AddRect(p_min, p_max, border_color, rounding, 0, thickness * zoom);

        ImVec2 text_size = ImGui::CalcTextSize(name.c_str());
        ImVec2 text_pos = ImVec2((p_min.x + p_max.x - text_size.x) * 0.5f, (p_min.y + p_max.y - text_size.y) * 0.5f);
        draw_list->AddText(text_pos, IM_COL32(240, 240, 250, 255), name.c_str());
    };

    auto draw_arrow = [&](ImVec2 local_start, ImVec2 local_end, const std::string& trigger, bool has_bezier = false,
                          ImVec2 local_cp = ImVec2(0, 0)) {
        ImVec2 p_start = local_to_screen(local_start);
        ImVec2 p_end = local_to_screen(local_end);

        ImU32 color = IM_COL32(160, 160, 170, 255);
        float thickness = 2.0f * zoom;
        float arrow_size = 8.0f * zoom;

        ImVec2 last_segment_start;
        if (has_bezier)
        {
            ImVec2 p_cp = local_to_screen(local_cp);
            const int steps = 20;
            ImVec2 points[steps + 1];
            for (int i = 0; i <= steps; ++i)
            {
                float t = (float)i / steps;
                float u = 1.0f - t;
                points[i] = ImVec2(u * u * p_start.x + 2.0f * u * t * p_cp.x + t * t * p_end.x,
                                   u * u * p_start.y + 2.0f * u * t * p_cp.y + t * t * p_end.y);
            }
            draw_list->AddPolyline(points, steps + 1, color, 0, thickness);
            last_segment_start = points[steps - 1];
        }
        else
        {
            draw_list->AddLine(p_start, p_end, color, thickness);
            last_segment_start = p_start;
        }

        ImVec2 dir = ImVec2(p_end.x - last_segment_start.x, p_end.y - last_segment_start.y);
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0.0001f)
        {
            dir.x /= len;
            dir.y /= len;
            ImVec2 norm = ImVec2(-dir.y, dir.x);
            ImVec2 p_left = ImVec2(p_end.x - dir.x * arrow_size + norm.x * arrow_size * 0.5f,
                                   p_end.y - dir.y * arrow_size + norm.y * arrow_size * 0.5f);
            ImVec2 p_right = ImVec2(p_end.x - dir.x * arrow_size - norm.x * arrow_size * 0.5f,
                                    p_end.y - dir.y * arrow_size - norm.y * arrow_size * 0.5f);
            draw_list->AddTriangleFilled(p_end, p_left, p_right, color);
        }

        if (!trigger.empty())
        {
            ImVec2 mid;
            if (has_bezier)
            {
                ImVec2 p_cp = local_to_screen(local_cp);
                mid = ImVec2(0.25f * p_start.x + 0.5f * p_cp.x + 0.25f * p_end.x,
                             0.25f * p_start.y + 0.5f * p_cp.y + 0.25f * p_end.y);
            }
            else
            {
                mid = ImVec2((p_start.x + p_end.x) * 0.5f, (p_start.y + p_end.y) * 0.5f);
            }

            ImVec2 text_size = ImGui::CalcTextSize(trigger.c_str());
            ImVec2 text_rect_min =
                ImVec2(mid.x - text_size.x * 0.5f - 4.0f * zoom, mid.y - text_size.y * 0.5f - 2.0f * zoom);
            ImVec2 text_rect_max =
                ImVec2(mid.x + text_size.x * 0.5f + 4.0f * zoom, mid.y + text_size.y * 0.5f + 2.0f * zoom);
            draw_list->AddRectFilled(text_rect_min, text_rect_max, IM_COL32(20, 20, 25, 220), 2.0f * zoom);
            draw_list->AddText(ImVec2(mid.x - text_size.x * 0.5f, mid.y - text_size.y * 0.5f),
                               IM_COL32(180, 180, 190, 255), trigger.c_str());
        }
    };

    ImGui::SetWindowFontScale(zoom);

    auto activeStates = sm.GetStates();
    if (activeStates.empty())
    {
        draw_node("No active states", ImVec2(380, 220), ImVec2(240, 60), false);
    }
    else
    {
        // Draw push transitions (arrows) first so they are behind nodes
        for (size_t i = 0; i < activeStates.size() - 1; ++i)
        {
            ImVec2 start(100.0f + i * 260.0f + 180.0f, 250.0f);
            ImVec2 end(100.0f + (i + 1) * 260.0f, 250.0f);
            draw_arrow(start, end, "Push");
        }

        // Draw active nodes
        for (size_t i = 0; i < activeStates.size(); ++i)
        {
            State* s = activeStates[i];
            std::string name = "UnknownState";
            if (s)
            {
                std::string rawName = typeid(*s).name();
                if (rawName.rfind("class ", 0) == 0)
                {
                    name = rawName.substr(6);
                }
                else
                {
                    name = rawName;
                }
            }
            bool isActive = (i == activeStates.size() - 1);
            draw_node(name, ImVec2(100.0f + i * 260.0f, 220.0f), ImVec2(180.0f, 60.0f), isActive);
        }
    }

    ImGui::SetWindowFontScale(1.0f);
    ImGui::EndChild();

    ImGui::End();
}
#endif
