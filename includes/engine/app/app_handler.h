#pragma once

#include <memory>
#include <functional>
#include <interface/window/i_window.h>

#include <string>

// Forward declarations
class KeyboardManager;
class MouseManager;
class InputManager;

class AppHandler
{
public:
    AppHandler(IWindow* window);
    ~AppHandler();

    void ProcessInput(IWindow* window);

    void OnResize(int width, int height);
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

    KeyboardManager& GetKeyboard() const { return *m_KeyboardManager; }
    MouseManager& GetMouse() const { return *m_MouseManager; }
    InputManager& GetInputManager() const { return *m_InputManager; }

private:
    std::unique_ptr<KeyboardManager> m_KeyboardManager;
    std::unique_ptr<MouseManager> m_MouseManager;
    std::unique_ptr<InputManager> m_InputManager;
};
