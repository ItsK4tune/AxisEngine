#include <app/monitor_manager.h>
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

MonitorManager::MonitorManager()
{
}

MonitorManager::~MonitorManager()
{
    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    glfwTerminate();
}

bool MonitorManager::Init()
{
    if (!glfwInit()) {
        std::cerr << "[MonitorManager] Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(m_Width, m_Height, m_Title.c_str(), NULL, NULL);
    if (m_Window == NULL)
    {
        std::cout << "[MonitorManager] Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(m_Window);

    if (m_Vsync) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "[MonitorManager] Failed to initialize GLAD" << std::endl;
        return false;
    }
    glEnable(GL_DEPTH_TEST);

    return true;
}

void MonitorManager::SetWindowConfiguration(int width, int height, WindowMode mode, int monitorIndex, int refreshRate)
{
    m_Width = width;
    m_Height = height;
    m_Mode = mode;
    m_MonitorIndex = monitorIndex;
    m_RefreshRate = refreshRate;

    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    GLFWmonitor* targetMonitor = nullptr;

    if (monitorIndex >= 0 && monitorIndex < count)
        targetMonitor = monitors[monitorIndex];
    else if (count > 0)
        targetMonitor = monitors[0];

    if (!targetMonitor) targetMonitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* videoMode = glfwGetVideoMode(targetMonitor);
    if (!videoMode) return;

    int targetRefreshRate = (refreshRate > 0) ? refreshRate : videoMode->refreshRate;

    if (mode == WindowMode::FULLSCREEN)
    {
        glfwSetWindowMonitor(m_Window, targetMonitor, 0, 0, width, height, targetRefreshRate);
    }
    else if (mode == WindowMode::BORDERLESS)
    {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_FALSE);
        
        int xpos = 0, ypos = 0;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);
        
        m_Width = videoMode->width;
        m_Height = videoMode->height;
        
        glfwSetWindowMonitor(m_Window, nullptr, xpos, ypos, videoMode->width, videoMode->height, targetRefreshRate);
    }
    else 
    {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_TRUE);
        
        int xpos = 0, ypos = 0;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);
        int cx = xpos + (videoMode->width - width) / 2;
        int cy = ypos + (videoMode->height - height) / 2;

        glfwSetWindowMonitor(m_Window, nullptr, cx, cy, width, height, targetRefreshRate);
    }

    glViewport(0, 0, m_Width, m_Height);
}

void MonitorManager::SetVsync(bool enable)
{
    m_Vsync = enable;
    glfwSwapInterval(enable ? 1 : 0);
}

void MonitorManager::SetFrameRateLimit(int limit)
{
    m_FrameRateLimit = limit;
}

void MonitorManager::SetWindowTitle(const std::string& title)
{
    m_Title = title;
    if (m_Window)
    {
        glfwSetWindowTitle(m_Window, m_Title.c_str());
    }
}

#include <stb_image.h>

void MonitorManager::SetWindowIcon(const std::string& path)
{
    if (!m_Window) return;

    GLFWimage images[1];
    int width, height, channels;
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4); // Force 4 channels (RGBA)

    if (pixels)
    {
        images[0].width = width;
        images[0].height = height;
        images[0].pixels = pixels;

        glfwSetWindowIcon(m_Window, 1, images);

        stbi_image_free(pixels);
    }
    else
    {
        std::cerr << "[MonitorManager] Failed to load icon: " << path << std::endl;
    }
}

void MonitorManager::OnResize(int width, int height)
{
    if (width == 0 || height == 0) return;

    m_Width = width;
    m_Height = height;
    glViewport(0, 0, width, height);
}

// IDeviceManager Implementation
std::vector<DeviceInfo> MonitorManager::GetAllDevices() const
{
    std::vector<DeviceInfo> devices;
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    
    for (int i = 0; i < count; i++)
    {
        DeviceInfo info;
        info.id = std::to_string(i);
        const char* name = glfwGetMonitorName(monitors[i]);
        info.name = name ? name : "Unknown Monitor";
        info.type = DeviceType::Monitor;
        info.isDefault = (i == 0); // Assuming 0 is primary usually
        devices.push_back(info);
    }
    return devices;
}

DeviceInfo MonitorManager::GetCurrentDevice() const
{
    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    
    DeviceInfo info;
    info.type = DeviceType::Monitor;
    info.isDefault = false;

    // Best effort to find current monitor.
    // Simplifying assumption: use m_MonitorIndex or check overlap.
    if (m_MonitorIndex >= 0 && m_MonitorIndex < count)
    {
        info.id = std::to_string(m_MonitorIndex);
        const char* name = glfwGetMonitorName(monitors[m_MonitorIndex]);
        info.name = name ? name : "Unknown Monitor";
    }
    else
    {
        info.id = "-1";
        info.name = "Unknown";
    }
    return info;
}

bool MonitorManager::SetActiveDevice(const std::string& deviceId)
{
    try {
        int index = std::stoi(deviceId);
        int count;
        glfwGetMonitors(&count);
        if (index >= 0 && index < count)
        {
            m_MonitorIndex = index;
            // Re-apply configuration to switch monitor
            SetWindowConfiguration(m_Width, m_Height, m_Mode, m_MonitorIndex, m_RefreshRate);
            return true;
        }
    } catch (...) {}
    return false;
}
