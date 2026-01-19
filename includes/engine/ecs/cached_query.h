#pragma once

#include <entt/entt.hpp>
#include <vector>
#include <functional>

template<typename... Components>
class CachedQuery
{
public:
    using UpdateCallback = std::function<void(entt::registry&)>;

    CachedQuery()
        : m_Dirty(true)
    {
    }

    void Update(entt::registry& registry)
    {
        if (!m_Dirty)
            return;

        m_CachedEntities.clear();
        
        auto view = registry.view<Components...>();
        for (auto entity : view)
        {
            m_CachedEntities.push_back(entity);
        }
        
        m_Dirty = false;
    }

    void MarkDirty()
    {
        m_Dirty = true;
    }

    bool IsDirty() const
    {
        return m_Dirty;
    }

    const std::vector<entt::entity>& GetEntities() const
    {
        return m_CachedEntities;
    }

    size_t GetEntityCount() const
    {
        return m_CachedEntities.size();
    }

    void Clear()
    {
        m_CachedEntities.clear();
        m_Dirty = true;
    }

private:
    std::vector<entt::entity> m_CachedEntities;
    bool m_Dirty;
};
