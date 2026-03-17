#pragma once

#include <core/type/app_config.h>
#include <core/interface/i_configurable.h>
#include <vector>
#include <mutex>
#include <memory>

/**
 * @brief Singleton manager that holds the global AppConfig and notifies observers of changes.
 */
class ConfigManager {
public:
    static ConfigManager& Instance();

    /**
     * @brief Overwrites the entire configuration and notifies all observers.
     */
    void SetConfig(const AppConfig& config);

    /**
     * @brief Returns a reference to the current global configuration.
     */
    const AppConfig& GetConfig() const { return m_Config; }

    /**
     * @brief Updates a specific configuration value by key and notifies observers.
     */
    void UpdateValue(const std::string& key, const ConfigValue& value);

    /**
     * @brief Registers an observer to receive configuration updates.
     */
    void Register(IConfigurable* observer);

    /**
     * @brief Unregisters an observer.
     */
    void Unregister(IConfigurable* observer);

private:
    ConfigManager() = default;

    AppConfig m_Config;
    std::vector<IConfigurable*> m_Observers;
    mutable std::mutex m_Mutex;
};
