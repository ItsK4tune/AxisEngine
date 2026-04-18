#pragma once

#include <functional>
#include <memory>
#include <platform/interface/i_window.h>
#include <platform/logic/input_manager.h>
#include <platform/logic/keyboard_manager.h>
#include <platform/logic/mouse_manager.h>
#include <platform/logic/monitor_manager.h>

class IGraphicsContext;


class IOHandler
{
public:
    IOHandler();
    ~IOHandler();

    bool Initialize(std::unique_ptr<IWindow> window, const std::string& title, int width, int height, int windowMode, int monitorIndex, int refreshRate, bool vsync, int frameRateLimit);
    void SetWindow(IWindow* window);
    void ProcessInput();

    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

    MonitorManager& GetMonitorManager() { return *m_MonitorManager; }
    KeyboardManager& GetKeyboard() const { return *m_KeyboardManager; }
    MouseManager& GetMouse() const { return *m_MouseManager; }
    InputManager& GetInputManager() const { return *m_InputManager; }

    std::unique_ptr<MonitorManager> m_MonitorManager;
    std::unique_ptr<KeyboardManager> m_KeyboardManager;
    std::unique_ptr<MouseManager> m_MouseManager;
    std::unique_ptr<InputManager> m_InputManager;
};