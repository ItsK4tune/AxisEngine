#pragma once

#include <core/type/app_config.h>
#include <string>
#include <functional>

class ConfigLoader
{
public:
    static void LoadConfig(std::stringstream &ss, AppConfig &config, bool headless = false);
};