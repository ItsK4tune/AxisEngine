#pragma once

#include <core/type/app_config.h>
#include <functional>
#include <string>

class ConfigLoader
{
public:
    static void LoadConfig(std::stringstream& ss, AppConfig& config, bool headless = false);
};
