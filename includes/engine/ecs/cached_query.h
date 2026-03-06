#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <vector>

template<typename... Components>
class CachedQuery
{
public:
    using UpdateCallback = std::function<void(entt::registry&)>;

    CachedQuery()
        : m_Dirty(true), m_SignalsConnected(false)
    {
    }

    ~CachedQuery()
    {
        // We can't safely disconnect signals here generally without a registry pointer, 
        // but since CachedQuery is almost exclusively stored IN systems whose lifecycles 
        // match the registry, it's generally safe. However, to be perfectly clean, 
        // we omit auto-disconnect here. EnTT will clean up signals when the registry dies.
    }

    void Update(entt::registry& registry)
    {
        if (!m_SignalsConnected)
        {
            (registry.on_construct<Components>().template connect<&CachedQuery::OnComponentAddedOrRemoved>(this), ...);
            (registry.on_destroy<Components>().template connect<&CachedQuery::OnComponentAddedOrRemoved>(this), ...);
            m_SignalsConnected = true;
        }

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
    void OnComponentAddedOrRemoved(entt::registry& registry, entt::entity entity)
    {
        m_Dirty = true;
    }

    std::vector<entt::entity> m_CachedEntities;
    bool m_Dirty;
    bool m_SignalsConnected;
};
