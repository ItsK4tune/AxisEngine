#pragma once
#include <string>
#include <variant>

/**
 * @brief Variant type for configuration values.
 */
using ConfigValue = std::variant<int, float, bool, std::string>;

/**
 * @brief Interface for classes that can be dynamically configured at runtime.
 */
class IConfigurable {
public:
    virtual ~IConfigurable() = default;

    /**
     * @brief Called when a configuration value has changed.
     * @param key The configuration key (e.g., "graphics.msaa").
     * @param value The new value.
     */
    virtual void OnConfigChanged(const std::string& key, const ConfigValue& value) = 0;
};
