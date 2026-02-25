#pragma once

#include <entt/entt.hpp>
#include <math/aabb.h>
#include <vector>
#include <memory>
#include <graphic/renderer/frustum.h>

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
    void Query(const Frustum& frustum, std::vector<entt::entity>& out_entities) const;
    
    void Rebuild(const std::vector<OctreeElement>& elements);

private:
    void Subdivide();
    bool IsLeaf() const { return m_Children[0] == nullptr; }
    int GetChildIndex(const AABB& itemAABB) const;

    AABB m_Boundary;
    int m_Depth;
    std::vector<OctreeElement> m_Elements;
    std::unique_ptr<OctreeNode> m_Children[8];

    static constexpr int MAX_ELEMENTS = 16;
    static constexpr int MAX_DEPTH = 6;
};

class Octree
{
public:
    Octree(const AABB& boundary);
    ~Octree() = default;

    void Insert(entt::entity entity, const AABB& aabb);
    void Remove(entt::entity entity);
    void Query(const Frustum& frustum, std::vector<entt::entity>& out_entities) const;
    
    void Rebuild(const std::vector<OctreeElement>& elements);

private:
    std::unique_ptr<OctreeNode> m_Root;
    AABB m_InitialBoundary;
};
