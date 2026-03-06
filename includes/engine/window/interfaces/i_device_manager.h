#pragma once

#include <window/interfaces/i_window.h>
#include <string>
#include <vector>

class IDeviceManager
{
public:
    virtual ~IDeviceManager() = default;

    virtual std::vector<DeviceInfo> GetAllDevices() const = 0;
    virtual DeviceInfo GetCurrentDevice() const = 0;
    virtual bool SetActiveDevice(const std::string& deviceId) = 0;
};
