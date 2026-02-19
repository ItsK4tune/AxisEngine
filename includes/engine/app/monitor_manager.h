#pragma once

#include <string>
#include <vector>
#include <interface/window/i_device_manager.h>

#include <memory>
#include <interface/window/i_window.h>

// struct GLFWwindow; // Removed forward declaration as we don't return it anymore



class MonitorManager : public IDeviceManager
{
public:
    MonitorManager();
    ~MonitorManager();

    bool Init(std::unique_ptr<IWindow> window);
    void SetWindowConfiguration(int width, int height, WindowMode mode = WindowMode::Windowed, int monitorIndex = 0, int refreshRate = 0);
    void SetFrameRateLimit(int limit);
    void SetWindowTitle(const std::string& title);
    void SetWindowIcon(const std::string& path);
    void SetVsync(bool vsync);
    void OnResize(int width, int height);

    IWindow* GetWindow() const { return m_Window.get(); }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    int GetFrameRateLimit() const { return m_FrameRateLimit; }

    std::vector<DeviceInfo> GetAllDevices() const override;
    DeviceInfo GetCurrentDevice() const override;
    bool SetActiveDevice(const std::string& deviceId) override;

private:
    std::unique_ptr<IWindow> m_Window;
    
    std::string m_Title = "Axis Engine";
    int m_Width = 800;
    int m_Height = 600;
    WindowMode m_Mode = WindowMode::Windowed;
    int m_MonitorIndex = 0;
    int m_RefreshRate = 0;
    int m_FrameRateLimit = 0;
};
