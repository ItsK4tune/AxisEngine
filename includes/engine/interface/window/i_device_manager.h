#pragma once

#include <string>
#include <vector>

#include <interface/window/i_window.h>

class IDeviceManager
{
public:
    virtual ~IDeviceManager() = default;

    virtual std::vector<DeviceInfo> GetAllDevices() const = 0;
    virtual DeviceInfo GetCurrentDevice() const = 0;
    virtual bool SetActiveDevice(const std::string& deviceId) = 0;
};
