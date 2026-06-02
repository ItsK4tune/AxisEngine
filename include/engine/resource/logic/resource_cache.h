#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T>
class ResourceCache
{
public:
    using ResourcePtr = std::shared_ptr<T>;

    ResourceCache() = default;
    ~ResourceCache() = default;

    ResourcePtr Get(const std::string& name) const
    {
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        auto it = m_Cache.find(name);
        if (it != m_Cache.end())
        {
            return it->second;
        }
        return nullptr;
    }

    void Add(const std::string& name, ResourcePtr resource)
    {
        if (!resource)
            return;
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Cache[name] = resource;
    }

    void Remove(const std::string& name)
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Cache.erase(name);
    }

    bool Has(const std::string& name) const
    {
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        return m_Cache.find(name) != m_Cache.end();
    }

    void Clear()
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Cache.clear();
    }

    std::vector<std::string> GetAllNames() const
    {
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        std::vector<std::string> names;
        names.reserve(m_Cache.size());
        for (const auto& pair : m_Cache)
        {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    std::unordered_map<std::string, ResourcePtr> m_Cache;
    mutable std::shared_mutex m_Mutex;
};
