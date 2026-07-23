#include <editor/panels/navigation_panel.h>

#ifdef ENABLE_EDITOR

#include <core/logic/service_locator.h>
#include <ecs/unit/core_components.h>
#include <editor/editor_selection.h>
#include <editor/editor_system.h>
#include <navigation/logic/navmesh_generator.h>
#include <navigation/unit/navmesh_component.h>
#include <navigation/unit/pathfollower_component.h>
#include <resource/logic/resource_manager.h>
#include <scene/logic/scene.h>
#include <imgui.h>
#include <algorithm>
#include <array>
#include <cstring>

namespace
{
const char* ConditionName(NavigationRuleCondition condition)
{
    static constexpr const char* names[] = {
        "Always", "Has tag", "Height above", "Height below", "Uphill step", "Downhill step",
        "Slope above (deg)", "Slope below (deg)"};
    return names[static_cast<int>(condition)];
}

const char* EffectName(NavigationRuleEffect effect)
{
    static constexpr const char* names[] = {"Reward", "Penalty", "Block"};
    return names[static_cast<int>(effect)];
}

void RequestFollowerRepaths(Scene& scene, entt::entity provider)
{
    for (auto [entity, follower] : scene.View<PathFollowerComponent>().each())
    {
        if (follower.navigationProviderEntity != provider)
            continue;
        follower.pathPending = true;
        ++follower.pathRequestGeneration;
    }
}

}

void NavigationPanel::OnImGui(Scene& scene)
{
    ImGui::Begin(GetTitle().c_str(), &m_Open);
    auto view = scene.View<NavMeshComponent>();
    if (view.empty())
    {
        ImGui::TextDisabled("No NavMeshComponent in the scene.");
        ImGui::TextWrapped("Add a NavMesh component to a provider entity in Inspector.");
        ImGui::End();
        return;
    }

    if (m_SelectedProvider == entt::null || !scene.IsValid(m_SelectedProvider) ||
        !scene.HasAllComponents<NavMeshComponent>(m_SelectedProvider))
        m_SelectedProvider = view.front();

    const char* preview = "NavMesh provider";
    if (const auto* info = scene.TryGetComponent<InfoComponent>(m_SelectedProvider))
        preview = info->name.c_str();
    if (ImGui::BeginCombo("Provider", preview))
    {
        for (const entt::entity entity : view)
        {
            std::string name = "Entity " + std::to_string(entt::to_integral(entity));
            if (const auto* info = scene.TryGetComponent<InfoComponent>(entity))
                name = info->name;
            if (ImGui::Selectable(name.c_str(), entity == m_SelectedProvider))
            {
                m_SelectedProvider = entity;
                ServiceLocator::Instance().Require<EditorSelection>().Select(scene, entity);
            }
        }
        ImGui::EndCombo();
    }

    auto& navMesh = scene.GetComponent<NavMeshComponent>(m_SelectedProvider);
    bool changed = false;
    bool rulesChanged = false;
    if (ImGui::CollapsingHeader("NavMesh bake", ImGuiTreeNodeFlags_DefaultOpen))
    {
    changed |= ImGui::Checkbox("Dynamic rebuild", &navMesh.isDynamic);
    changed |= ImGui::DragInt("Terrain grid resolution", &navMesh.terrainGridResolution, 1.0f, 2, 1024);
    changed |= ImGui::SliderFloat("Walkable normal Y", &navMesh.walkableNormalY, -1.0f, 1.0f);
    changed |= ImGui::DragFloat("Carve height padding", &navMesh.carveHeightPadding, 0.05f, 0.0f, 100.0f);
    changed |= ImGui::DragFloat("Agent radius", &navMesh.carveAgentRadius, 0.05f, 0.0f, 100.0f);
    if (changed)
    {
        navMesh.terrainGridResolution = std::clamp(navMesh.terrainGridResolution, 2, 1024);
        navMesh.walkableNormalY = std::clamp(navMesh.walkableNormalY, -1.0f, 1.0f);
        navMesh.carveHeightPadding = std::max(0.0f, navMesh.carveHeightPadding);
        navMesh.carveAgentRadius = std::max(0.0f, navMesh.carveAgentRadius);
        navMesh.needsRebuild = true;
    }

        ImGui::Text("Vertices %zu  |  Triangles %zu  |  Nodes %zu", navMesh.vertices.size(),
                    navMesh.triangles.size(), navMesh.nodes.size());
        ImGui::Text("Revision %llu  |  %s", static_cast<unsigned long long>(navMesh.revision),
                    navMesh.needsRebuild ? "Needs bake" : "Ready");

        if (ImGui::Button("Bake now"))
        {
            EditorSystem::BeginTransaction(scene, "Bake NavMesh");
            auto* resources = ServiceLocator::Instance().Resolve<ResourceManager>();
            NavMeshGenerator::Generate(scene, navMesh, resources);
            m_Status = navMesh.nodes.empty() ? "Bake completed, but no walkable nodes were generated."
                                             : "NavMesh bake completed.";
        }
        ImGui::SameLine();
        if (ImGui::Button("Mark dirty"))
        {
            navMesh.needsRebuild = true;
            m_Status = "NavMesh queued for runtime rebuild.";
        }
    }

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Custom path cost graph", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped(
            "Rules run from top to bottom for A*. Matching rewards reduce travel cost, penalties increase it, "
            "and Block removes the edge. Followers must use the Custom criterion.");
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.0f, 1.0f),
                           "Stored on this NavMesh entity (%zu rules). Press Ctrl+S to save them with the scene.",
                           navMesh.costRules.size());

        ImGui::TextDisabled("Geometry tags may be combined, for example: walkable+road+flat");
        if (ImGui::Button("+ Prefer flat road"))
        {
            navMesh.costRules.push_back({"Prefer flat road", true, NavigationConditionGroupMode::All,
                                         {{NavigationRuleCondition::TagEquals, "road", 0.0f, false},
                                          {NavigationRuleCondition::SlopeBelow, "", 5.0f, false}},
                                         NavigationRuleEffect::Reward, 3.0f, false});
            rulesChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Avoid tag"))
        {
            navMesh.costRules.push_back({"Avoid danger", true, NavigationConditionGroupMode::Any,
                                         {{NavigationRuleCondition::TagEquals, "fire", 0.0f, false},
                                          {NavigationRuleCondition::TagEquals, "poison", 0.0f, false}},
                                         NavigationRuleEffect::Penalty, 5.0f, false});
            rulesChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Block wall"))
        {
            navMesh.costRules.push_back({"Block wall", true, NavigationConditionGroupMode::All,
                                         {{NavigationRuleCondition::TagEquals, "wall", 0.0f, false}},
                                         NavigationRuleEffect::Block, 0.0f, true});
            rulesChanged = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("+ Custom rule"))
        {
            navMesh.costRules.emplace_back();
            rulesChanged = true;
        }

        int removeIndex = -1;
        int moveFrom = -1;
        int moveTo = -1;
        for (size_t index = 0; index < navMesh.costRules.size(); ++index)
        {
            auto& rule = navMesh.costRules[index];
            ImGui::PushID(static_cast<int>(index));
            ImGui::Separator();
            ImGui::TextDisabled("%s", index == 0 ? "START" : "then");
            ImGui::SameLine();
            rulesChanged |= ImGui::Checkbox("##enabled", &rule.enabled);
            ImGui::SameLine();

            std::array<char, 128> name{};
            std::strncpy(name.data(), rule.name.c_str(), name.size() - 1);
            ImGui::SetNextItemWidth((std::max)(140.0f, ImGui::GetContentRegionAvail().x - 96.0f));
            if (ImGui::InputText("##name", name.data(), name.size()))
            {
                rule.name = name.data();
                rulesChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("up", ImGuiDir_Up) && index > 0)
            {
                moveFrom = static_cast<int>(index);
                moveTo = moveFrom - 1;
            }
            ImGui::SameLine();
            if (ImGui::ArrowButton("down", ImGuiDir_Down) && index + 1 < navMesh.costRules.size())
            {
                moveFrom = static_cast<int>(index);
                moveTo = moveFrom + 1;
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("X"))
                removeIndex = static_cast<int>(index);

            int effect = static_cast<int>(rule.effect);
            ImGui::SetNextItemWidth(170.0f);
            const char* groupModes[] = {"ALL conditions", "ANY condition"};
            int groupMode = static_cast<int>(rule.conditionMode);
            if (ImGui::Combo("Match", &groupMode, groupModes, IM_ARRAYSIZE(groupModes)))
            {
                rule.conditionMode = static_cast<NavigationConditionGroupMode>(groupMode);
                rulesChanged = true;
            }
            ImGui::SetNextItemWidth(170.0f);
            if (ImGui::BeginCombo("Result", EffectName(rule.effect)))
            {
                for (int value = 0; value < 3; ++value)
                    if (ImGui::Selectable(EffectName(static_cast<NavigationRuleEffect>(value)), effect == value))
                    {
                        rule.effect = static_cast<NavigationRuleEffect>(value);
                        rulesChanged = true;
                    }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            rulesChanged |= ImGui::Checkbox("Stop after match", &rule.stopOnMatch);

            int removeCondition = -1;
            for (size_t conditionIndex = 0; conditionIndex < rule.conditions.size(); ++conditionIndex)
            {
                auto& predicate = rule.conditions[conditionIndex];
                ImGui::PushID(static_cast<int>(conditionIndex));
                ImGui::Indent(16.0f);
                ImGui::TextDisabled("%s", conditionIndex == 0 ? "IF" :
                    (rule.conditionMode == NavigationConditionGroupMode::All ? "AND" : "OR"));
                ImGui::SameLine();
                int condition = static_cast<int>(predicate.condition);
                ImGui::SetNextItemWidth(175.0f);
                if (ImGui::BeginCombo("##condition", ConditionName(predicate.condition)))
                {
                    for (int value = 0; value < 8; ++value)
                    {
                        if (ImGui::Selectable(ConditionName(static_cast<NavigationRuleCondition>(value)),
                                              condition == value))
                        {
                            predicate.condition = static_cast<NavigationRuleCondition>(value);
                            rulesChanged = true;
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                rulesChanged |= ImGui::Checkbox("NOT", &predicate.negate);
                ImGui::SameLine();
                if (ImGui::SmallButton("Remove"))
                    removeCondition = static_cast<int>(conditionIndex);

                if (predicate.condition == NavigationRuleCondition::TagEquals)
                {
                    std::array<char, 128> tag{};
                    std::strncpy(tag.data(), predicate.tag.c_str(), tag.size() - 1);
                    ImGui::SetNextItemWidth(240.0f);
                    if (ImGui::InputTextWithHint("Tag", "road, flat, fire...", tag.data(), tag.size()))
                    {
                        predicate.tag = tag.data();
                        rulesChanged = true;
                    }
                }
                else if (predicate.condition != NavigationRuleCondition::Always)
                {
                    const float speed = predicate.condition == NavigationRuleCondition::SlopeAbove ||
                                                predicate.condition == NavigationRuleCondition::SlopeBelow
                                            ? 0.5f
                                            : 0.05f;
                    rulesChanged |= ImGui::DragFloat("Threshold", &predicate.threshold, speed);
                }
                ImGui::Unindent(16.0f);
                ImGui::PopID();
            }
            if (removeCondition >= 0)
            {
                rule.conditions.erase(rule.conditions.begin() + removeCondition);
                rulesChanged = true;
            }
            if (ImGui::SmallButton("+ Condition"))
            {
                rule.conditions.emplace_back();
                rulesChanged = true;
            }

            if (rule.effect != NavigationRuleEffect::Block)
            {
                rulesChanged |= ImGui::DragFloat("Strength", &rule.value, 0.05f, 0.0f, 1000.0f);
                rule.value = (std::max)(0.0f, rule.value);
                ImGui::SameLine();
                if (rule.effect == NavigationRuleEffect::Reward)
                    ImGui::TextDisabled("cost x %.3f", 1.0f / (1.0f + rule.value));
                else
                    ImGui::TextDisabled("cost x %.3f", 1.0f + rule.value);
            }
            else
                ImGui::TextDisabled("Matching edges are removed from A*.");
            ImGui::PopID();
        }
        if (removeIndex >= 0)
        {
            navMesh.costRules.erase(navMesh.costRules.begin() + removeIndex);
            rulesChanged = true;
        }
        if (moveFrom >= 0)
        {
            std::swap(navMesh.costRules[moveFrom], navMesh.costRules[moveTo]);
            rulesChanged = true;
        }

        ImGui::Separator();
        ImGui::TextDisabled("END -> A* path");
    }

    if (ImGui::CollapsingHeader("Assign NavMesh rules to agents", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::TextWrapped("Select PathFollower agents below. Apply assigns this NavMesh entity as provider and "
                           "enables its embedded Custom rules. Everything is saved in the scene.");
        if (ImGui::Button("Select provider followers"))
        {
            for (auto [entity, follower] : scene.View<PathFollowerComponent>().each())
                if (follower.navigationProviderEntity != m_SelectedProvider)
                    m_SelectedFollowers.erase(static_cast<uint32_t>(entt::to_integral(entity)));
                else
                    m_SelectedFollowers.insert(static_cast<uint32_t>(entt::to_integral(entity)));
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear selection"))
            m_SelectedFollowers.clear();

        if (ImGui::BeginTable("NavigationAgents", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
                                                       ImGuiTableFlags_ScrollY, ImVec2(0.0f, 220.0f)))
        {
            ImGui::TableSetupColumn("Use", ImGuiTableColumnFlags_WidthFixed, 42.0f);
            ImGui::TableSetupColumn("Agent");
            ImGui::TableSetupColumn("Provider");
            ImGui::TableSetupColumn("Cost mode");
            ImGui::TableHeadersRow();
            for (auto [entity, follower] : scene.View<PathFollowerComponent>().each())
            {
                const uint32_t id = static_cast<uint32_t>(entt::to_integral(entity));
                bool selectedFollower = m_SelectedFollowers.contains(id);
                std::string name = "Entity " + std::to_string(id);
                if (const auto* info = scene.TryGetComponent<InfoComponent>(entity))
                    name = info->name;
                std::string providerName = follower.navigationProviderName.empty()
                                               ? "(Auto)"
                                               : follower.navigationProviderName;
                ImGui::PushID(static_cast<int>(id));
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Checkbox("##agent", &selectedFollower))
                {
                    if (selectedFollower)
                        m_SelectedFollowers.insert(id);
                    else
                        m_SelectedFollowers.erase(id);
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(providerName.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(follower.pathfindingOptions.criteria == PathfindingCriteria::Custom
                                           ? "Custom"
                                           : "Other");
                ImGui::PopID();
            }
            ImGui::EndTable();
        }

        ImGui::BeginDisabled(m_SelectedFollowers.empty());
        if (ImGui::Button("Apply NavMesh rules to selected agents"))
        {
            EditorSystem::BeginTransaction(scene, "Assign navigation profile");
            size_t count = 0;
            std::string providerName;
            if (const auto* info = scene.TryGetComponent<InfoComponent>(m_SelectedProvider))
                providerName = info->name;
            for (auto [entity, follower] : scene.View<PathFollowerComponent>().each())
            {
                const uint32_t id = static_cast<uint32_t>(entt::to_integral(entity));
                if (!m_SelectedFollowers.contains(id))
                    continue;
                follower.navigationProviderEntity = m_SelectedProvider;
                follower.navigationProviderName = providerName;
                follower.pathfindingOptions.provider = NavigationProvider::NavMesh;
                follower.pathfindingOptions.criteria = PathfindingCriteria::Custom;
                follower.pathPending = true;
                ++follower.pathRequestGeneration;
                ++count;
            }
            m_Status = "NavMesh rules applied to " + std::to_string(count) +
                       " selected agent(s). Save the scene to persist.";
        }
        ImGui::EndDisabled();
    }

    if (rulesChanged)
        ++navMesh.revision;
    if (changed || rulesChanged)
        RequestFollowerRepaths(scene, m_SelectedProvider);
    if (!m_Status.empty())
        ImGui::TextWrapped("%s", m_Status.c_str());
    ImGui::End();
}

#endif
