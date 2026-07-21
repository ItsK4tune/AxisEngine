#include <scene/logic/octree.h>
#include <scene/type/scene_types.h>
#include <algorithm>

OctreeNode::OctreeNode(const AABB& boundary, int depth) : m_Boundary(boundary), m_Depth(depth)
{
    for (int i = 0; i < 8; ++i) m_Children[i] = nullptr;
}

void OctreeNode::Subdivide()
{
    glm::vec3 center = m_Boundary.GetCenter();
    glm::vec3 min = m_Boundary.minBound;
    glm::vec3 max = m_Boundary.maxBound;

    m_Children[0] = std::make_unique<OctreeNode>(AABB(min, center), m_Depth + 1);
    m_Children[1] = std::make_unique<OctreeNode>(
        AABB(glm::vec3(center.x, min.y, min.z), glm::vec3(max.x, center.y, center.z)), m_Depth + 1);
    m_Children[2] = std::make_unique<OctreeNode>(
        AABB(glm::vec3(min.x, center.y, min.z), glm::vec3(center.x, max.y, center.z)), m_Depth + 1);
    m_Children[3] = std::make_unique<OctreeNode>(
        AABB(glm::vec3(center.x, center.y, min.z), glm::vec3(max.x, max.y, center.z)), m_Depth + 1);
    m_Children[4] = std::make_unique<OctreeNode>(
        AABB(glm::vec3(min.x, min.y, center.z), glm::vec3(center.x, center.y, max.z)), m_Depth + 1);
    m_Children[5] = std::make_unique<OctreeNode>(
        AABB(glm::vec3(center.x, min.y, center.z), glm::vec3(max.x, center.y, max.z)), m_Depth + 1);
    m_Children[6] = std::make_unique<OctreeNode>(
        AABB(glm::vec3(min.x, center.y, center.z), glm::vec3(center.x, max.y, max.z)), m_Depth + 1);
    m_Children[7] = std::make_unique<OctreeNode>(AABB(center, max), m_Depth + 1);

    std::vector<OctreeElement> remaining;
    for (const auto& el : m_Elements)
    {
        int index = GetChildIndex(el.aabb);
        if (index != -1)
            m_Children[index]->Insert(el.entity, el.aabb);
        else
            remaining.push_back(el);
    }
    m_Elements = std::move(remaining);
}

int OctreeNode::GetChildIndex(const AABB& itemAABB) const
{
    glm::vec3 center = m_Boundary.GetCenter();

    bool west = itemAABB.maxBound.x < center.x;
    bool east = itemAABB.minBound.x > center.x;
    bool bottom = itemAABB.maxBound.y < center.y;
    bool top = itemAABB.minBound.y > center.y;
    bool front = itemAABB.maxBound.z < center.z;
    bool back = itemAABB.minBound.z > center.z;

    if (!west && !east)
        return -1;
    if (!bottom && !top)
        return -1;
    if (!front && !back)
        return -1;

    int index = 0;
    if (east)
        index |= 1;
    if (top)
        index |= 2;
    if (back)
        index |= 4;

    return index;
}

void OctreeNode::Insert(entt::entity entity, const AABB& aabb)
{
    if (IsLeaf())
    {
        if (m_Elements.size() < scene::OCTREE_MAX_ELEMENTS || m_Depth >= scene::OCTREE_MAX_DEPTH)
        {
            m_Elements.push_back({entity, aabb});
            return;
        }
        Subdivide();
    }

    int index = GetChildIndex(aabb);
    if (index != -1)
        m_Children[index]->Insert(entity, aabb);
    else
        m_Elements.push_back({entity, aabb});
}

void OctreeNode::Remove(entt::entity entity)
{
    auto it = std::remove_if(m_Elements.begin(), m_Elements.end(),
                             [entity](const OctreeElement& el) { return el.entity == entity; });
    m_Elements.erase(it, m_Elements.end());

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i) m_Children[i]->Remove(entity);
    }
}

void OctreeNode::Query(const Frustum& frustum, std::vector<entt::entity>& out_entities, bool fullyInside) const
{
    if (fullyInside)
    {
        for (const auto& el : m_Elements)
        {
            out_entities.push_back(el.entity);
        }
        if (!IsLeaf())
        {
            for (int i = 0; i < 8; ++i) m_Children[i]->Query(frustum, out_entities, true);
        }
        return;
    }

    int state = frustum.ContainsBoxState(m_Boundary);
    if (state == 0) // Outside
        return;

    bool nextFullyInside = (state == 2); // 2: Fully Inside

    for (const auto& el : m_Elements)
    {
        if (nextFullyInside || frustum.IsBoxVisible(el.aabb))
            out_entities.push_back(el.entity);
    }

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i) m_Children[i]->Query(frustum, out_entities, nextFullyInside);
    }
}

void OctreeNode::Query(const AABB& aabb, std::vector<entt::entity>& out_entities, bool fullyInside) const
{
    if (fullyInside)
    {
        for (const auto& el : m_Elements)
        {
            out_entities.push_back(el.entity);
        }
        if (!IsLeaf())
        {
            for (int i = 0; i < 8; ++i) m_Children[i]->Query(aabb, out_entities, true);
        }
        return;
    }

    if (!m_Boundary.Overlaps(aabb))
        return;

    bool nextFullyInside = (m_Boundary.minBound.x >= aabb.minBound.x && m_Boundary.maxBound.x <= aabb.maxBound.x) &&
                           (m_Boundary.minBound.y >= aabb.minBound.y && m_Boundary.maxBound.y <= aabb.maxBound.y) &&
                           (m_Boundary.minBound.z >= aabb.minBound.z && m_Boundary.maxBound.z <= aabb.maxBound.z);

    for (const auto& el : m_Elements)
    {
        if (nextFullyInside || el.aabb.Overlaps(aabb))
            out_entities.push_back(el.entity);
    }

    if (!IsLeaf())
    {
        for (int i = 0; i < 8; ++i) m_Children[i]->Query(aabb, out_entities, nextFullyInside);
    }
}

void OctreeNode::Rebuild(const std::vector<OctreeElement>& elements)
{
    m_Elements.clear();
    for (int i = 0; i < 8; ++i) m_Children[i].reset();

    for (const auto& el : elements)
    {
        Insert(el.entity, el.aabb);
    }
}

Octree::Octree(const AABB& boundary) : m_InitialBoundary(boundary), m_CurrentBoundary(boundary)
{
    m_Root = std::make_unique<OctreeNode>(boundary);
}

void Octree::Insert(entt::entity entity, const AABB& aabb)
{
    if (const auto existing = m_Entries.find(entity); existing != m_Entries.end())
        m_Root->Remove(entity);
    m_Entries[entity] = aabb;
    if (!RootContains(aabb))
    {
        RebuildFromEntries();
        return;
    }
    m_Root->Insert(entity, aabb);
}

void Octree::Remove(entt::entity entity)
{
    m_Entries.erase(entity);
    m_Root->Remove(entity);
}

void Octree::Query(const Frustum& frustum, std::vector<entt::entity>& out_entities) const
{
    m_Root->Query(frustum, out_entities);
}

void Octree::Query(const AABB& aabb, std::vector<entt::entity>& out_entities) const
{
    m_Root->Query(aabb, out_entities);
}

void Octree::Rebuild(const std::vector<OctreeElement>& elements)
{
    m_Entries.clear();
    m_Entries.reserve(elements.size());
    for (const auto& element : elements)
        m_Entries[element.entity] = element.aabb;
    RebuildFromEntries();
}

bool Octree::RootContains(const AABB& aabb) const
{
    return m_CurrentBoundary.Contains(aabb.minBound) && m_CurrentBoundary.Contains(aabb.maxBound);
}

void Octree::RebuildFromEntries()
{
    if (m_Entries.empty())
    {
        m_CurrentBoundary = m_InitialBoundary;
        m_Root = std::make_unique<OctreeNode>(m_InitialBoundary);
        return;
    }

    auto first = m_Entries.begin();
    glm::vec3 minBound = first->second.minBound;
    glm::vec3 maxBound = first->second.maxBound;
    std::vector<OctreeElement> elements;
    elements.reserve(m_Entries.size());
    for (const auto& [entity, aabb] : m_Entries)
    {
        minBound = glm::min(minBound, aabb.minBound);
        maxBound = glm::max(maxBound, aabb.maxBound);
        elements.push_back({entity, aabb});
    }

    // Keep a small loose margin so sub-pixel/interpolation changes do not
    // force an object onto the root boundary on every rebuild.
    const glm::vec3 center = (minBound + maxBound) * 0.5f;
    const float halfExtent = (std::max)({maxBound.x - minBound.x, maxBound.y - minBound.y,
                                         maxBound.z - minBound.z}) *
                                 0.525f +
                             1.0f;
    m_CurrentBoundary = AABB(center - glm::vec3(halfExtent), center + glm::vec3(halfExtent));
    m_Root = std::make_unique<OctreeNode>(m_CurrentBoundary);
    m_Root->Rebuild(elements);
}
