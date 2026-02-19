#pragma once

#include <string>
#include <vector>

enum class DeviceType
{
    Monitor,
    Keyboard,
    Mouse,
    Joystick,
    AudioOutput,
    AudioInput
};

struct DeviceInfo
{
    std::string id;
    std::string name;
    DeviceType type;
    bool isDefault = false;
};

class IDeviceManager
{
public:
    virtual ~IDeviceManager() = default;

    virtual std::vector<DeviceInfo> GetAllDevices() const = 0;
    virtual DeviceInfo GetCurrentDevice() const = 0;
    virtual bool SetActiveDevice(const std::string& deviceId) = 0;
};
