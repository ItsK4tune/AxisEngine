#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace internal {
    inline int GetNextEventId() {
        static int id = 0;
        return id++;
    }
}

template<typename T>
inline int GetEventId() {
    static int id = internal::GetNextEventId();
    return id;
}

class IEventDispatcher
{
public:
    virtual ~IEventDispatcher() = default;
};

template <typename T>
class EventDispatcher : public IEventDispatcher
{
public:
    using Callback = std::function<void(const T &)>;

    struct Listener
    {
        int id;
        Callback callback;
    };

    EventDispatcher() {
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
            std::remove_if(newListeners->begin(), newListeners->end(),
                           [id](const Listener &l)
                           { return l.id == id; }),
            newListeners->end());
        m_Listeners = std::move(newListeners);
    }

    void Dispatch(const T &event)
    {
        std::shared_ptr<const std::vector<Listener>> localListeners;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            localListeners = m_Listeners;
        }

        for (const auto &listener : *localListeners)
        {
            listener.callback(event);
        }
    }

private:
    std::shared_ptr<const std::vector<Listener>> m_Listeners;
    std::mutex m_Mutex;
};

class EventManager
{
public:
    static EventManager &Instance();

    EventManager(const EventManager &) = delete;
    EventManager &operator=(const EventManager &) = delete;

    template <typename T>
    int Subscribe(std::function<void(const T &)> callback)
    {
        int eventId = GetEventId<T>();
        
        std::lock_guard<std::mutex> lock(m_DispatchersMutex);
        int listenerId = nextListenerId++;

        if (eventId >= (int)m_Dispatchers.size()) {
            m_Dispatchers.resize(eventId + 1);
        }

        if (!m_Dispatchers[eventId])
        {
            m_Dispatchers[eventId] = std::make_shared<EventDispatcher<T>>();
        }

        auto *dispatcher = static_cast<EventDispatcher<T> *>(m_Dispatchers[eventId].get());
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
            auto *dispatcher = static_cast<EventDispatcher<T> *>(m_Dispatchers[eventId].get());
            dispatcher->Unregister(listenerId);
        }
    }

    template <typename T>
    void Publish(const T &event)
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
            auto *dispatcher = static_cast<EventDispatcher<T> *>(dispatcherPtr.get());
            dispatcher->Dispatch(event);
        }
    }

private:
    EventManager() = default;
    ~EventManager() = default;

    int nextListenerId = 0;
    std::vector<std::shared_ptr<IEventDispatcher>> m_Dispatchers;
    std::mutex m_DispatchersMutex;
};

template <typename T>
class ScopedSubscriber
{
public:
    ScopedSubscriber() = default;
    ScopedSubscriber(int id) : listenerId(id) {}

    ~ScopedSubscriber()
    {
        Unsubscribe();
    }

    ScopedSubscriber(const ScopedSubscriber &) = delete;
    ScopedSubscriber &operator=(const ScopedSubscriber &) = delete;

    ScopedSubscriber(ScopedSubscriber &&other) noexcept : listenerId(other.listenerId)
    {
        other.listenerId = -1;
    }

    ScopedSubscriber &operator=(ScopedSubscriber &&other) noexcept
    {
        if (this != &other)
        {
            Unsubscribe();
            listenerId = other.listenerId;
            other.listenerId = -1;
        }
        return *this;
    }

    void Unsubscribe()
    {
        if (listenerId != -1)
        {
            EventManager::Instance().Unsubscribe<T>(listenerId);
            listenerId = -1;
        }
    }

    void Reset(int id)
    {
        Unsubscribe();
        listenerId = id;
    }

private:
    int listenerId = -1;
};