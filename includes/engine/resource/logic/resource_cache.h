#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>

/**
 * @brief Thread-safe generic cache for engine resources.
 */
template <typename T>
class ResourceCache {
public:
    using ResourcePtr = std::shared_ptr<T>;

    ResourceCache() = default;
    ~ResourceCache() = default;

    /**
     * @brief Retrieves a resource from the cache by name.
     * @return shared_ptr to the resource or nullptr if not found.
     */
    ResourcePtr Get(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Cache.find(name);
        if (it != m_Cache.end()) {
            return it->second;
        }
        return nullptr;
    }

    /**
     * @brief Adds a resource to the cache. Overwrites existing entry if name matches.
     */
    void Add(const std::string& name, ResourcePtr resource) {
        if (!resource) return;
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Cache[name] = resource;
    }

    /**
     * @brief Removes a resource from the cache.
     */
    void Remove(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Cache.erase(name);
    }

    /**
     * @brief Checks if a resource exists in the cache.
     */
    bool Has(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Cache.find(name) != m_Cache.end();
    }

    /**
     * @brief Clears all resources from the cache.
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Cache.clear();
    }

    /**
     * @brief Returns a list of all resource names currently in cache.
     */
    std::vector<std::string> GetAllNames() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        std::vector<std::string> names;
        names.reserve(m_Cache.size());
        for (const auto& pair : m_Cache) {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    std::unordered_map<std::string, ResourcePtr> m_Cache;
    mutable std::mutex m_Mutex;
};
