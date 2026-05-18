#pragma once

#include <typeindex>
#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace internal
{
inline int GetNextEventId()
{
    static int id = 0;
    return id++;
}
}  // namespace internal

template <typename T>
inline int GetEventId()
{
    static int id = internal::GetNextEventId();
    return id;
}

class IEventDispatcher
{
public:
    virtual ~IEventDispatcher() = default;
    virtual bool HasListeners() const = 0;
};

template <typename T>
class EventDispatcher : public IEventDispatcher
{
public:
    using Callback = std::function<void(const T&)>;

    struct Listener
    {
        int id;
        Callback callback;
    };

    EventDispatcher()
    {
        m_Listeners = std::make_shared<std::vector<Listener>>();
    }

    void Register(int id, Callback cb)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto newListeners = std::make_shared<std::vector<Listener>>(*m_Listeners);
        newListeners->push_back({id, cb});
        m_Listeners = std::move(newListeners);
    }

    void Unregister(int id)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto newListeners = std::make_shared<std::vector<Listener>>(*m_Listeners);
        newListeners->erase(
            std::remove_if(newListeners->begin(), newListeners->end(), [id](const Listener& l) { return l.id == id; }),
            newListeners->end());
        m_Listeners = std::move(newListeners);
    }

    void Dispatch(const T& event)
    {
        std::shared_ptr<const std::vector<Listener>> localListeners;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            localListeners = m_Listeners;
        }

        for (const auto& listener : *localListeners)
        {
            listener.callback(event);
        }
    }

    bool HasListeners() const override
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return !m_Listeners->empty();
    }

private:
    std::shared_ptr<const std::vector<Listener>> m_Listeners;
    mutable std::mutex m_Mutex;
};

class EventManager
{
public:
    static EventManager& Instance();

    EventManager(const EventManager&) = delete;
    EventManager& operator=(const EventManager&) = delete;

    template <typename T>
    int Subscribe(std::function<void(const T&)> callback)
    {
        int eventId = GetEventId<T>();

        std::lock_guard<std::mutex> lock(m_DispatchersMutex);
        int listenerId = m_NextListenerId++;

        if (eventId >= (int)m_Dispatchers.size())
        {
            m_Dispatchers.resize(eventId + 1);
        }

        if (!m_Dispatchers[eventId])
        {
            m_Dispatchers[eventId] = std::make_shared<EventDispatcher<T>>();
        }

        auto* dispatcher = static_cast<EventDispatcher<T>*>(m_Dispatchers[eventId].get());
        dispatcher->Register(listenerId, callback);

        return listenerId;
    }

    template <typename T>
    void Unsubscribe(int listenerId)
    {
        int eventId = GetEventId<T>();

        std::lock_guard<std::mutex> lock(m_DispatchersMutex);
        if (eventId < (int)m_Dispatchers.size() && m_Dispatchers[eventId])
        {
            auto* dispatcher = static_cast<EventDispatcher<T>*>(m_Dispatchers[eventId].get());
            dispatcher->Unregister(listenerId);
        }
    }

    template <typename T>
    void Publish(const T& event)
    {
        int eventId = GetEventId<T>();

        std::shared_ptr<IEventDispatcher> dispatcherPtr;
        {
            std::lock_guard<std::mutex> lock(m_DispatchersMutex);
            if (eventId < (int)m_Dispatchers.size())
            {
                dispatcherPtr = m_Dispatchers[eventId];
            }
        }

        if (dispatcherPtr)
        {
            auto* dispatcher = static_cast<EventDispatcher<T>*>(dispatcherPtr.get());
            dispatcher->Dispatch(event);
        }
    }

    template <typename T>
    bool HasListeners()
    {
        int eventId = GetEventId<T>();

        std::shared_ptr<IEventDispatcher> dispatcherPtr;
        {
            std::lock_guard<std::mutex> lock(m_DispatchersMutex);
            if (eventId < (int)m_Dispatchers.size())
            {
                dispatcherPtr = m_Dispatchers[eventId];
            }
        }

        if (dispatcherPtr)
        {
            return dispatcherPtr->HasListeners();
        }
        return false;
    }

private:
    EventManager() = default;
    ~EventManager() = default;

    int m_NextListenerId = 0;
    std::vector<std::shared_ptr<IEventDispatcher>> m_Dispatchers;
    std::mutex m_DispatchersMutex;
};

template <typename T>
class ScopedSubscriber
{
public:
    ScopedSubscriber() = default;
    ScopedSubscriber(int id) : m_ListenerId(id)
    {
    }

    ~ScopedSubscriber()
    {
        Unsubscribe();
    }

    ScopedSubscriber(const ScopedSubscriber&) = delete;
    ScopedSubscriber& operator=(const ScopedSubscriber&) = delete;

    ScopedSubscriber(ScopedSubscriber&& other) noexcept : m_ListenerId(other.m_ListenerId)
    {
        other.m_ListenerId = -1;
    }

    ScopedSubscriber& operator=(ScopedSubscriber&& other) noexcept
    {
        if (this != &other)
        {
            Unsubscribe();
            m_ListenerId = other.m_ListenerId;
            other.m_ListenerId = -1;
        }
        return *this;
    }

    void Unsubscribe()
    {
        if (m_ListenerId != -1)
        {
            EventManager::Instance().Unsubscribe<T>(m_ListenerId);
            m_ListenerId = -1;
        }
    }

    void Reset(int id)
    {
        Unsubscribe();
        m_ListenerId = id;
    }

private:
    int m_ListenerId = -1;
};
