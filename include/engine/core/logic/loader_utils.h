#pragma once

#include <core/logic/logger.h>
#include <core/logic/yaml_parser.h>
#include <algorithm>
#include <string>
#include <vector>

namespace LoaderUtils
{

inline void ValidateKeys(const YAMLNode& node, const std::vector<std::string>& allowedKeys,
                         const std::string& componentName)
{
    for (const auto& child : node.children)
    {
        if (std::find(allowedKeys.begin(), allowedKeys.end(), child.key) == allowedKeys.end())
        {
            LOGGER_WARN("Loader") << "Unknown key '" << child.key << "' in component '" << componentName << "'";
        }
    }
}

inline int SafeStoi(const std::string& str, int defaultValue = 0)
{
    if (str.empty()) return defaultValue;
    try
    {
        return std::stoi(str);
    }
    catch (...)
    {
        LOGGER_WARN("Loader") << "Invalid integer value: '" << str << "'. Using default: " << defaultValue;
        return defaultValue;
    }
}

inline float SafeStof(const std::string& str, float defaultValue = 0.0f)
{
    if (str.empty()) return defaultValue;
    try
    {
        return std::stof(str);
    }
    catch (...)
    {
        LOGGER_WARN("Loader") << "Invalid float value: '" << str << "'. Using default: " << defaultValue;
        return defaultValue;
    }
}

inline unsigned long SafeStoul(const std::string& str, unsigned long defaultValue = 0)
{
    if (str.empty()) return defaultValue;
    try
    {
        return std::stoul(str);
    }
    catch (...)
    {
        LOGGER_WARN("Loader") << "Invalid unsigned long value: '" << str << "'. Using default: " << defaultValue;
        return defaultValue;
    }
}
}  // namespace LoaderUtils
