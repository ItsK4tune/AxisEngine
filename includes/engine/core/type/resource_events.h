#pragma once

#include <string>

struct ResourceReloadEvent
{
    std::string name;
    std::string type;
    std::string filePath;
};

struct ResourceLoadedEvent
{
    std::string name;
    std::string type;
    bool success;
};