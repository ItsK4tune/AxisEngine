#pragma once

#include <platform/interface/device_type.h>
#include <string>

struct DeviceInfo
{
    std::string id;
    std::string name;
    DeviceType type;
    bool isDefault;
};