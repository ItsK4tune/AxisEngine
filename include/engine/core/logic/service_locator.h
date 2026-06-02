#pragma once

#include <atomic>
#include <typeindex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

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

    template <typename T>
    void Register(T* service)
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Services[std::type_index(typeid(T))] = static_cast<void*>(service);
        uint64_t globalVer = m_Version.load(std::memory_order_acquire);
        TypeCache<T>::ptr.store(service, std::memory_order_release);
        TypeCache<T>::valid.store(true, std::memory_order_release);
        TypeCache<T>::version.store(globalVer, std::memory_order_release);
    }

    template <typename T>
    void Unregister()
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Services.erase(std::type_index(typeid(T)));
        TypeCache<T>::ptr.store(nullptr, std::memory_order_release);
        TypeCache<T>::valid.store(false, std::memory_order_release);
    }

    template <typename T>
    T* Resolve() const
    {
        uint64_t globalVer = m_Version.load(std::memory_order_acquire);
        if (TypeCache<T>::version.load(std::memory_order_acquire) == globalVer &&
            TypeCache<T>::valid.load(std::memory_order_acquire))
            return TypeCache<T>::ptr.load(std::memory_order_acquire);

        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        auto it = m_Services.find(std::type_index(typeid(T)));
        if (it != m_Services.end())
        {
            T* result = static_cast<T*>(it->second);
            TypeCache<T>::ptr.store(result, std::memory_order_release);
            TypeCache<T>::valid.store(true, std::memory_order_release);
            TypeCache<T>::version.store(globalVer, std::memory_order_release);
            return result;
        }
        TypeCache<T>::ptr.store(nullptr, std::memory_order_release);
        TypeCache<T>::valid.store(false, std::memory_order_release);
        TypeCache<T>::version.store(globalVer, std::memory_order_release);
        return nullptr;
    }

    template <typename T>
    T& Require() const
    {
        T* service = Resolve<T>();
        if (!service)
            throw std::runtime_error(std::string("ServiceLocator: service not registered: ") + typeid(T).name());
        return *service;
    }

    template <typename T>
    bool Has() const
    {
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        return m_Services.find(std::type_index(typeid(T))) != m_Services.end();
    }

    template <typename T>
    void Register(const std::string& name, T* service)
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_NamedServices[name] = static_cast<void*>(service);
    }

    template <typename T>
    T* Resolve(const std::string& name) const
    {
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        auto it = m_NamedServices.find(name);
        if (it != m_NamedServices.end())
            return static_cast<T*>(it->second);
        return nullptr;
    }

    template <typename T>
    T& Require(const std::string& name) const
    {
        T* service = Resolve<T>(name);
        if (!service)
            throw std::runtime_error("ServiceLocator: service not registered by name: " + name);
        return *service;
    }

    bool Has(const std::string& name) const
    {
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        return m_NamedServices.find(name) != m_NamedServices.end();
    }

    void ClearAll()
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Services.clear();
        m_NamedServices.clear();
        m_Version.fetch_add(1, std::memory_order_release);
    }

private:
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    std::unordered_map<std::type_index, void*> m_Services;
    std::unordered_map<std::string, void*> m_NamedServices;
    mutable std::shared_mutex m_Mutex;
    std::atomic<uint64_t> m_Version{0};

    template <typename T>
    struct TypeCache
    {
        static inline std::atomic<T*> ptr{nullptr};
        static inline std::atomic<bool> valid{false};
        static inline std::atomic<uint64_t> version{0};
    };
};
