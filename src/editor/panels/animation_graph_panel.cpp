#include <editor/panels/animation_graph_panel.h>
#ifdef ENABLE_EDITOR
#include <core/logic/service_locator.h>
#include <ecs/logic/effect_graph_runtime.h>
#include <ecs/unit/media_components.h>
#include <editor/editor_selection.h>
#include <editor/editor_system.h>
#include <scene/logic/scene.h>
#include <imgui.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cmath>
#include <unordered_map>

namespace
{
void CopyText(char* target, size_t size, const std::string& value)
{
    std::snprintf(target, size, "%s", value.c_str());
}

void DrawHelp(const char* text)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 30.0f);
        ImGui::TextUnformatted(text);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
}

void AnimationGraphPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    EditorSystem::BeginPanelTransactionOnContentClick(scene, "Animation edit");
    const entt::entity entity = ServiceLocator::Instance().Require<EditorSelection>().GetPrimary();
    if (entity == entt::null || !scene.GetRegistry().valid(entity))
    {
        ImGui::TextDisabled("Select an entity with an Animator component.");
        ImGui::End();
        return;
    }
    auto* animation = scene.GetRegistry().try_get<AnimationComponent>(entity);
    if (!animation)
    {
        ImGui::TextDisabled("Selected entity has no Animator component.");
        ImGui::End();
        return;
    }

    auto& graph = animation->graph;
    ImGui::Checkbox("Use State Machine", &graph.enabled);
    DrawHelp("Enabled: the runtime evaluates this graph and transitions between states. Disabled: the Animator uses its legacy/default animation playback.");
    ImGui::SameLine();
    if (ImGui::Button("Add State"))
    {
        const uint32_t id = graph.nextId++;
        const std::string clip = animation->animations.empty() ? std::string{} : animation->animations.front();
        graph.states.push_back({id, "State " + std::to_string(id), clip, 1.0f,
                                glm::vec2(40.0f + graph.states.size() * 35.0f, 60.0f + graph.states.size() * 25.0f)});
        if (graph.entryState == 0)
            graph.entryState = id;
        graph.activeState = 0;
        m_SelectedState = id;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Parameter"))
        graph.parameters.push_back({"parameter" + std::to_string(graph.parameters.size() + 1),
                                    AnimationParameterType::Float, 0.0f, false, false});
    ImGui::SameLine();
    if (ImGui::Button("Add Transition") && graph.states.size() >= 2)
    {
        graph.transitions.push_back(
            {graph.nextId++, graph.states[0].id, graph.states[1].id, 0.2f, false, 0.9f, {}});
        m_SelectedTransition = graph.transitions.back().id;
        m_SelectedState = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset View"))
    {
        m_CanvasZoom = 1.0f;
        m_CanvasPanX = 0.0f;
        m_CanvasPanY = 0.0f;
    }
    ImGui::Separator();

    const float totalWidth = ImGui::GetContentRegionAvail().x;
    m_InspectorWidth = std::clamp(m_InspectorWidth, 260.0f, std::max(260.0f, totalWidth - 220.0f));
    const float canvasWidth = std::max(210.0f, totalWidth - m_InspectorWidth - 8.0f);
    ImGui::BeginChild("AnimationGraphCanvas", ImVec2(canvasWidth, 0), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    const bool canvasHovered = ImGui::IsWindowHovered();
    ImGuiIO& io = ImGui::GetIO();
    if (canvasHovered && io.MouseWheel != 0.0f)
    {
        const float oldZoom = m_CanvasZoom;
        const float nextZoom = std::clamp(oldZoom * (io.MouseWheel > 0.0f ? 1.1f : 1.0f / 1.1f), 0.35f, 2.5f);
        const ImVec2 mouse(io.MousePos.x - origin.x, io.MousePos.y - origin.y);
        const float worldX = (mouse.x - m_CanvasPanX) / oldZoom;
        const float worldY = (mouse.y - m_CanvasPanY) / oldZoom;
        m_CanvasPanX = mouse.x - worldX * nextZoom;
        m_CanvasPanY = mouse.y - worldY * nextZoom;
        m_CanvasZoom = nextZoom;
    }
    const bool dragEmptyCanvas = canvasHovered && ImGui::IsMouseDragging(ImGuiMouseButton_Left) &&
                                 !ImGui::IsAnyItemActive();
    if (canvasHovered && (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || dragEmptyCanvas))
    {
        m_CanvasPanX += io.MouseDelta.x;
        m_CanvasPanY += io.MouseDelta.y;
    }
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), IM_COL32(24, 27, 33, 255));
    const float grid = 64.0f * m_CanvasZoom;
    const float gridX = std::fmod(m_CanvasPanX, grid);
    const float gridY = std::fmod(m_CanvasPanY, grid);
    for (float x = gridX; x < canvasSize.x; x += grid)
        draw->AddLine(ImVec2(origin.x + x, origin.y), ImVec2(origin.x + x, origin.y + canvasSize.y),
                      IM_COL32(43, 47, 56, 255));
    for (float y = gridY; y < canvasSize.y; y += grid)
        draw->AddLine(ImVec2(origin.x, origin.y + y), ImVec2(origin.x + canvasSize.x, origin.y + y),
                      IM_COL32(43, 47, 56, 255));
    draw->PushClipRect(origin, ImVec2(origin.x + canvasSize.x, origin.y + canvasSize.y), true);

    std::unordered_map<uint32_t, ImVec2> centers;
    for (const auto& state : graph.states)
        centers[state.id] = ImVec2(origin.x + m_CanvasPanX + (state.editorPosition.x + 90.0f) * m_CanvasZoom,
                                   origin.y + m_CanvasPanY + (state.editorPosition.y + 32.0f) * m_CanvasZoom);
    for (const auto& transition : graph.transitions)
    {
        auto from = centers.find(transition.fromState);
        auto to = centers.find(transition.toState);
        if (from == centers.end() || to == centers.end())
            continue;
        const ImU32 color = transition.id == m_SelectedTransition ? IM_COL32(255, 196, 64, 255)
                                                                  : IM_COL32(135, 155, 190, 255);
        draw->AddLine(from->second, to->second, color, transition.id == m_SelectedTransition ? 3.0f : 2.0f);
        const ImVec2 midpoint((from->second.x + to->second.x) * 0.5f, (from->second.y + to->second.y) * 0.5f);
        draw->AddCircleFilled(midpoint, 5.0f, color);
    }

    for (auto& state : graph.states)
    {
        const ImVec2 nodePosition(origin.x + m_CanvasPanX + state.editorPosition.x * m_CanvasZoom,
                                  origin.y + m_CanvasPanY + state.editorPosition.y * m_CanvasZoom);
        ImGui::SetCursorScreenPos(nodePosition);
        ImGui::PushID(static_cast<int>(state.id));
        ImGui::InvisibleButton("node", ImVec2(180.0f * m_CanvasZoom, 64.0f * m_CanvasZoom));
        if (ImGui::IsItemClicked())
        {
            m_SelectedState = state.id;
            m_SelectedTransition = 0;
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const ImVec2 delta = ImGui::GetIO().MouseDelta;
            state.editorPosition.x += delta.x / m_CanvasZoom;
            state.editorPosition.y += delta.y / m_CanvasZoom;
        }
        const ImVec2 min = nodePosition;
        const ImVec2 max(min.x + 180.0f * m_CanvasZoom, min.y + 64.0f * m_CanvasZoom);
        ImU32 fill = state.id == graph.activeState ? IM_COL32(35, 110, 70, 255) : IM_COL32(48, 56, 70, 255);
        if (state.id == m_SelectedState)
            fill = IM_COL32(64, 86, 125, 255);
        draw->AddRectFilled(min, max, fill, 7.0f);
        draw->AddRect(min, max, state.id == graph.entryState ? IM_COL32(255, 210, 75, 255)
                                                             : IM_COL32(115, 130, 155, 255),
                      7.0f, 0, state.id == graph.entryState ? 3.0f : 1.5f);
        draw->AddText(nullptr, ImGui::GetFontSize() * m_CanvasZoom,
                      ImVec2(min.x + 10.0f * m_CanvasZoom, min.y + 9.0f * m_CanvasZoom), IM_COL32_WHITE,
                      state.name.c_str());
        draw->AddText(nullptr, ImGui::GetFontSize() * m_CanvasZoom,
                      ImVec2(min.x + 10.0f * m_CanvasZoom, min.y + 35.0f * m_CanvasZoom),
                      IM_COL32(175, 190, 215, 255), state.clip.c_str());
        ImGui::PopID();
    }
    draw->PopClipRect();
    ImGui::SetCursorScreenPos(ImVec2(origin.x + 8.0f, origin.y + 8.0f));
    ImGui::TextDisabled("Drag empty space: pan | Wheel: zoom | %.0f%%", m_CanvasZoom * 100.0f);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::InvisibleButton("AnimationGraphSplitter", ImVec2(6.0f, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemActive())
        m_InspectorWidth = std::clamp(m_InspectorWidth - ImGui::GetIO().MouseDelta.x, 260.0f,
                                      std::max(260.0f, totalWidth - 220.0f));
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGui::SameLine();
    ImGui::BeginChild("AnimationGraphInspector", ImVec2(0, 0), true);
    ImGui::TextUnformatted("Parameters");
    for (size_t index = 0; index < graph.parameters.size(); ++index)
    {
        auto& parameter = graph.parameters[index];
        ImGui::PushID(static_cast<int>(index));
        char name[96];
        CopyText(name, sizeof(name), parameter.name);
        const std::string previousName = parameter.name;
        if (ImGui::InputText("##name", name, sizeof(name)))
        {
            parameter.name = name;
            for (auto& transition : graph.transitions)
                for (auto& condition : transition.conditions)
                    if (condition.parameter == previousName) condition.parameter = parameter.name;
        }
        ImGui::SameLine();
        int type = static_cast<int>(parameter.type);
        ImGui::SetNextItemWidth(75.0f);
        if (ImGui::Combo("##type", &type, "Float\0Bool\0Trigger\0"))
            parameter.type = static_cast<AnimationParameterType>(type);
        if (parameter.type == AnimationParameterType::Float)
            ImGui::DragFloat("Value", &parameter.floatValue, 0.05f);
        else if (parameter.type == AnimationParameterType::Bool)
            ImGui::Checkbox("Value", &parameter.boolValue);
        else if (ImGui::Button(parameter.triggerValue ? "Triggered" : "Set Trigger"))
            parameter.triggerValue = true;
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove"))
        {
            const std::string removedName = parameter.name;
            graph.parameters.erase(graph.parameters.begin() + static_cast<std::ptrdiff_t>(index));
            for (auto& transition : graph.transitions)
                std::erase_if(transition.conditions,
                              [&removedName](const auto& condition) { return condition.parameter == removedName; });
            ImGui::PopID();
            break;
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    ImGui::TextUnformatted("Transitions");
    for (const auto& transition : graph.transitions)
    {
        const auto* from = AnimationGraphRuntime::FindState(graph, transition.fromState);
        const auto* to = AnimationGraphRuntime::FindState(graph, transition.toState);
        const std::string label = (from ? from->name : "Missing") + std::string(" -> ") +
                                  (to ? to->name : "Missing") + "##" + std::to_string(transition.id);
        if (ImGui::Selectable(label.c_str(), transition.id == m_SelectedTransition))
        {
            m_SelectedTransition = transition.id;
            m_SelectedState = 0;
        }
    }
    ImGui::Separator();

    if (auto* state = AnimationGraphRuntime::FindState(graph, m_SelectedState))
    {
        ImGui::TextUnformatted("State");
        char name[128];
        CopyText(name, sizeof(name), state->name);
        if (ImGui::InputText("Name", name, sizeof(name))) state->name = name;
        if (ImGui::BeginCombo("Clip", state->clip.c_str()))
        {
            for (const auto& item : animation->animations)
                if (ImGui::Selectable(item.c_str(), item == state->clip)) state->clip = item;
            ImGui::EndCombo();
        }
        ImGui::DragFloat("State Speed", &state->speed, 0.05f, 0.0f, 10.0f);
        if (ImGui::Button("Set Entry"))
        {
            graph.entryState = state->id;
            graph.activeState = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button("Delete State"))
        {
            const uint32_t removed = state->id;
            std::erase_if(graph.states, [removed](const auto& item) { return item.id == removed; });
            std::erase_if(graph.transitions, [removed](const auto& item) {
                return item.fromState == removed || item.toState == removed;
            });
            if (graph.entryState == removed)
                graph.entryState = graph.states.empty() ? 0 : graph.states.front().id;
            graph.activeState = 0;
            m_SelectedState = 0;
        }
    }
    else
    {
        auto transitionIt = std::find_if(graph.transitions.begin(), graph.transitions.end(),
                                         [this](const auto& item) { return item.id == m_SelectedTransition; });
        if (transitionIt != graph.transitions.end())
        {
            auto& transition = *transitionIt;
            ImGui::TextUnformatted("Transition");
            auto drawStateCombo = [&](const char* label, uint32_t& value) {
                const auto* selected = AnimationGraphRuntime::FindState(graph, value);
                if (ImGui::BeginCombo(label, selected ? selected->name.c_str() : "None"))
                {
                    for (const auto& state : graph.states)
                        if (ImGui::Selectable(state.name.c_str(), state.id == value)) value = state.id;
                    ImGui::EndCombo();
                }
            };
            drawStateCombo("From", transition.fromState);
            drawStateCombo("To", transition.toState);
            ImGui::DragFloat("Blend Duration", &transition.duration, 0.01f, 0.0f, 10.0f);
            ImGui::Checkbox("Has Exit Time", &transition.hasExitTime);
            if (transition.hasExitTime)
                ImGui::SliderFloat("Exit Time", &transition.exitTime, 0.0f, 1.0f);
            int logic = static_cast<int>(transition.conditionLogic);
            if (ImGui::Combo("Condition Logic", &logic, "AND\0OR\0XOR\0NAND\0NOR\0XNOR\0"))
                transition.conditionLogic = static_cast<GraphConditionLogic>(logic);
            DrawHelp("Combines every condition in this transition. NOT can invert each individual condition.");
            if (ImGui::Button("Add Condition") && !graph.parameters.empty())
                transition.conditions.push_back({graph.parameters.front().name, AnimationConditionOp::Greater, 0.0f});
            for (size_t index = 0; index < transition.conditions.size(); ++index)
            {
                auto& condition = transition.conditions[index];
                ImGui::PushID(static_cast<int>(index));
                if (ImGui::BeginCombo("Parameter", condition.parameter.c_str()))
                {
                    for (const auto& parameter : graph.parameters)
                        if (ImGui::Selectable(parameter.name.c_str(), parameter.name == condition.parameter))
                            condition.parameter = parameter.name;
                    ImGui::EndCombo();
                }
                int op = static_cast<int>(condition.op);
                if (ImGui::Combo("Operator", &op, ">\0>=\0<\0<=\0==\0!=\0True\0False\0Triggered\0"))
                    condition.op = static_cast<AnimationConditionOp>(op);
                ImGui::Checkbox("NOT", &condition.negated);
                if (op <= static_cast<int>(AnimationConditionOp::NotEqual))
                    ImGui::DragFloat("Threshold", &condition.threshold, 0.05f);
                if (ImGui::SmallButton("Remove Condition"))
                {
                    transition.conditions.erase(transition.conditions.begin() + static_cast<std::ptrdiff_t>(index));
                    ImGui::PopID();
                    break;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Delete Transition"))
            {
                graph.transitions.erase(transitionIt);
                m_SelectedTransition = 0;
            }
        }
    }
    ImGui::EndChild();
    ImGui::End();
}
#endif
