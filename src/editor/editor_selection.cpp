#include <editor/editor_selection.h>

#ifdef ENABLE_EDITOR

#include <core/logic/event_manager.h>
#include <core/type/event_types.h>
#include <ecs/unit/core_components.h>
#include <scene/logic/scene.h>

#include <algorithm>
#include <unordered_set>

entt::entity EditorSelection::GetPrimary() const
{
    return m_Primary;
}

std::span<const entt::entity> EditorSelection::GetAll() const
{
    return m_Entities;
}

bool EditorSelection::Contains(entt::entity entity) const
{
    return std::find(m_Entities.begin(), m_Entities.end(), entity) != m_Entities.end();
}

bool EditorSelection::Empty() const
{
    return m_Entities.empty();
}

void EditorSelection::Clear()
{
    if (m_Entities.empty() && m_Primary == entt::null)
        return;
    m_Entities.clear();
    m_Primary = entt::null;
    PublishChanged();
}

void EditorSelection::Select(Scene& scene, entt::entity entity)
{
    if (entity == entt::null || !scene.IsValid(entity))
    {
        Clear();
        return;
    }
    Set(scene, {entity}, entity);
}

void EditorSelection::Toggle(Scene& scene, entt::entity entity)
{
    if (entity == entt::null || !scene.IsValid(entity))
        return;

    auto entities = m_Entities;
    const auto it = std::find(entities.begin(), entities.end(), entity);
    if (it == entities.end())
    {
        entities.push_back(entity);
        Set(scene, std::move(entities), entity);
    }
    else
    {
        entities.erase(it);
        Set(scene, std::move(entities), m_Primary == entity ? entt::null : m_Primary);
    }
}

void EditorSelection::Set(Scene& scene, std::vector<entt::entity> entities, entt::entity primary)
{
    entities.erase(std::remove_if(entities.begin(), entities.end(), [&](entt::entity entity) {
                       return entity == entt::null || !scene.IsValid(entity);
                   }),
                   entities.end());
    std::sort(entities.begin(), entities.end(), [](entt::entity lhs, entt::entity rhs) {
        return entt::to_integral(lhs) < entt::to_integral(rhs);
    });
    entities.erase(std::unique(entities.begin(), entities.end()), entities.end());
    ApplyParentChildPolicy(scene, entities);

    if (primary == entt::null || std::find(entities.begin(), entities.end(), primary) == entities.end())
        primary = entities.empty() ? entt::null : entities.back();

    if (m_Entities == entities && m_Primary == primary)
        return;
    m_Entities = std::move(entities);
    m_Primary = primary;
    PublishChanged();
}

void EditorSelection::PruneInvalid(Scene& scene)
{
    Set(scene, m_Entities, m_Primary);
}

ParentChildSelectionPolicy EditorSelection::GetParentChildPolicy() const
{
    return m_ParentChildPolicy;
}

void EditorSelection::SetParentChildPolicy(Scene& scene, ParentChildSelectionPolicy policy)
{
    if (m_ParentChildPolicy == policy)
        return;
    m_ParentChildPolicy = policy;
    Set(scene, m_Entities, m_Primary);
}

void EditorSelection::PublishChanged() const
{
    EventManager::Instance().Publish(EntitySelectionChangedEvent{static_cast<uint32_t>(m_Primary)});
}

void EditorSelection::ApplyParentChildPolicy(Scene& scene, std::vector<entt::entity>& entities) const
{
    auto& registry = scene.GetRegistry();
    if (m_ParentChildPolicy == ParentChildSelectionPolicy::Independent)
        return;

    if (m_ParentChildPolicy == ParentChildSelectionPolicy::CollapseToRoots)
    {
        const std::unordered_set<entt::entity> selected(entities.begin(), entities.end());
        entities.erase(std::remove_if(entities.begin(), entities.end(), [&](entt::entity entity) {
                           entt::entity ancestor = entity;
                           while (const auto* hierarchy = registry.try_get<HierarchyComponent>(ancestor))
                           {
                               ancestor = hierarchy->parent;
                               if (ancestor == entt::null)
                                   break;
                               if (selected.contains(ancestor))
                                   return true;
                           }
                           return false;
                       }),
                       entities.end());
        return;
    }

    std::vector<entt::entity> pending = entities;
    while (!pending.empty())
    {
        const entt::entity entity = pending.back();
        pending.pop_back();
        if (const auto* hierarchy = registry.try_get<HierarchyComponent>(entity))
        {
            for (const entt::entity child : hierarchy->children)
            {
                if (!registry.valid(child) || std::find(entities.begin(), entities.end(), child) != entities.end())
                    continue;
                entities.push_back(child);
                pending.push_back(child);
            }
        }
    }
    std::sort(entities.begin(), entities.end(), [](entt::entity lhs, entt::entity rhs) {
        return entt::to_integral(lhs) < entt::to_integral(rhs);
    });
}

#endif
