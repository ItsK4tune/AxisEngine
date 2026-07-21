#include <editor/panels/state_panel.h>

#ifdef ENABLE_EDITOR
#include <core/app/runtime_core.h>
#include <core/logic/service_locator.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
struct GraphNode
{
    StateInfo info;
    ImVec2 pos = ImVec2(0.0f, 0.0f);
    ImVec2 size = ImVec2(220.0f, 72.0f);
};

struct GraphEdge
{
    int fromIndex = -1;
    int toIndex = -1;
    std::vector<std::string> labels;
    bool observed = false;
    bool registered = false;
};

std::string FitTextToWidth(const std::string& text, float maxWidth, float scale)
{
    if (text.empty() || ImGui::CalcTextSize(text.c_str()).x * scale <= maxWidth)
        return text;

    std::string trimmed = text;
    while (trimmed.size() > 3)
    {
        trimmed.pop_back();
        std::string candidate = trimmed + "...";
        if (ImGui::CalcTextSize(candidate.c_str()).x * scale <= maxWidth)
            return candidate;
    }
    return "...";
}

void AddUniqueLabel(std::vector<std::string>& labels, const std::string& label)
{
    if (std::find(labels.begin(), labels.end(), label) == labels.end())
        labels.push_back(label);
}

std::string JoinLabels(const std::vector<std::string>& labels)
{
    std::ostringstream oss;
    for (size_t i = 0; i < labels.size(); ++i)
    {
        if (i > 0)
            oss << ", ";
        oss << labels[i];
    }
    return oss.str();
}
}  // namespace

void StatePanel::OnImGui(Scene& scene)
{
    (void)scene;

    ImGui::Begin(GetTitle().c_str(), &m_Open);

    auto* core = ServiceLocator::Instance().Resolve<RuntimeCore>();
    if (!core)
    {
        ImGui::Text("RuntimeCore offline");
        ImGui::End();
        return;
    }

    auto& sm = core->GetStateMachine();
    auto activeStates = sm.GetStates();
    auto registeredStates = sm.GetRegisteredStates();
    auto transitions = sm.GetStateTransitions();

    std::string currentStateName = "None";
    if (State* currentState = sm.GetCurrentState())
        currentStateName = sm.GetStateName(*currentState);

    ImGui::Text("Current State: %s", currentStateName.c_str());
    ImGui::SameLine();
    bool resetView = ImGui::Button("Reset View");
    ImGui::SameLine();
    ImGui::TextDisabled("States: %d  Transitions: %d  Stack: %d", (int)registeredStates.size(), (int)transitions.size(),
                        (int)activeStates.size());

    std::vector<GraphNode> nodes;
    std::unordered_map<std::string, int> nodeIndex;
    std::unordered_set<std::string> activeStateNames;

    auto add_node = [&](const StateInfo& info) -> int {
        if (info.name.empty())
            return -1;

        auto it = nodeIndex.find(info.name);
        if (it != nodeIndex.end())
        {
            auto& existing = nodes[it->second].info;
            existing.explicitlyRegistered = existing.explicitlyRegistered || info.explicitlyRegistered;
            existing.observed = existing.observed || info.observed;
            existing.referencedByTransition = existing.referencedByTransition || info.referencedByTransition;
            return it->second;
        }

        int index = (int)nodes.size();
        nodeIndex.emplace(info.name, index);
        nodes.push_back({info});
        return index;
    };

    for (const auto& info : registeredStates) add_node(info);

    for (State* state : activeStates)
    {
        if (!state)
            continue;

        std::string name = sm.GetStateName(*state);
        activeStateNames.insert(name);
        add_node({name, false, true, false});
    }

    std::map<std::pair<int, int>, GraphEdge> edgeMap;
    for (const auto& transition : transitions)
    {
        int fromIndex = add_node({transition.from, false, false, true});
        int toIndex = add_node({transition.to, false, false, true});
        if (fromIndex < 0 || toIndex < 0)
            continue;

        auto key = std::make_pair(fromIndex, toIndex);
        auto& edge = edgeMap[key];
        edge.fromIndex = fromIndex;
        edge.toIndex = toIndex;
        edge.observed = edge.observed || transition.observed;
        edge.registered = edge.registered || !transition.observed;

        std::string label = transition.label.empty() ? StateTransitionKindName(transition.kind) : transition.label;
        AddUniqueLabel(edge.labels, label);
    }

    std::vector<GraphEdge> edges;
    edges.reserve(edgeMap.size());
    for (auto& [_, edge] : edgeMap) edges.push_back(edge);

    const int nodeCount = (int)nodes.size();
    const int columns = std::max(1, (int)std::ceil(std::sqrt((float)std::max(1, nodeCount))));
    const float xSpacing = 290.0f;
    const float ySpacing = 155.0f;
    for (int i = 0; i < nodeCount; ++i)
    {
        int row = i / columns;
        int col = i % columns;
        nodes[i].pos = ImVec2(col * xSpacing, row * ySpacing);
    }

    ImGui::BeginChild("StateMachineCanvas", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);

    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.x < 50.0f)
        canvas_size.x = 50.0f;
    if (canvas_size.y < 50.0f)
        canvas_size.y = 50.0f;
    ImVec2 canvas_end = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);

    ImVec2 scrolling(m_ScrollX, m_ScrollY);
    float& zoom = m_Zoom;

    if (m_FirstFrame || resetView)
    {
        float rows = (float)std::max(1, (nodeCount + columns - 1) / columns);
        float graphWidth = nodeCount == 0 ? 240.0f : (std::min(nodeCount, columns) - 1) * xSpacing + 220.0f;
        float graphHeight = nodeCount == 0 ? 72.0f : (rows - 1.0f) * ySpacing + 72.0f;
        scrolling = ImVec2((canvas_size.x - graphWidth * zoom) * 0.5f, (canvas_size.y - graphHeight * zoom) * 0.35f);
        m_FirstFrame = false;
    }

    if (ImGui::IsWindowHovered() && ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0f))
    {
        scrolling.x += ImGui::GetIO().MouseDelta.x;
        scrolling.y += ImGui::GetIO().MouseDelta.y;
    }

    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0.0f)
    {
        float prevZoom = zoom;
        zoom += ImGui::GetIO().MouseWheel * 0.05f;
        zoom = std::max(0.35f, std::min(zoom, 2.0f));

        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 mouse_in_canvas =
            ImVec2(mouse_pos.x - canvas_pos.x - scrolling.x, mouse_pos.y - canvas_pos.y - scrolling.y);
        scrolling.x -= mouse_in_canvas.x * (zoom / prevZoom - 1.0f);
        scrolling.y -= mouse_in_canvas.y * (zoom / prevZoom - 1.0f);
    }

    m_ScrollX = scrolling.x;
    m_ScrollY = scrolling.y;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRectFilled(canvas_pos, canvas_end, IM_COL32(20, 20, 25, 255));
    draw_list->PushClipRect(canvas_pos, canvas_end, true);

    float gridSize = 32.0f * zoom;
    for (float x = std::fmod(scrolling.x, gridSize); x < canvas_size.x; x += gridSize)
        draw_list->AddLine(ImVec2(canvas_pos.x + x, canvas_pos.y), ImVec2(canvas_pos.x + x, canvas_end.y),
                           IM_COL32(35, 35, 45, 255));
    for (float y = std::fmod(scrolling.y, gridSize); y < canvas_size.y; y += gridSize)
        draw_list->AddLine(ImVec2(canvas_pos.x, canvas_pos.y + y), ImVec2(canvas_end.x, canvas_pos.y + y),
                           IM_COL32(35, 35, 45, 255));

    auto local_to_screen = [&](ImVec2 local) -> ImVec2 {
        return ImVec2(canvas_pos.x + scrolling.x + local.x * zoom, canvas_pos.y + scrolling.y + local.y * zoom);
    };

    auto draw_arrow = [&](ImVec2 local_start, ImVec2 local_end, const std::string& label, ImU32 color,
                          bool has_bezier = false, ImVec2 local_cp = ImVec2(0, 0)) {
        ImVec2 p_start = local_to_screen(local_start);
        ImVec2 p_end = local_to_screen(local_end);

        float thickness = 2.0f * zoom;
        float arrow_size = 8.0f * zoom;
        ImVec2 last_segment_start = p_start;

        if (has_bezier)
        {
            ImVec2 p_cp = local_to_screen(local_cp);
            const int steps = 24;
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

        if (!label.empty())
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

            std::string displayLabel = FitTextToWidth(label, 190.0f * zoom, zoom);
            ImVec2 text_size = ImGui::CalcTextSize(displayLabel.c_str());
            text_size.x *= zoom;
            text_size.y *= zoom;
            ImVec2 text_rect_min =
                ImVec2(mid.x - text_size.x * 0.5f - 5.0f * zoom, mid.y - text_size.y * 0.5f - 3.0f * zoom);
            ImVec2 text_rect_max =
                ImVec2(mid.x + text_size.x * 0.5f + 5.0f * zoom, mid.y + text_size.y * 0.5f + 3.0f * zoom);
            draw_list->AddRectFilled(text_rect_min, text_rect_max, IM_COL32(20, 20, 25, 230), 3.0f * zoom);
            draw_list->AddText(ImGui::GetFont(), ImGui::GetFontSize() * zoom,
                               ImVec2(mid.x - text_size.x * 0.5f, mid.y - text_size.y * 0.5f),
                               IM_COL32(220, 220, 230, 255), displayLabel.c_str());
        }
    };

    auto draw_edge = [&](const GraphEdge& edge) {
        if (edge.fromIndex < 0 || edge.toIndex < 0 || edge.fromIndex >= nodeCount || edge.toIndex >= nodeCount)
            return;

        const auto& from = nodes[edge.fromIndex];
        const auto& to = nodes[edge.toIndex];
        ImU32 color = edge.observed ? IM_COL32(235, 190, 80, 255) : IM_COL32(120, 160, 210, 255);
        std::string label = JoinLabels(edge.labels);

        if (edge.fromIndex == edge.toIndex)
        {
            ImVec2 start(from.pos.x + from.size.x, from.pos.y + from.size.y * 0.35f);
            ImVec2 end(from.pos.x + from.size.x * 0.65f, from.pos.y);
            ImVec2 cp(from.pos.x + from.size.x + 95.0f, from.pos.y - 90.0f);
            draw_arrow(start, end, label, color, true, cp);
            return;
        }

        ImVec2 fromCenter(from.pos.x + from.size.x * 0.5f, from.pos.y + from.size.y * 0.5f);
        ImVec2 toCenter(to.pos.x + to.size.x * 0.5f, to.pos.y + to.size.y * 0.5f);
        float dx = toCenter.x - fromCenter.x;
        float dy = toCenter.y - fromCenter.y;

        ImVec2 start;
        ImVec2 end;
        ImVec2 cp;
        if (std::fabs(dx) >= std::fabs(dy))
        {
            start = ImVec2(dx >= 0.0f ? from.pos.x + from.size.x : from.pos.x, fromCenter.y);
            end = ImVec2(dx >= 0.0f ? to.pos.x : to.pos.x + to.size.x, toCenter.y);
            cp = ImVec2((start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f);
        }
        else
        {
            start = ImVec2(fromCenter.x, dy >= 0.0f ? from.pos.y + from.size.y : from.pos.y);
            end = ImVec2(toCenter.x, dy >= 0.0f ? to.pos.y : to.pos.y + to.size.y);
            cp = ImVec2((start.x + end.x) * 0.5f, (start.y + end.y) * 0.5f);
        }
        draw_arrow(start, end, label, color, false, cp);
    };

    auto draw_node = [&](const GraphNode& node) {
        ImVec2 p_min = local_to_screen(node.pos);
        ImVec2 p_max = local_to_screen(ImVec2(node.pos.x + node.size.x, node.pos.y + node.size.y));
        bool isCurrent = node.info.name == currentStateName;
        bool isActive = activeStateNames.find(node.info.name) != activeStateNames.end();

        ImU32 bg_color = isCurrent                        ? IM_COL32(45, 65, 105, 255)
                         : isActive                       ? IM_COL32(34, 48, 62, 255)
                         : node.info.explicitlyRegistered ? IM_COL32(32, 36, 45, 255)
                                                          : IM_COL32(28, 28, 34, 255);
        ImU32 border_color = isCurrent                        ? IM_COL32(255, 205, 70, 255)
                             : isActive                       ? IM_COL32(90, 185, 220, 255)
                             : node.info.explicitlyRegistered ? IM_COL32(105, 135, 190, 255)
                                                              : IM_COL32(65, 65, 75, 255);

        float rounding = 6.0f * zoom;
        float thickness = isCurrent ? 2.5f * zoom : 1.5f * zoom;

        if (isCurrent)
        {
            float t = (float)ImGui::GetTime();
            float glow = 0.5f + 0.5f * std::sin(t * 6.0f);
            draw_list->AddRect(ImVec2(p_min.x - 3.0f * zoom, p_min.y - 3.0f * zoom),
                               ImVec2(p_max.x + 3.0f * zoom, p_max.y + 3.0f * zoom),
                               IM_COL32(255, 205, 70, (int)(glow * 140)), rounding, 0, 2.0f * zoom);
        }

        draw_list->AddRectFilled(p_min, p_max, bg_color, rounding);
        draw_list->AddRect(p_min, p_max, border_color, rounding, 0, thickness);

        const float fontSize = ImGui::GetFontSize() * zoom;
        std::string name = FitTextToWidth(node.info.name, (node.size.x - 18.0f) * zoom, zoom);
        ImVec2 name_size = ImGui::CalcTextSize(name.c_str());
        name_size.x *= zoom;
        name_size.y *= zoom;

        std::string status = isCurrent                          ? "Current"
                             : isActive                         ? "Active"
                             : node.info.explicitlyRegistered   ? "Registered"
                             : node.info.referencedByTransition ? "Referenced"
                                                                : "Observed";
        ImVec2 status_size = ImGui::CalcTextSize(status.c_str());
        status_size.x *= zoom;
        status_size.y *= zoom;

        float textY = p_min.y + (node.size.y * zoom - name_size.y - status_size.y - 5.0f * zoom) * 0.5f;
        draw_list->AddText(ImGui::GetFont(), fontSize,
                           ImVec2(p_min.x + (node.size.x * zoom - name_size.x) * 0.5f, textY),
                           IM_COL32(240, 240, 250, 255), name.c_str());
        draw_list->AddText(
            ImGui::GetFont(), fontSize * 0.86f,
            ImVec2(p_min.x + (node.size.x * zoom - status_size.x) * 0.5f, textY + name_size.y + 5.0f * zoom),
            IM_COL32(165, 170, 185, 255), status.c_str());
    };

    if (nodes.empty())
    {
        GraphNode empty;
        empty.info.name = "No registered states";
        empty.pos = ImVec2(0.0f, 0.0f);
        empty.size = ImVec2(250.0f, 72.0f);
        draw_node(empty);
    }
    else
    {
        for (const auto& edge : edges) draw_edge(edge);
        for (const auto& node : nodes) draw_node(node);
    }

    draw_list->PopClipRect();
    ImGui::EndChild();

    ImGui::End();
}
#endif
