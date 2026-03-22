#pragma once

#include <core/type/app_config.h>
#include <core/type/event_types.h>
#include <core/logic/event_system.h>
#include <mutex>

/**
 * @brief Layer 3 Domain Service that owns the runtime configuration state.
 * 
 * Replaces direct storage of AppConfig in Application and other systems.
 */
class ConfigManager
{
public:
    ConfigManager() = default;
    ~ConfigManager() = default;

    /**
     * @brief Initialize with a starting config.
     */
    void Initialize(const AppConfig& config) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Config = config;
    }

    /**
     * @brief Update the internal config and publish events.
     * @param config The new config to apply.
     * @param type The type of changes being applied (default: All).
     */
    void UpdateConfig(const AppConfig& config, uint32_t type = ConfigChangedEvent::All) {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Config = config;
        }
        EventSystem::Instance().Publish(ConfigChangedEvent{ m_Config, type });
    }

    /**
     * @brief Silent update for resolution (usually called from Application::OnResize).
     */
    void SetResolution(int width, int height) {
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_Config.width == width && m_Config.height == height) return;
            m_Config.width = width;
            m_Config.height = height;
        }
        EventSystem::Instance().Publish(ConfigChangedEvent{ m_Config, ConfigChangedEvent::Window });
    }

    /**
     * @brief Get a read-only copy of the current config.
     */
    const AppConfig& GetConfig() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_Config;
    }

private:
    AppConfig m_Config;
    mutable std::mutex m_Mutex;
};
