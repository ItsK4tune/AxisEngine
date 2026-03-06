#pragma once

#include <string>
#include <vector>

enum class WindowMode
{
    Windowed,
    Fullscreen,
    Borderless,
    BorderlessFullscreen
};

struct MonitorInfo
{
    std::string name;
    int index;
    int width;
    int height;
    int refreshRate;
    bool isPrimary;
};

enum class AxisDeviceType
{
    Monitor,
    Keyboard,
    Mouse,
    Joystick,
    Gamepad,
    AudioOutput,
    AudioInput
};

struct DeviceInfo
{
    std::string id;
    std::string name;
    AxisDeviceType type;
    bool isDefault;
};
