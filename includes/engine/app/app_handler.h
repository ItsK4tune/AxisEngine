#pragma once

#include <memory>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <input/keyboard_manager.h>
#include <input/mouse_manager.h>
#include <input/input_manager.h>

class AppHandler
{
public:
    AppHandler(GLFWwindow* window);
    ~AppHandler();

    void ProcessInput(GLFWwindow* window);

    // Callbacks
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(int button, int action, int mods);
    void OnScroll(double xoffset, double yoffset);

    // Accessors
    KeyboardManager& GetKeyboard() const { return *m_KeyboardManager; }
    MouseManager& GetMouse() const { return *m_MouseManager; }
    InputManager& GetInputManager() const { return *m_InputManager; }

private:
    std::unique_ptr<KeyboardManager> m_KeyboardManager;
    std::unique_ptr<MouseManager> m_MouseManager;
    std::unique_ptr<InputManager> m_InputManager;
};
