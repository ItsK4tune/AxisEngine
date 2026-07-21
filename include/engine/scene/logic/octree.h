#pragma once

#include <core/unit/aabb.h>
#include <render/unit/frustum.h>
#include <scene/type/scene_types.h>
#include <entt/entt.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

struct OctreeElement
{
    entt::entity entity;
    AABB aabb;
};

class OctreeNode
{
public:
    OctreeNode(const AABB& boundary, int depth = 0);
    ~OctreeNode() = default;

    void Insert(entt::entity entity, const AABB& aabb);
    void Remove(entt::entity entity);
    void Query(const Frustum& frustum, std::vector<entt::entity>& out_entities, bool fullyInside = false) const;
    void Query(const AABB& aabb, std::vector<entt::entity>& out_entities, bool fullyInside = false) const;

    void Rebuild(const std::vector<OctreeElement>& elements);

private:
    void Subdivide();
    bool IsLeaf() const
    {
        return m_Children[0] == nullptr;
    }
    int GetChildIndex(const AABB& itemAABB) const;

    AABB m_Boundary;
    int m_Depth;
    std::vector<OctreeElement> m_Elements;
    std::unique_ptr<OctreeNode> m_Children[8];
};

class Octree
{
public:
    Octree(const AABB& boundary);
    ~Octree() = default;

    void Insert(entt::entity entity, const AABB& aabb);
    void Remove(entt::entity entity);
    void Query(const Frustum& frustum, std::vector<entt::entity>& out_entities) const;
    void Query(const AABB& aabb, std::vector<entt::entity>& out_entities) const;

    void Rebuild(const std::vector<OctreeElement>& elements);

private:
    void RebuildFromEntries();
    bool RootContains(const AABB& aabb) const;

    std::unique_ptr<OctreeNode> m_Root;
    AABB m_InitialBoundary;
    AABB m_CurrentBoundary;
    std::unordered_map<entt::entity, AABB> m_Entries;
};
