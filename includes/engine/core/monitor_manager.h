#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <core/device_manager.h>

enum class WindowMode
{
    WINDOWED,
    FULLSCREEN,
    BORDERLESS
};

class MonitorManager : public IDeviceManager
{
public:
    MonitorManager();
    ~MonitorManager();

    bool Init();
    void SetWindowConfiguration(int width, int height, WindowMode mode = WindowMode::WINDOWED, int monitorIndex = 0, int refreshRate = 0);
    void SetVsync(bool enable);
    void SetFrameRateLimit(int limit);
    void SetWindowTitle(const std::string& title);
    void SetWindowIcon(const std::string& path);
    void OnResize(int width, int height);

    GLFWwindow* GetWindow() const { return m_Window; }
    int GetWidth() const { return m_Width; }
    int GetHeight() const { return m_Height; }
    int GetFrameRateLimit() const { return m_FrameRateLimit; }

    // IDeviceManager Implementation
    std::vector<DeviceInfo> GetAllDevices() const override;
    DeviceInfo GetCurrentDevice() const override;
    bool SetActiveDevice(const std::string& deviceId) override;

private:
    GLFWwindow* m_Window = nullptr;
    
    // Default configuration
    std::string m_Title = "Game Engine";
    int m_Width = 800;
    int m_Height = 600;
    bool m_Vsync = false;
    WindowMode m_Mode = WindowMode::WINDOWED;
    int m_MonitorIndex = 0;
    int m_RefreshRate = 0;
    int m_FrameRateLimit = 0;
};
