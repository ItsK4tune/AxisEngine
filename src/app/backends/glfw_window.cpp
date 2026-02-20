#include <engine/app/backends/glfw_window.h>
#include <app/backends/glfw_translator.h>
#include <utils/logger.h>
#include <iostream>

namespace {

}

GLFWWindow::GLFWWindow() {}

GLFWWindow::~GLFWWindow() {
    Shutdown();
}

bool GLFWWindow::Init(int width, int height, const std::string& title) {
    if (!glfwInit()) {
        LOGGER_ERROR("GLFWWindow") << "Failed to initialize GLFW";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_Window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        LOGGER_ERROR("GLFWWindow") << "Failed to create window";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, this);

    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
    glfwSetKeyCallback(m_Window, key_callback);
    glfwSetMouseButtonCallback(m_Window, mouse_button_callback);
    glfwSetCursorPosCallback(m_Window, cursor_position_callback);
    glfwSetScrollCallback(m_Window, scroll_callback);

    m_Width = width;
    m_Height = height;

    return true;
}

void GLFWWindow::SetTitle(const std::string& title) {
    if (m_Window) glfwSetWindowTitle(m_Window, title.c_str());
}

void GLFWWindow::SetIcon(int width, int height, unsigned char* pixels) {
    if (m_Window && pixels) {
        GLFWimage images[1];
        images[0].width = width;
        images[0].height = height;
        images[0].pixels = pixels;
        glfwSetWindowIcon(m_Window, 1, images);
    }
}

void GLFWWindow::SetVsync(bool enabled) {
    if (m_Window) glfwSwapInterval(enabled ? 1 : 0);
}

void GLFWWindow::Update() {
    PollEvents();
    SwapBuffers();
}

void GLFWWindow::Shutdown() {
    if (m_Window) {
        glfwDestroyWindow(m_Window);
        m_Window = nullptr;
    }
    glfwTerminate();
}

bool GLFWWindow::ShouldClose() const {
    return glfwWindowShouldClose(m_Window);
}

void GLFWWindow::SetShouldClose(bool value) {
    glfwSetWindowShouldClose(m_Window, value);
}

void GLFWWindow::SwapBuffers() {
    glfwSwapBuffers(m_Window);
}

void GLFWWindow::PollEvents() {
    glfwPollEvents();
}

int GLFWWindow::GetWidth() const { return m_Width; }
int GLFWWindow::GetHeight() const { return m_Height; }

void GLFWWindow::SetCursorMode(Input::CursorMode mode) {
    int glfwMode = GLFWTranslator::ToGLFWCursorMode(mode);
    glfwSetInputMode(m_Window, GLFW_CURSOR, glfwMode);
}

void GLFWWindow::SetWindowConfiguration(int width, int height, WindowMode mode, int monitorIndex, int refreshRate) {
    if (!m_Window) return;

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

    if (mode == WindowMode::Fullscreen) {
        glfwSetWindowMonitor(m_Window, targetMonitor, 0, 0, width, height, targetRefreshRate);
    } else if (mode == WindowMode::Borderless) {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_FALSE);
        int xpos, ypos;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);
        m_Width = videoMode->width;
        m_Height = videoMode->height;
        glfwSetWindowMonitor(m_Window, nullptr, xpos, ypos, videoMode->width, videoMode->height, targetRefreshRate);
    } else {
        glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_TRUE);
        int xpos, ypos;
        glfwGetMonitorPos(targetMonitor, &xpos, &ypos);
        int cx = xpos + (videoMode->width - width) / 2;
        int cy = ypos + (videoMode->height - height) / 2;
        glfwSetWindowMonitor(m_Window, nullptr, cx, cy, width, height, targetRefreshRate);
        m_Width = width;
        m_Height = height;
    }
}

std::vector<MonitorInfo> GLFWWindow::GetMonitors() const {
    std::vector<MonitorInfo> monitors;
    int count;
    GLFWmonitor** glMonitors = glfwGetMonitors(&count);
    GLFWmonitor* primary = glfwGetPrimaryMonitor();

    for (int i = 0; i < count; i++) {
        MonitorInfo info;
        info.index = i;
        const char* name = glfwGetMonitorName(glMonitors[i]);
        info.name = name ? name : "Unknown";
        const GLFWvidmode* mode = glfwGetVideoMode(glMonitors[i]);
        if (mode) {
            info.width = mode->width;
            info.height = mode->height;
            info.refreshRate = mode->refreshRate;
        } else {
            info.width = 0;
            info.height = 0;
            info.refreshRate = 0;
        }
        info.isPrimary = (glMonitors[i] == primary);
        monitors.push_back(info);
    }
    return monitors;
}

std::vector<DeviceInfo> GLFWWindow::GetConnectedDevices() const {
    std::vector<DeviceInfo> devices;

    // Default Keyboard & Mouse
    DeviceInfo kbdInfo;
    kbdInfo.id = "keyboard_0";
    kbdInfo.name = "Primary Keyboard";
    kbdInfo.type = AxisDeviceType::Keyboard;
    kbdInfo.isDefault = true;
    devices.push_back(kbdInfo);

    DeviceInfo mouseInfo;
    mouseInfo.id = "mouse_0";
    mouseInfo.name = "Primary Mouse";
    mouseInfo.type = AxisDeviceType::Mouse;
    mouseInfo.isDefault = true;
    devices.push_back(mouseInfo);

    // Joysticks
    for (int i = 0; i <= GLFW_JOYSTICK_LAST; i++) {
        if (glfwJoystickPresent(i)) {
            DeviceInfo joyInfo;
            joyInfo.id = std::to_string(i);
            const char* name = glfwGetJoystickName(i);
            joyInfo.name = name ? name : "Unknown Joystick";
            joyInfo.type = AxisDeviceType::Joystick;
            joyInfo.isDefault = false;
            devices.push_back(joyInfo);
        }
    }
    return devices;
}

void* GLFWWindow::GetNativeWindow() const {
    return m_Window;
}

bool GLFWWindow::GetKey(Input::Key key) const {
    if (!m_Window) return false;
    return glfwGetKey(m_Window, GLFWTranslator::ToGLFWKey(key)) == GLFW_PRESS;
}

bool GLFWWindow::GetMouseButton(Input::Mouse button) const {
    if (!m_Window) return false;
    return glfwGetMouseButton(m_Window, GLFWTranslator::ToGLFWMouse(button)) == GLFW_PRESS;
}

void GLFWWindow::GetCursorPos(double& x, double& y) const {
    if (m_Window) glfwGetCursorPos(m_Window, &x, &y);
    else { x = 0; y = 0; }
}

void GLFWWindow::SetCursorPos(double x, double y) {
    if (m_Window) glfwSetCursorPos(m_Window, x, y);
}

void GLFWWindow::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self) {
        self->m_Width = width;
        self->m_Height = height;
        if (self->m_ResizeCallback) self->m_ResizeCallback(width, height);
    }
}

void GLFWWindow::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_KeyCallback) {

        self->m_KeyCallback((int)GLFWTranslator::ToInputKey(key), scancode, action, mods);
    }
}

void GLFWWindow::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_MouseButtonCallback) {
        self->m_MouseButtonCallback((int)GLFWTranslator::ToInputMouse(button), action, mods);
    }
}

void GLFWWindow::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_CursorPosCallback) {
        self->m_CursorPosCallback(xpos, ypos);
    }
}

void GLFWWindow::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto self = static_cast<GLFWWindow*>(glfwGetWindowUserPointer(window));
    if (self && self->m_ScrollCallback) {
        self->m_ScrollCallback(xoffset, yoffset);
    }
}
