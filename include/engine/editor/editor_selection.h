#pragma once

#include <entt/entity/entity.hpp>
#include <span>
#include <vector>

struct Scene;

enum class ParentChildSelectionPolicy
{
    Independent,
    CollapseToRoots,
    IncludeDescendants
};

// Central editor selection model shared by hierarchy, viewport, inspector and tools.
class EditorSelection
{
public:
    entt::entity GetPrimary() const;
    std::span<const entt::entity> GetAll() const;
    bool Contains(entt::entity entity) const;
    bool Empty() const;

    void Clear();
    void Select(Scene& scene, entt::entity entity);
    void Toggle(Scene& scene, entt::entity entity);
    void Set(Scene& scene, std::vector<entt::entity> entities, entt::entity primary = entt::null);
    void PruneInvalid(Scene& scene);

    ParentChildSelectionPolicy GetParentChildPolicy() const;
    void SetParentChildPolicy(Scene& scene, ParentChildSelectionPolicy policy);

private:
    void PublishChanged() const;
    void ApplyParentChildPolicy(Scene& scene, std::vector<entt::entity>& entities) const;

    std::vector<entt::entity> m_Entities;
    entt::entity m_Primary = entt::null;
    ParentChildSelectionPolicy m_ParentChildPolicy = ParentChildSelectionPolicy::CollapseToRoots;
};
