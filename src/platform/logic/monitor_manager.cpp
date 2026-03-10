#include <platform/logic/monitor_manager.h>
#include <platform/interface/i_window.h>
#include <stb_image.h>
#include <core/logic/logger.h>

MonitorManager::MonitorManager()
{
}

MonitorManager::~MonitorManager()
{
}

bool MonitorManager::Initialize(std::unique_ptr<IWindow> window)
{

    m_Window = std::move(window);

    if (!m_Window->Initialize(m_Width, m_Height, m_Title))
    {
        LOGGER_ERROR("MonitorManager") << "Failed to initialize window";
        return false;
    }

    m_Window->SetWindowConfiguration(m_Width, m_Height, m_Mode, m_MonitorIndex, m_RefreshRate);

    return true;
}

void MonitorManager::SetWindowConfiguration(int width, int height, WindowMode mode, int monitorIndex, int refreshRate)
{
    if (m_Mode == WindowMode::Windowed)
    {
        m_lastWindowedWidth = m_Width;
        m_lastWindowedHeight = m_Height;
    }
    else
    {
        m_lastSpecialMode = m_Mode;
        m_lastSpecialWidth = m_Width;
        m_lastSpecialHeight = m_Height;
    }

    m_Width = width;
    m_Height = height;
    m_Mode = mode;
    m_MonitorIndex = monitorIndex;
    m_RefreshRate = refreshRate;

    if (m_Window)
    {
        m_Window->SetWindowConfiguration(width, height, mode, monitorIndex, refreshRate);
    }
}

void MonitorManager::SetFrameRateLimit(int limit)
{
    m_FrameRateLimit = limit;
}

void MonitorManager::SetWindowTitle(const std::string& title)
{
    m_Title = title;
    if (m_Window)
        m_Window->SetTitle(m_Title);
}

void MonitorManager::SetVsync(bool vsync)
{
    if (m_Window)
        m_Window->SetVsync(vsync);
}

void MonitorManager::SetWindowIcon(const std::string& path)
{
    if (!m_Window)
        return;

    LOGGER_INFO("MonitorManager") << "Attempting to load icon from: " << path;

    int width, height, channels;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);

    if (pixels)
    {
        LOGGER_INFO("MonitorManager") << "Icon loaded successfully (" << width << "x" << height << ")";
        m_Window->SetIcon(width, height, pixels);
        stbi_image_free(pixels);
    }
    else
    {
        LOGGER_ERROR("MonitorManager") << "Failed to load icon: " << path;
    }
}

void MonitorManager::OnResize(int width, int height)
{
    if (width == 0 || height == 0)
        return;
    m_Width = width;
    m_Height = height;
}

std::vector<DeviceInfo> MonitorManager::GetAllDevices() const
{
    std::vector<DeviceInfo> devices;
    if (!m_Window) return devices;

    std::vector<MonitorInfo> monitors = m_Window->GetMonitors();

    for (const auto& monitor : monitors)
    {
        DeviceInfo info;
        info.id = std::to_string(monitor.index);
        info.name = monitor.name;
        info.type = DeviceType::Monitor;
        info.isDefault = monitor.isPrimary;
        devices.push_back(info);
    }
    return devices;
}

DeviceInfo MonitorManager::GetCurrentDevice() const
{
    DeviceInfo info;
    info.type = DeviceType::Monitor;
    info.isDefault = false;

    if (!m_Window)
    {
        info.id = "-1";
        info.name = "Unknown";
        return info;
    }

    std::vector<MonitorInfo> monitors = m_Window->GetMonitors();

    if (m_MonitorIndex >= 0 && m_MonitorIndex < monitors.size())
    {

        if (monitors[m_MonitorIndex].index == m_MonitorIndex)
        {
             info.id = std::to_string(m_MonitorIndex);
             info.name = monitors[m_MonitorIndex].name;
             return info;
        }
    }

    info.id = std::to_string(m_MonitorIndex);
    info.name = "Monitor " + std::to_string(m_MonitorIndex);
    return info;
}

bool MonitorManager::SetActiveDevice(const std::string& deviceId)
{
    try
    {
        int index = std::stoi(deviceId);
        m_MonitorIndex = index;
        SetWindowConfiguration(m_Width, m_Height, m_Mode, m_MonitorIndex, m_RefreshRate);
        return true;
    }
    catch (...)
    {
    }
    return false;
}

void MonitorManager::ToggleFullscreen()
{
    if (m_Mode == WindowMode::Windowed)
    {
        SetWindowConfiguration(m_lastSpecialWidth, m_lastSpecialHeight, m_lastSpecialMode, m_MonitorIndex, 0);
    }
    else
    {
        SetWindowConfiguration(m_lastWindowedWidth, m_lastWindowedHeight, WindowMode::Windowed, m_MonitorIndex, 0);
    }
}
