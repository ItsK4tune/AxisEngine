#include <platform/strategy/opengl/glfw_window.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <platform/strategy/opengl/glfw_translator.h>
#include <resource/logic/stb_image_loader.h>
#include <core/logic/service_locator.h>
#include <core/logic/config_manager.h>
#include <algorithm>

#ifdef _WIN32
#undef APIENTRY
#include <windows.h>
#include <asset/project/resource.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#endif

namespace
{
#ifdef _WIN32
std::wstring BuildAppUserModelId()
{
    wchar_t exePath[MAX_PATH] = {};
    DWORD length = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring exeName = length > 0 ? std::wstring(exePath, length) : L"AxisEngineApplication";

    size_t slash = exeName.find_last_of(L"\\/");
    if (slash != std::wstring::npos)
        exeName = exeName.substr(slash + 1);

    size_t dot = exeName.find_last_of(L'.');
    if (dot != std::wstring::npos)
        exeName = exeName.substr(0, dot);

    for (wchar_t& ch : exeName)
    {
        const bool alphaNumeric =
            (ch >= L'0' && ch <= L'9') || (ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z');
        if (!alphaNumeric)
            ch = L'.';
    }

    return L"AxisEngine." + exeName;
}

void SetProcessAppUserModelId()
{
    using SetAppUserModelIdFn = HRESULT(WINAPI*)(PCWSTR);

    HMODULE shell32 = GetModuleHandleW(L"shell32.dll");
    if (!shell32)
        shell32 = LoadLibraryW(L"shell32.dll");
    if (!shell32)
        return;

    auto setAppUserModelId =
        reinterpret_cast<SetAppUserModelIdFn>(GetProcAddress(shell32, "SetCurrentProcessExplicitAppUserModelID"));
    if (!setAppUserModelId)
        return;

    std::wstring appId = BuildAppUserModelId();
    setAppUserModelId(appId.c_str());
}

HICON LoadSharedAppIcon(int width, int height)
{
    return static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, width,
                                         height, LR_DEFAULTCOLOR | LR_SHARED));
}

bool ApplyEmbeddedWindowIcon(GLFWwindow* window)
{
    if (!window)
        return false;

    HWND hwnd = glfwGetWin32Window(window);
    if (!hwnd)
        return false;

    HICON bigIcon = LoadSharedAppIcon(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    if (!bigIcon)
        bigIcon = LoadSharedAppIcon(0, 0);

    HICON smallIcon = LoadSharedAppIcon(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
    if (!smallIcon)
        smallIcon = bigIcon;

    if (!bigIcon && !smallIcon)
        return false;

    if (bigIcon)
    {
        SendMessage(hwnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(bigIcon));
        SetClassLongPtr(hwnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(bigIcon));
    }

    if (smallIcon)
    {
        SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
        SetClassLongPtr(hwnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(smallIcon));
    }

    return true;
}
#endif
}  // namespace

GLFWWindow::GLFWWindow()
{
}

GLFWWindow::~GLFWWindow()
{
    Shutdown();
}

bool GLFWWindow::Initialize(int width, int height, const std::string& title, int msaaSamples)
{
    if (!glfwInit())
    {
        LOGGER_ERROR("GLFWWindow") << "Failed to initialize GLFW";
        return false;
    }

#ifdef __APPLE__
    LOGGER_ERROR("GLFWWindow")
        << "The built-in renderer requires OpenGL/GLSL 4.6, while macOS exposes at most OpenGL 4.1. "
           "A compatible graphics provider is required; refusing to start with shaders that cannot compile.";
    glfwTerminate();
    return false;
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (msaaSamples > 0)
    {
        glfwWindowHint(GLFW_SAMPLES, msaaSamples);
    }

    glfwWindowHint(GLFW_STENCIL_BITS, 8);

#ifdef _WIN32
    SetProcessAppUserModelId();
#endif

    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Window)
    {
        LOGGER_ERROR("GLFWWindow") << "Failed to create window";
        glfwTerminate();
        return false;
    }

#ifdef _WIN32
    if (!ApplyEmbeddedWindowIcon(m_Window))
    {
        int w, h, ch;
        std::string iconPath = FileSystem::getEngineAssetPath("project/icon.png");
        unsigned char* pixels = StbImageLoader::Load(iconPath.c_str(), &w, &h, &ch, 4, false);
        if (pixels)
        {
            GLFWimage images[1];
            images[0].width = w;
            images[0].height = h;
            images[0].pixels = pixels;
            glfwSetWindowIcon(m_Window, 1, images);
            StbImageLoader::Free(pixels);
            LOGGER_INFO("GLFWWindow") << "Loaded window icon from file: " << iconPath;
        }
        else
        {
            LOGGER_WARN("GLFWWindow") << "Failed to load window icon from file: " << iconPath;
        }
    }
#endif

    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, this);

    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
    glfwSetKeyCallback(m_Window, key_callback);
    glfwSetMouseButtonCallback(m_Window, mouse_button_callback);
    glfwSetCursorPosCallback(m_Window, cursor_position_callback);
    glfwSetScrollCallback(m_Window, scroll_callback);
    glfwSetWindowFocusCallback(m_Window, focus_callback);

    m_Width = width;
    m_Height = height;

    return true;
}

void GLFWWindow::SetTitle(const std::string& title)
{
    if (m_Window)
        glfwSetWindowTitle(m_Window, title.c_str());
}

void GLFWWindow::SetIcon(int width, int height, unsigned char* pixels)
{
    if (m_Window && pixels)
    {
        GLFWimage images[1];
        images[0].width = width;
        images[0].height = height;
        images[0].pixels = pixels;
        glfwSetWindowIcon(m_Window, 1, images);
#ifdef _WIN32
        ApplyEmbeddedWindowIcon(m_Window);
#endif
    }
}

void GLFWWindow::SetVsync(bool enabled)
{
    if (m_Window)
        glfwSwapInterval(enabled ? 1 : 0);
}

void GLFWWindow::Update()
{
    PollEvents();
    SwapBuffers();
}

void GLFWWindow::Shutdown()
{
    if (m_Window)
    {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    glfwTerminate();
}

bool GLFWWindow::ShouldClose() const
{
    return glfwWindowShouldClose(m_Window);
}

void GLFWWindow::SetShouldClose(bool value)
{
    glfwSetWindowShouldClose(m_Window, value);
}

void GLFWWindow::SwapBuffers()
{
    glfwSwapBuffers(m_Window);
}

void GLFWWindow::PollEvents()
{
    glfwPollEvents();
}

int GLFWWindow::GetWidth() const
{
    return m_Width;
}
int GLFWWindow::GetHeight() const
{
    return m_Height;
}

void GLFWWindow::SetCursorMode(CursorMode mode)
{
    int glfwMode = GLFWTranslator::ToGLFWCursorMode(mode);
    glfwSetInputMode(m_Window, GLFW_CURSOR, glfwMode);

    auto* cm = ServiceLocator::Instance().Resolve<ConfigManager>();
    bool enableRaw = false;
    if (cm && cm->GetConfigSnapshot()->input.rawMouseInput &&
        (mode == CursorMode::Locked || mode == CursorMode::LockedHidden))
    {
        enableRaw = true;
    }

    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(m_Window, GLFW_RAW_MOUSE_MOTION, enableRaw ? GLFW_TRUE : GLFW_FALSE);
    }
}

void GLFWWindow::SetAspectRatio(int numerator, int denominator)
{
    if (m_Window)
    {
        glfwSetWindowAspectRatio(m_Window, numerator, denominator);
    }
}

void GLFWWindow::SetWindowConfiguration(int width, int height, WindowMode mode, int monitorIndex, int refreshRate)
{
    if (!m_Window)
        return;

    int count;
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    GLFWmonitor* targetMonitor = nullptr;

    if (monitorIndex >= 0 && monitorIndex < count)
        targetMonitor = monitors[monitorIndex];
    else if (count > 0)
        targetMonitor = monitors[0];

    if (!targetMonitor)
        targetMonitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* videoMode = glfwGetVideoMode(targetMonitor);
    if (!videoMode)
        return;

    if (width <= 0)
        width = videoMode->width;
    if (height <= 0)
        height = videoMode->height;

    int targetRefreshRate = (refreshRate > 0) ? refreshRate : videoMode->refreshRate;

    if (mode == WindowMode::Fullscreen)
    {
        glfwSetWindowMonitor(m_Window, targetMonitor, 0, 0, width, height, targetRefreshRate);
        m_Width = width;
        m_Height = height;
        if (m_ResizeCallback)
            m_ResizeCallback(m_Width, m_Height);
        LOGGER_INFO("GLFWWindow") << "Window set to Exclusive Fullscreen: " << width << "x" << height << "@"
                                  << targetRefreshRate;
    }
    else if (mode == WindowMode::BorderlessFullscreen)
    {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_FALSE);
        int xpos, ypos;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);
        m_Width = videoMode->width;
        m_Height = videoMode->height;
        if (m_ResizeCallback)
            m_ResizeCallback(m_Width, m_Height);
        glfwSetWindowMonitor(m_Window, nullptr, xpos, ypos, m_Width, m_Height, GLFW_DONT_CARE);
        LOGGER_INFO("GLFWWindow") << "Window set to Borderless Fullscreen: " << m_Width << "x" << m_Height;
    }
    else if (mode == WindowMode::Borderless)
    {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_FALSE);
        int xpos, ypos;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);

        int cx = xpos + (videoMode->width - width) / 2;
        int cy = ypos + (videoMode->height - height) / 2;

        glfwSetWindowMonitor(m_Window, nullptr, cx, cy, width, height, GLFW_DONT_CARE);
        m_Width = width;
        m_Height = height;
        if (m_ResizeCallback)
            m_ResizeCallback(m_Width, m_Height);
        LOGGER_INFO("GLFWWindow") << "Window set to Borderless: " << width << "x" << height << " at (" << cx << ","
                                  << cy << ")";
    }
    else
    {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_TRUE);

        int xpos, ypos;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);

        if (width <= 100)
            width = 1280;
        if (height <= 100)
            height = 720;

        int cx = xpos + (videoMode->width - width) / 2;
        int cy = ypos + (videoMode->height - height) / 2;

        if (cy < ypos + 30)
            cy = ypos + 30;

        glfwSetWindowMonitor(m_Window, nullptr, cx, cy, width, height, GLFW_DONT_CARE);

        glfwRestoreWindow(m_Window);
        glfwShowWindow(m_Window);

        m_Width = width;
        m_Height = height;

        // Force a viewport update
        if (m_ResizeCallback)
            m_ResizeCallback(m_Width, m_Height);

        LOGGER_INFO("GLFWWindow") << "Window set to Windowed: " << width << "x" << height << " at (" << cx << "," << cy
                                  << ")";
    }
}

std::vector<MonitorInfo> GLFWWindow::GetMonitors() const
{
    std::vector<MonitorInfo> monitors;
    int count;
    GLFWmonitor** glMonitors = glfwGetMonitors(&count);
    GLFWmonitor* primary = glfwGetPrimaryMonitor();

    for (int i = 0; i < count; i++)
    {
        MonitorInfo info;
        info.index = i;
        const char* name = glfwGetMonitorName(glMonitors[i]);
        info.name = name ? name : "Unknown";
        const GLFWvidmode* mode = glfwGetVideoMode(glMonitors[i]);
        if (mode)
        {
            info.width = mode->width;
            info.height = mode->height;
            info.refreshRate = mode->refreshRate;
        }
        else
        {
            info.width = 0;
            info.height = 0;
            info.refreshRate = 0;
        }
        info.isPrimary = (glMonitors[i] == primary);
        monitors.push_back(info);
    }
    return monitors;
}

std::vector<DeviceInfo> GLFWWindow::GetConnectedDevices() const
{
    std::vector<DeviceInfo> devices;

    DeviceInfo kbdInfo;
    kbdInfo.id = "keyboard_0";
    kbdInfo.name = "Primary Keyboard";
    kbdInfo.type = DeviceType::Keyboard;
    kbdInfo.isDefault = true;
    devices.push_back(kbdInfo);

    DeviceInfo mouseInfo;
    mouseInfo.id = "mouse_0";
    mouseInfo.name = "Primary Mouse";
    mouseInfo.type = DeviceType::Mouse;
    mouseInfo.isDefault = true;
    devices.push_back(mouseInfo);

    for (int i = 0; i <= GLFW_JOYSTICK_LAST; i++)
    {
        if (glfwJoystickPresent(i))
        {
            DeviceInfo joyInfo;
            const bool isGamepad = glfwJoystickIsGamepad(i) == GLFW_TRUE;
            joyInfo.id = std::string(isGamepad ? "gamepad_" : "joystick_") + std::to_string(i);
            const char* name = glfwGetJoystickName(i);
            joyInfo.name = name ? name : "Unknown Joystick";
            joyInfo.type = isGamepad ? DeviceType::Gamepad : DeviceType::Joystick;
            joyInfo.isDefault = false;
            devices.push_back(joyInfo);
        }
    }
    return devices;
}

void* GLFWWindow::GetNativeWindow() const
{
    return m_Window;
}

bool GLFWWindow::GetKey(Key key) const
{
    if (!m_Window)
        return false;
    return glfwGetKey(m_Window, GLFWTranslator::ToGLFWKey(key)) == GLFW_PRESS;
}

bool GLFWWindow::GetMouseButton(Mouse button) const
{
    if (!m_Window)
        return false;
    return glfwGetMouseButton(m_Window, GLFWTranslator::ToGLFWMouse(button)) == GLFW_PRESS;
}

bool GLFWWindow::GetGamepadButton(int deviceIndex, Gamepad button) const
{
    if (deviceIndex < GLFW_JOYSTICK_1 || deviceIndex > GLFW_JOYSTICK_LAST || !glfwJoystickPresent(deviceIndex))
        return false;

    const int buttonIndex = static_cast<int>(button);
    if (glfwJoystickIsGamepad(deviceIndex))
    {
        GLFWgamepadstate state{};
        return glfwGetGamepadState(deviceIndex, &state) == GLFW_TRUE && buttonIndex >= 0 &&
               buttonIndex <= GLFW_GAMEPAD_BUTTON_LAST && state.buttons[buttonIndex] == GLFW_PRESS;
    }

    int count = 0;
    const unsigned char* buttons = glfwGetJoystickButtons(deviceIndex, &count);
    return buttons && buttonIndex >= 0 && buttonIndex < count && buttons[buttonIndex] == GLFW_PRESS;
}

float GLFWWindow::GetGamepadAxis(int deviceIndex, GamepadAxis axis) const
{
    if (deviceIndex < GLFW_JOYSTICK_1 || deviceIndex > GLFW_JOYSTICK_LAST || !glfwJoystickPresent(deviceIndex))
        return 0.0f;
    const int axisIndex = static_cast<int>(axis);
    float value = 0.0f;
    if (glfwJoystickIsGamepad(deviceIndex))
    {
        GLFWgamepadstate state{};
        if (glfwGetGamepadState(deviceIndex, &state) != GLFW_TRUE || axisIndex < 0 || axisIndex > GLFW_GAMEPAD_AXIS_LAST)
            return 0.0f;
        value = state.axes[axisIndex];
    }
    else
    {
        int count = 0;
        const float* axes = glfwGetJoystickAxes(deviceIndex, &count);
        if (!axes || axisIndex < 0 || axisIndex >= count)
            return 0.0f;
        value = axes[axisIndex];
    }
    if (axis == GamepadAxis::LeftTrigger || axis == GamepadAxis::RightTrigger)
        return (std::clamp)(value * 0.5f + 0.5f, 0.0f, 1.0f);
    return (std::clamp)(value, -1.0f, 1.0f);
}

void GLFWWindow::GetCursorPos(double& x, double& y) const
{
    if (m_Window)
        glfwGetCursorPos(m_Window, &x, &y);
    else
    {
        x = 0;
        y = 0;
    }
}

void GLFWWindow::SetCursorPos(double x, double y)
{
    if (m_Window)
        glfwSetCursorPos(m_Window, x, y);
}

void GLFWWindow::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self)
    {
        self->m_Width = width;
        self->m_Height = height;
        if (self->m_ResizeCallback)
            self->m_ResizeCallback(width, height);
    }
}

void GLFWWindow::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_KeyCallback)
    {
        self->m_KeyCallback((int)GLFWTranslator::ToInputKey(key), scancode, action, mods);
    }
}

void GLFWWindow::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_MouseButtonCallback)
    {
        self->m_MouseButtonCallback((int)GLFWTranslator::ToInputMouse(button), action, mods);
    }
}

void GLFWWindow::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_CursorPosCallback)
    {
        self->m_CursorPosCallback(xpos, ypos);
    }
}

void GLFWWindow::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_ScrollCallback)
    {
        self->m_ScrollCallback(xoffset, yoffset);
    }
}

void GLFWWindow::focus_callback(GLFWwindow* window, int focused)
{
    if (auto* self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window)); self && self->m_FocusCallback)
        self->m_FocusCallback(focused == GLFW_TRUE);
}
