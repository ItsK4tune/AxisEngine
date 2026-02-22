#pragma once

#include <string>

struct ResourceReloadEvent
{
    std::string name;
    std::string type;
    std::string filePath;
};
