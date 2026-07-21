#pragma once

#include <resource/type/resource_events.h>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

class ResourceWatcher
{
public:
    ResourceWatcher();
    ~ResourceWatcher();

    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_Running.load(std::memory_order_acquire); }

    void Watch(const std::string& name, const std::string& path, const std::string& type);
    void Watch(const std::string& name, const std::string& path, const std::string& type, const std::string& vsPath,
               const std::string& fsPath, const std::string& gsPath);

    void Update(float dt);

private:
    struct WatchEntry
    {
        std::string name;
        std::string filePath;
        std::filesystem::file_time_type lastWriteTime;
        std::filesystem::file_time_type vsLastWriteTime;
        std::filesystem::file_time_type fsLastWriteTime;
        std::filesystem::file_time_type gsLastWriteTime;
        std::string type;
        std::string vsPath;
        std::string fsPath;
        std::string gsPath;
    };

    std::vector<WatchEntry> m_Watchers;
    std::vector<ResourceReloadEvent> m_PendingReloads;
    std::mutex m_Mutex;
    std::atomic<bool> m_Running{false};
    std::thread m_WatcherThread;
    std::condition_variable m_StopCV;
    std::mutex m_StopMutex;

    void WatcherLoop();
};
