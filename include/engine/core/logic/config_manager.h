#pragma once

#include <core/logic/event_manager.h>
#include <core/logic/config_validation.h>
#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <mutex>
#include <atomic>
#include <memory>
#include <shared_mutex>

class ConfigManager
{
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    void Initialize(const AppConfig& config, const ConfigValidationPolicy& policy = {})
    {
        std::unique_lock<std::shared_mutex> lock(m_Mutex);
        m_ValidationPolicy = policy;
        m_Config = ValidateAndSanitizeConfig(config, m_ValidationPolicy).config;
        m_IsHeadless = m_Config.headlessMode;
        m_ConfigSnapshot.store(std::make_shared<const AppConfig>(m_Config), std::memory_order_release);
    }

    void UpdateConfig(const AppConfig& config, uint32_t type = ConfigChangedEvent::All)
    {
        AppConfig snapshot;
        bool isHeadless = false;
        {
            std::unique_lock<std::shared_mutex> lock(m_Mutex);
            m_Config = ValidateAndSanitizeConfig(config, m_ValidationPolicy).config;
            m_Config.headlessMode = m_IsHeadless;  // preserve headless flag
            snapshot = m_Config;
            isHeadless = m_IsHeadless;
            m_ConfigSnapshot.store(std::make_shared<const AppConfig>(m_Config), std::memory_order_release);
        }
        // In headless mode, strip non-critical change categories
        if (isHeadless)
        {
            type &= ~(ConfigChangedEvent::Graphics | ConfigChangedEvent::Window | ConfigChangedEvent::Audio |
                      ConfigChangedEvent::Input);
            if (type == ConfigChangedEvent::None)
                return;
        }
        EventManager::Instance().Publish(ConfigChangedEvent{snapshot, type});
    }

    void SetResolution(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;
        AppConfig snapshot;
        {
            std::unique_lock<std::shared_mutex> lock(m_Mutex);
            if (m_Config.window.width == width && m_Config.window.height == height)
                return;
            m_Config.window.width = width;
            m_Config.window.height = height;
            snapshot = m_Config;
            m_ConfigSnapshot.store(std::make_shared<const AppConfig>(m_Config), std::memory_order_release);
        }
        EventManager::Instance().Publish(ConfigChangedEvent{snapshot, ConfigChangedEvent::Window});
    }

    AppConfig GetConfig() const
    {
        return *m_ConfigSnapshot.load(std::memory_order_acquire);
    }

    std::shared_ptr<const AppConfig> GetConfigSnapshot() const
    {
        return m_ConfigSnapshot.load(std::memory_order_acquire);
    }

private:
    AppConfig m_Config;
    std::atomic<std::shared_ptr<const AppConfig>> m_ConfigSnapshot{std::make_shared<const AppConfig>()};
    mutable std::shared_mutex m_Mutex;
    bool m_IsHeadless = false;
    ConfigValidationPolicy m_ValidationPolicy;

public:
    bool IsHeadless() const
    {
        return GetConfigSnapshot()->headlessMode;
    }
};
