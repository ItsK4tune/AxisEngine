#pragma once

#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>
#include <string>

/**
 * @brief Centralized service registry — replaces EngineContext raw pointer passing.
 * 
 * Services register themselves on creation and unregister on destruction.
 * Consumers call Resolve<T>() or Require<T>() instead of storing raw pointers.
 * 
 * Only services registered via Register<T>() are available. This class is a
 * singleton at Layer 2 (Core Infrastructure).
 */
class ServiceLocator
{
public:
    static ServiceLocator& Instance()
    {
        static ServiceLocator instance;
        return instance;
    }

    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;

    /**
     * @brief Register a service instance. The caller retains ownership.
     */
    template <typename T>
    void Register(T* service)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Services[std::type_index(typeid(T))] = static_cast<void*>(service);
    }

    /**
     * @brief Unregister a service (typically called in destructor).
     */
    template <typename T>
    void Unregister()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Services.erase(std::type_index(typeid(T)));
    }

    /**
     * @brief Resolve a service by type. Returns nullptr if not registered.
     */
    template <typename T>
    T* Resolve() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_Services.find(std::type_index(typeid(T)));
        if (it != m_Services.end())
            return static_cast<T*>(it->second);
        return nullptr;
    }

    /**
     * @brief Resolve a service by type. Throws if not registered.
     */
    template <typename T>
    T& Require() const
    {
        T* service = Resolve<T>();
        if (!service)
            throw std::runtime_error(std::string("ServiceLocator: service not registered: ") + typeid(T).name());
        return *service;
    }

    /**
     * @brief Check if a service is registered.
     */
    template <typename T>
    bool Has() const
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Services.find(std::type_index(typeid(T))) != m_Services.end();
    }

    /**
     * @brief Clear all registered services (used during shutdown).
     */
    void ClearAll()
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Services.clear();
    }

private:
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    std::unordered_map<std::type_index, void*> m_Services;
    mutable std::mutex m_Mutex;
};
