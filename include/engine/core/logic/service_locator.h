#pragma once

#include <atomic>
#include <mutex>
#include <memory>
#include <typeindex>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>

class EngineAccessor;

class ServiceLocator
{
public:
    class Activation
    {
    public:
        explicit Activation(ServiceLocator& locator) : m_Previous(s_Active)
        {
            s_Active = &locator;
        }
        ~Activation()
        {
            s_Active = m_Previous;
        }
        Activation(const Activation&) = delete;
        Activation& operator=(const Activation&) = delete;

    private:
        ServiceLocator* m_Previous = nullptr;
    };

    static ServiceLocator& Instance()
    {
        if (s_Active)
            return *s_Active;
        if (auto* processDefault = s_ProcessDefault.load(std::memory_order_acquire))
            return *processDefault;
        static ServiceLocator fallback;
        return fallback;
    }

    // Application installs its scoped registry as the process default so jobs
    // spawned on worker threads resolve the same services. Activation remains
    // a thread-local override for nested tests and tools.
    static bool SetProcessDefault(ServiceLocator* locator)
    {
        if (!locator)
            return false;
        ServiceLocator* expected = nullptr;
        return s_ProcessDefault.compare_exchange_strong(expected, locator, std::memory_order_acq_rel) ||
               expected == locator;
    }

    static void ClearProcessDefault(ServiceLocator* locator)
    {
        ServiceLocator* expected = locator;
        s_ProcessDefault.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
    }

    ServiceLocator() = default;
    ~ServiceLocator() = default;
    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;

    Activation Activate()
    {
        return Activation(*this);
    }

    template <typename T>
    void Register(T* service)
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Services[std::type_index(typeid(T))] = static_cast<void*>(service);
        m_ServiceSnapshot.store(std::make_shared<const ServiceMap>(m_Services), std::memory_order_release);
    }

    template <typename T>
    void Unregister()
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Services.erase(std::type_index(typeid(T)));
        m_ServiceSnapshot.store(std::make_shared<const ServiceMap>(m_Services), std::memory_order_release);
    }

    template <typename T>
    T* Resolve() const
    {
        const auto services = m_ServiceSnapshot.load(std::memory_order_acquire);
        auto it = services->find(std::type_index(typeid(T)));
        if (it != services->end())
            return static_cast<T*>(it->second);
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
        const auto services = m_ServiceSnapshot.load(std::memory_order_acquire);
        return services->find(std::type_index(typeid(T))) != services->end();
    }

    template <typename T>
    void Register(const std::string& name, T* service)
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_NamedServices[name] = static_cast<void*>(service);
        m_NamedServiceSnapshot.store(std::make_shared<const NamedServiceMap>(m_NamedServices),
                                     std::memory_order_release);
    }

    template <typename T>
    T* Resolve(const std::string& name) const
    {
        const auto services = m_NamedServiceSnapshot.load(std::memory_order_acquire);
        auto it = services->find(name);
        if (it != services->end())
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
        const auto services = m_NamedServiceSnapshot.load(std::memory_order_acquire);
        return services->find(name) != services->end();
    }

    void ClearAll()
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Services.clear();
        m_NamedServices.clear();
        m_ServiceSnapshot.store(std::make_shared<const ServiceMap>(), std::memory_order_release);
        m_NamedServiceSnapshot.store(std::make_shared<const NamedServiceMap>(), std::memory_order_release);
    }

private:
    friend class EngineAccessor;

    void* ResolveByType(std::type_index type) const
    {
        const auto services = m_ServiceSnapshot.load(std::memory_order_acquire);
        const auto it = services->find(type);
        return it != services->end() ? it->second : nullptr;
    }

    using ServiceMap = std::unordered_map<std::type_index, void*>;
    using NamedServiceMap = std::unordered_map<std::string, void*>;

    ServiceMap m_Services;
    NamedServiceMap m_NamedServices;
    std::atomic<std::shared_ptr<const ServiceMap>> m_ServiceSnapshot{std::make_shared<const ServiceMap>()};
    std::atomic<std::shared_ptr<const NamedServiceMap>> m_NamedServiceSnapshot{
        std::make_shared<const NamedServiceMap>()};
    mutable std::shared_mutex m_Mutex;
    static inline thread_local ServiceLocator* s_Active = nullptr;
    static inline std::atomic<ServiceLocator*> s_ProcessDefault = nullptr;
};
