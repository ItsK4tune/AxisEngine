#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <functional>
#include <memory>
#include <algorithm>

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

    void Register(int id, Callback cb)
    {
        listeners.push_back({id, cb});
    }

    void Unregister(int id)
    {
        listeners.erase(
            std::remove_if(listeners.begin(), listeners.end(),
                           [id](const Listener &l)
                           { return l.id == id; }),
            listeners.end());
    }

    void Dispatch(const T &event)
    {
        for (const auto &listener : listeners)
        {
            listener.callback(event);
        }
    }

private:
    std::vector<Listener> listeners;
};

class EventSystem
{
public:
    static EventSystem &Instance();

    EventSystem(const EventSystem &) = delete;
    EventSystem &operator=(const EventSystem &) = delete;

    template <typename T>
    int Subscribe(std::function<void(const T &)> callback)
    {
        int id = nextListenerId++;
        std::type_index typeIndex = std::type_index(typeid(T));

        if (dispatchers.find(typeIndex) == dispatchers.end())
        {
            dispatchers[typeIndex] = std::make_unique<EventDispatcher<T>>();
        }

        auto *dispatcher = static_cast<EventDispatcher<T> *>(dispatchers[typeIndex].get());
        dispatcher->Register(id, callback);

        return id;
    }

    template <typename T>
    void Unsubscribe(int listenerId)
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        if (dispatchers.find(typeIndex) != dispatchers.end())
        {
            auto *dispatcher = static_cast<EventDispatcher<T> *>(dispatchers[typeIndex].get());
            dispatcher->Unregister(listenerId);
        }
    }

    template <typename T>
    void Publish(const T &event)
    {
        std::type_index typeIndex = std::type_index(typeid(T));

        if (dispatchers.find(typeIndex) != dispatchers.end())
        {
            auto *dispatcher = static_cast<EventDispatcher<T> *>(dispatchers[typeIndex].get());
            dispatcher->Dispatch(event);
        }
    }

private:
    EventSystem() = default;
    ~EventSystem() = default;

    int nextListenerId = 0;
    std::unordered_map<std::type_index, std::unique_ptr<IEventDispatcher>> dispatchers;
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
            EventSystem::Instance().Unsubscribe<T>(listenerId);
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
