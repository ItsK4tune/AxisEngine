#pragma once

#include <core/logic/event_manager.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <mutex>
#include <shared_mutex>

class ConfigManager
{
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    void Initialize(const AppConfig& config)
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_Config = config;
        m_IsHeadless = config.headlessMode;
    }

    void UpdateConfig(const AppConfig& config, uint32_t type = ConfigChangedEvent::All)
    {
        {
            std::unique_lock<std::shared_mutex> lock(m_Mutex);
            m_Config = config;
            m_Config.headlessMode = m_IsHeadless;  // preserve headless flag
        }
        // In headless mode, strip non-critical change categories
        if (m_IsHeadless)
        {
            type &= ~(ConfigChangedEvent::Graphics | ConfigChangedEvent::Window | ConfigChangedEvent::Audio |
                      ConfigChangedEvent::Input);
            if (type == ConfigChangedEvent::None)
                return;
        }
        EventManager::Instance().Publish(ConfigChangedEvent{m_Config, type});
    }

    void SetResolution(int width, int height)
    {
        {
            std::unique_lock<std::shared_mutex> lock(m_Mutex);
            if (m_Config.width == width && m_Config.height == height)
                return;
            m_Config.width = width;
            m_Config.height = height;
        }
        EventManager::Instance().Publish(ConfigChangedEvent{m_Config, ConfigChangedEvent::Window});
    }

    const AppConfig& GetConfig() const
    {
        std::shared_lock<std::shared_mutex> lock(m_Mutex);
        return m_Config;
    }

private:
    AppConfig m_Config;
    mutable std::shared_mutex m_Mutex;
    bool m_IsHeadless = false;

public:
    bool IsHeadless() const
    {
        return m_IsHeadless;
    }
};
