#ifndef I_WINDOW_H
#define I_WINDOW_H

#include <string>
#include <functional>

namespace Input {

}
#include "input_codes.h"

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

class IWindow {
public:
    virtual ~IWindow() = default;

    virtual bool Init(int width, int height, const std::string& title) = 0;
    virtual void SetTitle(const std::string& title) = 0;
    virtual void SetIcon(int width, int height, unsigned char* pixels) = 0;
    virtual void SetVsync(bool enabled) = 0;
    virtual void Update() = 0;
    virtual void Shutdown() = 0;

    virtual bool ShouldClose() const = 0;
    virtual void SetShouldClose(bool value) = 0;

    virtual void SwapBuffers() = 0;
    virtual void PollEvents() = 0;

    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;

    virtual void SetWindowConfiguration(int width, int height, WindowMode mode, int monitorIndex, int refreshRate) = 0;
    virtual std::vector<MonitorInfo> GetMonitors() const = 0;

    virtual void SetCursorMode(Input::CursorMode mode) = 0;

    virtual void* GetNativeWindow() const = 0;

    virtual bool GetKey(Input::Key key) const = 0;
    virtual bool GetMouseButton(Input::Mouse button) const = 0;
    virtual void GetCursorPos(double& x, double& y) const = 0;
    virtual void SetCursorPos(double x, double y) = 0;

    virtual std::vector<struct DeviceInfo> GetConnectedDevices() const = 0;

    using ResizeCallback = std::function<void(int, int)>;
    using KeyCallback = std::function<void(int, int, int, int)>;
    using MouseButtonCallback = std::function<void(int, int, int)>;
    using MousePositionCallback = std::function<void(double, double)>;
    using ScrollCallback = std::function<void(double, double)>;

    virtual void SetResizeCallback(const ResizeCallback& callback) = 0;
    virtual void SetKeyCallback(const KeyCallback& callback) = 0;
    virtual void SetMouseButtonCallback(const MouseButtonCallback& callback) = 0;
    virtual void SetCursorPosCallback(const MousePositionCallback& callback) = 0;
    virtual void SetScrollCallback(const ScrollCallback& callback) = 0;
};

#endif
