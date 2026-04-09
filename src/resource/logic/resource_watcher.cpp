#include <chrono>
#include <core/logic/event_manager.h>
#include <iostream>
#include <resource/logic/resource_watcher.h>
#include <resource/type/resource_events.h>
#include <core/logic/logger.h>

ResourceWatcher::ResourceWatcher()
    : m_Running(true)
{
    m_WatcherThread = std::thread(&ResourceWatcher::WatcherLoop, this);
}

ResourceWatcher::~ResourceWatcher()
{
    {
        std::lock_guard<std::mutex> lock(m_StopMutex);
        m_Running = false;
    }
    m_StopCV.notify_all();

    if (m_WatcherThread.joinable())
        m_WatcherThread.join();
}

void ResourceWatcher::Watch(const std::string &name, const std::string &path, const std::string &type)
{
    bool alreadyWatched = false;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        for (auto &w : m_Watchers)
        {
            if (w.name == name && w.type == type)
            {
                alreadyWatched = true;
                break;
            }
        }
    }

    if (!alreadyWatched)
    {
        WatchEntry entry;
        entry.name = name;
        entry.filePath = path;
        entry.type = type;

        try
        {
            entry.lastWriteTime = std::filesystem::last_write_time(path);
        }
        catch (...)
        {
        }

        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Watchers.push_back(entry);
    }
}

void ResourceWatcher::Watch(const std::string &name, const std::string &path, const std::string &type,
                            const std::string &vsPath, const std::string &fsPath, const std::string &gsPath)
{
    WatchEntry entry;
    entry.name = name;
    entry.filePath = path;
    entry.type = type;
    entry.vsPath = vsPath;
    entry.fsPath = fsPath;
    entry.gsPath = gsPath;

    try
    {
        entry.lastWriteTime = std::filesystem::last_write_time(path);
    }
    catch (...)
    {
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Watchers.push_back(entry);
}

void ResourceWatcher::WatcherLoop()
{
    while (m_Running)
    {
        {
            std::unique_lock<std::mutex> lock(m_StopMutex);
            m_StopCV.wait_for(lock, std::chrono::milliseconds(500),
                              [this]()
                              { return !m_Running.load(); });
        }

        if (!m_Running)
            break;

        std::vector<WatchEntry> watchersCopy;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            watchersCopy = m_Watchers;
        }

        for (auto &watcher : watchersCopy)
        {
            try
            {
                if (!std::filesystem::exists(watcher.filePath))
                    continue;

                auto currentWriteTime = std::filesystem::last_write_time(watcher.filePath);
                if (currentWriteTime > watcher.lastWriteTime)
                {

                    {
                        std::lock_guard<std::mutex> lock(m_Mutex);
                        for (auto& w : m_Watchers) {
                            if (w.name == watcher.name && w.type == watcher.type) {
                                w.lastWriteTime = currentWriteTime;
                                break;
                            }
                        }
                    }

                    LOGGER_INFO("HotReload") << "Detected change in: " << watcher.filePath;

                    ResourceReloadEvent e;
                    e.name = watcher.name;
                    e.type = watcher.type;
                    e.filePath = watcher.filePath;

                    std::lock_guard<std::mutex> lock(m_Mutex);
                    m_PendingReloads.push_back(e);
                }
            }
            catch (const std::filesystem::filesystem_error &)
            {
            }
        }
    }
}

void ResourceWatcher::Update(float dt)
{
    std::vector<ResourceReloadEvent> eventsToProcess;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        if (!m_PendingReloads.empty())
        {
            eventsToProcess = std::move(m_PendingReloads);
            m_PendingReloads.clear();
        }
    }

    for (const auto &event : eventsToProcess)
    {
        EventManager::Instance().Publish(event);
    }
}
