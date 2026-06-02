#pragma once

#include <entt/entt.hpp>
#include <functional>
#include <vector>

template <typename... Components>
class CachedQuery
{
public:
    using UpdateCallback = std::function<void(entt::registry&)>;

    CachedQuery() : m_Dirty(true), m_SignalsConnected(false)
    {
    }

    ~CachedQuery()
    {
        DisconnectSignals();
    }

    void Update(entt::registry& registry)
    {
        if (m_Registry != &registry)
        {
            DisconnectSignals();
            m_Registry = &registry;
            m_Dirty = true;
        }

        if (!m_SignalsConnected && m_Registry)
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

    void DisconnectSignals()
    {
        if (!m_Registry || !m_SignalsConnected)
            return;

        (m_Registry->on_construct<Components>().template disconnect<&CachedQuery::OnComponentAddedOrRemoved>(this),
         ...);
        (m_Registry->on_destroy<Components>().template disconnect<&CachedQuery::OnComponentAddedOrRemoved>(this),
         ...);
        m_SignalsConnected = false;
    }

    std::vector<entt::entity> m_CachedEntities;
    entt::registry* m_Registry = nullptr;
    bool m_Dirty;
    bool m_SignalsConnected;
};
