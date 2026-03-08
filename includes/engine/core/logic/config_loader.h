#pragma once

#include <core/type/app_config.h>
#include <core/unit/engine_context.h>
#include <iostream>
#include <sstream>
#include <string>

class ConfigLoader
{
public:
    static void LoadConfig(std::stringstream &ss, EngineContext ctx);
    static void LoadConfig(std::stringstream &ss, AppConfig &config);
};