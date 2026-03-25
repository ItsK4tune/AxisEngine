#pragma once

#include <functional>
#include <platform/interface/action.h>
#include <platform/interface/cursor_mode.h>
#include <platform/interface/gamepad.h>
#include <platform/interface/key.h>
#include <platform/interface/mouse.h>
#include <platform/type/device_info.h>
#include <platform/type/monitor_info.h>
#include <platform/type/window_mode.h>

class IWindow {
public:
    virtual ~IWindow() = default;

    virtual bool Initialize(int width, int height, const std::string& title, int msaaSamples = 0) = 0;
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

    virtual void SetCursorMode(CursorMode mode) = 0;
    virtual void SetAspectRatio(int numerator, int denominator) = 0;

    virtual void* GetNativeWindow() const = 0;

    virtual bool GetKey(Key key) const = 0;
    virtual bool GetMouseButton(Mouse button) const = 0;
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