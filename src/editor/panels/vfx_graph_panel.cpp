#include <editor/panels/vfx_graph_panel.h>
#ifdef ENABLE_EDITOR
#include <ecs/logic/effect_graph_runtime.h>
#include <ecs/unit/media_components.h>
#include <editor/panels/scene_hierarchy_panel.h>
#include <scene/logic/scene.h>
#include <imgui.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cmath>
#include <unordered_map>

namespace
{
const char* NodeTypeName(VFXNodeType type)
{
    static const char* names[] = {"Spawn", "Lifetime", "Velocity", "Gravity", "Drag", "Color Over Life",
                                  "Size Over Life", "Output"};
    const int value = static_cast<int>(type);
    return value >= 0 && value < 8 ? names[value] : "Unknown";
}

VFXGraphNode MakeNode(uint32_t id, VFXNodeType type, size_t index)
{
    VFXGraphNode node;
    node.id = id;
    node.type = type;
    node.name = NodeTypeName(type);
    node.editorPosition = glm::vec2(35.0f + index * 28.0f, 45.0f + index * 35.0f);
    switch (type)
    {
        case VFXNodeType::Spawn: node.scalarA = 10.0f; break;
        case VFXNodeType::Lifetime: node.scalarA = 2.0f; break;
        case VFXNodeType::Velocity:
            node.valueA = glm::vec4(-0.1f, 1.0f, -0.1f, 0.0f);
            node.valueB = glm::vec4(0.1f, 4.0f, 0.1f, 0.0f);
            break;
        case VFXNodeType::Gravity: node.valueA = glm::vec4(0.0f, -9.81f, 0.0f, 0.0f); break;
        case VFXNodeType::Drag: node.scalarA = 0.0f; break;
        case VFXNodeType::ColorOverLife:
            node.valueA = glm::vec4(1.0f);
            node.valueB = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
            break;
        case VFXNodeType::SizeOverLife:
            node.scalarA = 1.0f;
            node.scalarB = 0.0f;
            break;
        case VFXNodeType::Output: break;
    }
    return node;
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

void VFXGraphPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    const entt::entity entity = SceneHierarchyPanel::s_SelectedEntity;
    if (entity == entt::null || !scene.GetRegistry().valid(entity))
    {
        ImGui::TextDisabled("Select an entity with a Particle Emitter component.");
        ImGui::End();
        return;
    }
    auto* particles = scene.GetRegistry().try_get<ParticleEmitterComponent>(entity);
    if (!particles)
    {
        ImGui::TextDisabled("Selected entity has no Particle Emitter component.");
        ImGui::End();
        return;
    }

    auto& graph = particles->graph;
    ImGui::Checkbox("Use VFX Graph", &graph.enabled);
    DrawHelp("Enabled: active graph nodes write their values to the particle emitter. Disabled: the emitter keeps its legacy inspector values.");
    ImGui::SameLine();
    static int newNodeType = 0;
    ImGui::SetNextItemWidth(145.0f);
    ImGui::Combo("##newVfxNode", &newNodeType,
                 "Spawn\0Lifetime\0Velocity\0Gravity\0Drag\0Color Over Life\0Size Over Life\0Output\0");
    ImGui::SameLine();
    if (ImGui::Button("Add Node"))
    {
        graph.nodes.push_back(MakeNode(graph.nextId++, static_cast<VFXNodeType>(newNodeType), graph.nodes.size()));
        m_SelectedNode = graph.nodes.back().id;
        m_SelectedLink = 0;
    }
    ImGui::SameLine();
    if (ImGui::Button("Add Parameter"))
        graph.parameters.push_back({"parameter" + std::to_string(graph.parameters.size() + 1),
                                    AnimationParameterType::Float, 0.0f, false, false});

    static uint32_t linkFrom = 0;
    static uint32_t linkTo = 0;
    auto nodeCombo = [&](const char* label, uint32_t& value) {
        const auto selected = std::find_if(graph.nodes.begin(), graph.nodes.end(),
                                           [value](const auto& node) { return node.id == value; });
        const char* preview = selected == graph.nodes.end() ? "None" : selected->name.c_str();
        ImGui::SetNextItemWidth(115.0f);
        if (ImGui::BeginCombo(label, preview))
        {
            for (const auto& node : graph.nodes)
                if (ImGui::Selectable((node.name + "##" + std::to_string(node.id)).c_str(), node.id == value))
                    value = node.id;
            ImGui::EndCombo();
        }
    };
    ImGui::SameLine();
    nodeCombo("From", linkFrom);
    ImGui::SameLine();
    nodeCombo("To", linkTo);
    ImGui::SameLine();
    if (ImGui::Button("Link") && linkFrom != 0 && linkTo != 0 && linkFrom != linkTo)
    {
        const bool duplicate = std::any_of(graph.links.begin(), graph.links.end(), [&](const auto& link) {
            return link.fromNode == linkFrom && link.toNode == linkTo;
        });
        if (!duplicate)
        {
            graph.links.push_back({graph.nextId++, linkFrom, linkTo});
            m_SelectedLink = graph.links.back().id;
            m_SelectedNode = 0;
        }
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
    ImGui::BeginChild("VFXGraphCanvas", ImVec2(canvasWidth, 0), true,
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
    for (const auto& node : graph.nodes)
        centers[node.id] = ImVec2(origin.x + m_CanvasPanX + (node.editorPosition.x + 85.0f) * m_CanvasZoom,
                                  origin.y + m_CanvasPanY + (node.editorPosition.y + 28.0f) * m_CanvasZoom);
    for (const auto& link : graph.links)
    {
        const auto from = centers.find(link.fromNode);
        const auto to = centers.find(link.toNode);
        if (from != centers.end() && to != centers.end())
        {
            const bool active = VFXGraphRuntime::ConditionsPass(graph, link);
            const ImU32 color = link.id == m_SelectedLink ? IM_COL32(255, 196, 64, 255)
                                  : active ? IM_COL32(105, 185, 225, 255) : IM_COL32(95, 95, 100, 255);
            draw->AddLine(from->second, to->second, color, link.id == m_SelectedLink ? 3.0f : 2.0f);
        }
    }

    for (auto& node : graph.nodes)
    {
        const ImVec2 nodePosition(origin.x + m_CanvasPanX + node.editorPosition.x * m_CanvasZoom,
                                  origin.y + m_CanvasPanY + node.editorPosition.y * m_CanvasZoom);
        ImGui::SetCursorScreenPos(nodePosition);
        ImGui::PushID(static_cast<int>(node.id));
        ImGui::InvisibleButton("node", ImVec2(170.0f * m_CanvasZoom, 56.0f * m_CanvasZoom));
        if (ImGui::IsItemClicked())
        {
            m_SelectedNode = node.id;
            m_SelectedLink = 0;
        }
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            const ImVec2 delta = ImGui::GetIO().MouseDelta;
            node.editorPosition.x += delta.x / m_CanvasZoom;
            node.editorPosition.y += delta.y / m_CanvasZoom;
        }
        const ImVec2 min = nodePosition;
        const ImVec2 max(min.x + 170.0f * m_CanvasZoom, min.y + 56.0f * m_CanvasZoom);
        ImU32 fill = node.id == m_SelectedNode ? IM_COL32(53, 96, 122, 255) : IM_COL32(45, 61, 70, 255);
        if (!VFXGraphRuntime::IsNodeActive(graph, node.id)) fill = IM_COL32(58, 58, 61, 255);
        draw->AddRectFilled(min, max, fill, 7.0f);
        draw->AddRect(min, max, node.type == VFXNodeType::Output ? IM_COL32(255, 190, 80, 255)
                                                                 : IM_COL32(105, 170, 195, 255),
                      7.0f, 0, 2.0f);
        draw->AddText(nullptr, ImGui::GetFontSize() * m_CanvasZoom,
                      ImVec2(min.x + 10.0f * m_CanvasZoom, min.y + 8.0f * m_CanvasZoom), IM_COL32_WHITE,
                      node.name.c_str());
        draw->AddText(nullptr, ImGui::GetFontSize() * m_CanvasZoom,
                      ImVec2(min.x + 10.0f * m_CanvasZoom, min.y + 32.0f * m_CanvasZoom),
                      IM_COL32(170, 205, 215, 255), NodeTypeName(node.type));
        ImGui::PopID();
    }
    draw->PopClipRect();
    ImGui::SetCursorScreenPos(ImVec2(origin.x + 8.0f, origin.y + 8.0f));
    ImGui::TextDisabled("Drag empty space: pan | Wheel: zoom | %.0f%%", m_CanvasZoom * 100.0f);
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::InvisibleButton("VFXGraphSplitter", ImVec2(6.0f, ImGui::GetContentRegionAvail().y));
    if (ImGui::IsItemActive())
        m_InspectorWidth = std::clamp(m_InspectorWidth - ImGui::GetIO().MouseDelta.x, 260.0f,
                                      std::max(260.0f, totalWidth - 220.0f));
    if (ImGui::IsItemHovered()) ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGui::SameLine();
    ImGui::BeginChild("VFXGraphInspector", ImVec2(0, 0), true);
    ImGui::TextUnformatted("Parameters");
    for (size_t index = 0; index < graph.parameters.size(); ++index)
    {
        auto& parameter = graph.parameters[index];
        ImGui::PushID(static_cast<int>(index));
        char parameterName[96];
        std::snprintf(parameterName, sizeof(parameterName), "%s", parameter.name.c_str());
        const std::string previousName = parameter.name;
        if (ImGui::InputText("##parameterName", parameterName, sizeof(parameterName)))
        {
            parameter.name = parameterName;
            for (auto& link : graph.links)
                for (auto& condition : link.conditions)
                    if (condition.parameter == previousName) condition.parameter = parameter.name;
        }
        ImGui::SameLine();
        int type = static_cast<int>(parameter.type);
        ImGui::SetNextItemWidth(75.0f);
        if (ImGui::Combo("##parameterType", &type, "Float\0Bool\0Trigger\0"))
            parameter.type = static_cast<AnimationParameterType>(type);
        if (parameter.type == AnimationParameterType::Float)
            ImGui::DragFloat("Value", &parameter.floatValue, 0.05f);
        else if (parameter.type == AnimationParameterType::Bool)
            ImGui::Checkbox("Value", &parameter.boolValue);
        else if (ImGui::Button(parameter.triggerValue ? "Triggered" : "Set Trigger"))
            parameter.triggerValue = !parameter.triggerValue;
        if (ImGui::SmallButton("Remove Parameter"))
        {
            const std::string removedName = parameter.name;
            graph.parameters.erase(graph.parameters.begin() + static_cast<std::ptrdiff_t>(index));
            for (auto& link : graph.links)
                std::erase_if(link.conditions,
                              [&removedName](const auto& condition) { return condition.parameter == removedName; });
            ImGui::PopID();
            break;
        }
        ImGui::Separator();
        ImGui::PopID();
    }
    auto selected = std::find_if(graph.nodes.begin(), graph.nodes.end(),
                                 [this](const auto& node) { return node.id == m_SelectedNode; });
    if (selected == graph.nodes.end())
    {
        ImGui::TextDisabled("Select a node to edit its module settings.");
    }
    else
    {
        auto& node = *selected;
        char name[96];
        std::snprintf(name, sizeof(name), "%s", node.name.c_str());
        if (ImGui::InputText("Name", name, sizeof(name))) node.name = name;
        ImGui::Checkbox("Node Enabled", &node.enabled);
        ImGui::Text("Module: %s", NodeTypeName(node.type));
        switch (node.type)
        {
            case VFXNodeType::Spawn: ImGui::DragFloat("Particles / second", &node.scalarA, 0.5f, 0.0f, 100000.0f); break;
            case VFXNodeType::Lifetime: ImGui::DragFloat("Lifetime", &node.scalarA, 0.05f, 0.001f, 300.0f); break;
            case VFXNodeType::Velocity:
                ImGui::DragFloat3("Minimum", &node.valueA.x, 0.05f);
                ImGui::DragFloat3("Maximum", &node.valueB.x, 0.05f);
                break;
            case VFXNodeType::Gravity: ImGui::DragFloat3("Acceleration", &node.valueA.x, 0.05f); break;
            case VFXNodeType::Drag: ImGui::DragFloat("Drag", &node.scalarA, 0.01f, 0.0f, 100.0f); break;
            case VFXNodeType::ColorOverLife:
                ImGui::ColorEdit4("Start", &node.valueA.x);
                ImGui::ColorEdit4("End", &node.valueB.x);
                break;
            case VFXNodeType::SizeOverLife:
                ImGui::DragFloat("Start", &node.scalarA, 0.01f, 0.0f, 1000.0f);
                ImGui::DragFloat("End", &node.scalarB, 0.01f, 0.0f, 1000.0f);
                break;
            case VFXNodeType::Output: ImGui::TextDisabled("Connect modules to this node to activate them."); break;
        }
        if (ImGui::Button("Delete Node"))
        {
            const uint32_t removed = node.id;
            graph.nodes.erase(selected);
            std::erase_if(graph.links, [removed](const auto& link) {
                return link.fromNode == removed || link.toNode == removed;
            });
            m_SelectedNode = 0;
            if (linkFrom == removed) linkFrom = 0;
            if (linkTo == removed) linkTo = 0;
        }
    }
    ImGui::Separator();
    ImGui::Text("Links: %d", static_cast<int>(graph.links.size()));
    for (size_t index = 0; index < graph.links.size(); ++index)
    {
        ImGui::PushID(static_cast<int>(index));
        const auto& listedLink = graph.links[index];
        const std::string linkLabel = std::to_string(listedLink.fromNode) + " -> " +
                                      std::to_string(listedLink.toNode) + "##link" +
                                      std::to_string(listedLink.id);
        if (ImGui::Selectable(linkLabel.c_str(), listedLink.id == m_SelectedLink))
        {
            m_SelectedLink = listedLink.id;
            m_SelectedNode = 0;
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Remove"))
        {
            const uint32_t removedLink = listedLink.id;
            graph.links.erase(graph.links.begin() + static_cast<std::ptrdiff_t>(index));
            if (m_SelectedLink == removedLink) m_SelectedLink = 0;
            ImGui::PopID();
            break;
        }
        ImGui::PopID();
    }
    auto selectedLink = std::find_if(graph.links.begin(), graph.links.end(),
                                     [this](const auto& link) { return link.id == m_SelectedLink; });
    if (selectedLink != graph.links.end())
    {
        ImGui::Separator();
        ImGui::Text("Link %u -> %u Conditions", selectedLink->fromNode, selectedLink->toNode);
        int logic = static_cast<int>(selectedLink->conditionLogic);
        if (ImGui::Combo("Condition Logic", &logic, "AND\0OR\0XOR\0NAND\0NOR\0XNOR\0"))
            selectedLink->conditionLogic = static_cast<GraphConditionLogic>(logic);
        DrawHelp("Only links whose condition group passes can activate upstream VFX nodes. NOT inverts an individual condition.");
        if (ImGui::Button("Add Condition") && !graph.parameters.empty())
            selectedLink->conditions.push_back(
                {graph.parameters.front().name, AnimationConditionOp::Greater, 0.0f});
        for (size_t index = 0; index < selectedLink->conditions.size(); ++index)
        {
            auto& condition = selectedLink->conditions[index];
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
                selectedLink->conditions.erase(selectedLink->conditions.begin() + static_cast<std::ptrdiff_t>(index));
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    ImGui::End();
}
#endif
