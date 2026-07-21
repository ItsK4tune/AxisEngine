#include <resource/logic/resource_watcher.h>
#include <core/logic/event_manager.h>
#include <core/logic/logger.h>
#include <resource/type/resource_events.h>
#include <chrono>
#include <iostream>

ResourceWatcher::ResourceWatcher()
{
}

ResourceWatcher::~ResourceWatcher()
{
    SetEnabled(false);
}

void ResourceWatcher::SetEnabled(bool enabled)
{
    if (enabled)
    {
        bool expected = false;
        if (!m_Running.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
            return;
        m_WatcherThread = std::thread(&ResourceWatcher::WatcherLoop, this);
        return;
    }

    bool expected = true;
    if (!m_Running.compare_exchange_strong(expected, false, std::memory_order_acq_rel))
        return;
    m_StopCV.notify_all();
    if (m_WatcherThread.joinable())
        m_WatcherThread.join();
}

void ResourceWatcher::Watch(const std::string& name, const std::string& path, const std::string& type)
{
    WatchEntry entry;
    entry.name = name;
    entry.filePath = path;
    entry.type = type;

    try
    {
        entry.lastWriteTime = std::filesystem::last_write_time(path);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        LOGGER_WARN("HotReload") << "Cannot watch '" << name << "' at " << path << ": " << e.what();
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& w : m_Watchers)
    {
        if (w.name == name && w.type == type)
        {
            w = entry;
            return;
        }
    }
    m_Watchers.push_back(entry);
}

void ResourceWatcher::Watch(const std::string& name, const std::string& path, const std::string& type,
                            const std::string& vsPath, const std::string& fsPath, const std::string& gsPath)
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
        if (!vsPath.empty())
            entry.vsLastWriteTime = std::filesystem::last_write_time(vsPath);
        if (!fsPath.empty())
            entry.fsLastWriteTime = std::filesystem::last_write_time(fsPath);
        if (!gsPath.empty())
            entry.gsLastWriteTime = std::filesystem::last_write_time(gsPath);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        LOGGER_WARN("HotReload") << "Cannot watch shader '" << name << "': " << e.what();
    }

    std::lock_guard<std::mutex> lock(m_Mutex);
    for (auto& w : m_Watchers)
    {
        if (w.name == name && w.type == type)
        {
            w = entry;
            return;
        }
    }
    m_Watchers.push_back(entry);
}

void ResourceWatcher::WatcherLoop()
{
    while (m_Running)
    {
        {
            std::unique_lock<std::mutex> lock(m_StopMutex);
            m_StopCV.wait_for(lock, std::chrono::milliseconds(500), [this]() { return !m_Running.load(); });
        }

        if (!m_Running)
            break;

        std::vector<WatchEntry> watchersCopy;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            watchersCopy = m_Watchers;
        }

        for (auto& watcher : watchersCopy)
        {
            try
            {
                auto checkFile = [&](const std::string& path, std::filesystem::file_time_type& previous) -> bool {
                    if (path.empty() || !std::filesystem::exists(path))
                        return false;

                    auto currentWriteTime = std::filesystem::last_write_time(path);
                    if (currentWriteTime <= previous)
                        return false;

                    previous = currentWriteTime;
                    return true;
                };

                std::string changedPath;
                if (checkFile(watcher.filePath, watcher.lastWriteTime))
                    changedPath = watcher.filePath;
                if (checkFile(watcher.vsPath, watcher.vsLastWriteTime))
                    changedPath = watcher.vsPath;
                if (checkFile(watcher.fsPath, watcher.fsLastWriteTime))
                    changedPath = watcher.fsPath;
                if (checkFile(watcher.gsPath, watcher.gsLastWriteTime))
                    changedPath = watcher.gsPath;

                if (!changedPath.empty())
                {
                    {
                        std::lock_guard<std::mutex> lock(m_Mutex);
                        for (auto& w : m_Watchers)
                        {
                            if (w.name == watcher.name && w.type == watcher.type)
                            {
                                w.lastWriteTime = watcher.lastWriteTime;
                                w.vsLastWriteTime = watcher.vsLastWriteTime;
                                w.fsLastWriteTime = watcher.fsLastWriteTime;
                                w.gsLastWriteTime = watcher.gsLastWriteTime;
                                break;
                            }
                        }
                    }

                    LOGGER_INFO("HotReload") << "Detected change in: " << changedPath;

                    ResourceReloadEvent e;
                    e.name = watcher.name;
                    e.type = watcher.type;
                    e.filePath = changedPath;

                    std::lock_guard<std::mutex> lock(m_Mutex);
                    m_PendingReloads.push_back(e);
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {
                LOGGER_WARN("HotReload") << "Watcher check failed for '" << watcher.name << "' (" << watcher.type
                                         << "): " << e.what();
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

    for (const auto& event : eventsToProcess)
    {
        EventManager::Instance().Publish(event);
    }
}
