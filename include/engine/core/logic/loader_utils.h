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
}  // namespace LoaderUtils
