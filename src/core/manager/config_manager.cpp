#include <core/manager/config_manager.h>
#include <algorithm>

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::SetConfig(const AppConfig& config) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Config = config;
    
    // For a full set, we might need a specific notification or just loop through common keys.
    // Simplifying for now: Notify observers that "global" config changed.
    for (auto* observer : m_Observers) {
        observer->OnConfigChanged("all", "");
    }
}

void ConfigManager::UpdateValue(const std::string& key, const ConfigValue& value) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    
    // Example: Reflection-like mapping (manual for now)
    if (key == "graphics.msaa") {
        if (auto* v = std::get_if<int>(&value)) m_Config.msaaSamples = *v;
    } else if (key == "audio.masterVolume") {
        if (auto* v = std::get_if<float>(&value)) m_Config.masterVolume = *v;
    }
    // ... more mappings as needed ...

    for (auto* observer : m_Observers) {
        observer->OnConfigChanged(key, value);
    }
}

void ConfigManager::Register(IConfigurable* observer) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (std::find(m_Observers.begin(), m_Observers.end(), observer) == m_Observers.end()) {
        m_Observers.push_back(observer);
    }
}

void ConfigManager::Unregister(IConfigurable* observer) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Observers.erase(std::remove(m_Observers.begin(), m_Observers.end(), observer), m_Observers.end());
}
