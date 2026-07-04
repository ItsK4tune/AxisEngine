#pragma once

#include <core/type/app_config.h>
#include <string>

class ConfigSerializer
{
public:
    static bool Deserialize(const std::string& filepath, AppConfig& config, bool headless = false);
    static bool Serialize(const std::string& filepath, const AppConfig& config);
};
