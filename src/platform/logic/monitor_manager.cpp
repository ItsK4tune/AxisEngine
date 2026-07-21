#include <platform/logic/monitor_manager.h>
#include <core/logic/logger.h>
#include <platform/interface/i_window.h>
#include <resource/logic/stb_image_loader.h>
#include <algorithm>
#include <charconv>

MonitorManager::MonitorManager()
    : m_Width(1280),
      m_Height(720),
      m_lastWindowedWidth(1280),
      m_lastWindowedHeight(720),
      m_lastSpecialWidth(1920),
      m_lastSpecialHeight(1080)
{
    m_initialMode = m_Mode;
}

MonitorManager::~MonitorManager()
{
}

bool MonitorManager::Initialize(std::unique_ptr<IWindow> window)
{
    if (!window)
    {
        LOGGER_ERROR("MonitorManager") << "Cannot initialize with a null window";
        return false;
    }
    m_Window = std::move(window);

    if (!m_Window->Initialize(m_Width, m_Height, m_Title))
    {
        LOGGER_ERROR("MonitorManager") << "Failed to initialize window";
        return false;
    }

    if (m_Width <= 0)
        m_Width = 1280;
    if (m_Height <= 0)
        m_Height = 720;

    m_initialMode = m_Mode;
    SetWindowConfiguration(m_Width, m_Height, m_Mode, m_MonitorIndex, m_RefreshRate);

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
        m_Width = m_Window->GetWidth();
        m_Height = m_Window->GetHeight();

        if (mode == WindowMode::Windowed && m_Width > 0 && m_Height > 0)
        {
            m_Window->SetAspectRatio(m_Width, m_Height);
        }
        else if (m_Window)
        {
            m_Window->SetAspectRatio(0, 0);  // No constraint for fullscreen
        }
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
    unsigned char* pixels = StbImageLoader::Load(path.c_str(), &width, &height, &channels, 4, false);

    if (pixels)
    {
        LOGGER_INFO("MonitorManager") << "Icon loaded successfully (" << width << "x" << height << ")";
        m_Window->SetIcon(width, height, pixels);
        StbImageLoader::Free(pixels);
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
    if (!m_Window)
        return devices;

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

    const auto current = std::find_if(monitors.begin(), monitors.end(),
                                      [this](const MonitorInfo& monitor) { return monitor.index == m_MonitorIndex; });
    if (current != monitors.end())
    {
        info.id = std::to_string(current->index);
        info.name = current->name;
        info.isDefault = current->isPrimary;
        return info;
    }

    info.id = std::to_string(m_MonitorIndex);
    info.name = "Monitor " + std::to_string(m_MonitorIndex);
    return info;
}

bool MonitorManager::SetActiveDevice(const std::string& deviceId)
{
    if (!m_Window)
        return false;

    int index = 0;
    const auto [end, error] = std::from_chars(deviceId.data(), deviceId.data() + deviceId.size(), index);
    if (error != std::errc{} || end != deviceId.data() + deviceId.size() || index < 0)
        return false;

    const auto monitors = m_Window->GetMonitors();
    if (std::none_of(monitors.begin(), monitors.end(),
                     [index](const MonitorInfo& monitor) { return monitor.index == index; }))
        return false;

    m_MonitorIndex = index;
    SetWindowConfiguration(m_Width, m_Height, m_Mode, m_MonitorIndex, m_RefreshRate);
    return true;
}

void MonitorManager::ToggleFullscreen()
{
    if (m_Mode != m_initialMode)
    {
        // Return to initial mode
        SetWindowConfiguration(m_lastWindowedWidth, m_lastWindowedHeight, m_initialMode, m_MonitorIndex, m_RefreshRate);
        return;
    }

    if (m_Mode == WindowMode::Windowed)
    {
        SetWindowConfiguration(m_Width, m_Height, WindowMode::Fullscreen, m_MonitorIndex, 0);
    }
    else if (m_Mode == WindowMode::Fullscreen)
    {
        SetWindowConfiguration(m_Width, m_Height, WindowMode::Windowed, m_MonitorIndex, 0);
    }
    else if (m_Mode == WindowMode::BorderlessFullscreen || m_Mode == WindowMode::Borderless)
    {
        int nativeW = m_Width;
        int nativeH = m_Height;
        if (m_Window)
        {
            auto monitors = m_Window->GetMonitors();
            if (m_MonitorIndex >= 0 && m_MonitorIndex < monitors.size())
            {
                nativeW = monitors[m_MonitorIndex].width;
                nativeH = monitors[m_MonitorIndex].height;
            }
        }
        SetWindowConfiguration(nativeW, nativeH, WindowMode::Windowed, m_MonitorIndex, 0);
    }
}
