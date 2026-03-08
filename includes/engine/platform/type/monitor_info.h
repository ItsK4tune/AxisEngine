#pragma once

#include <string>

struct MonitorInfo
{
    std::string name;
    int index;
    int width;
    int height;
    int refreshRate;
    bool isPrimary;
};