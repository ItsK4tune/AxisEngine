#pragma once

#include <memory>
#include <functional>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <core/keyboard_manager.h>
#include <core/mouse_manager.h>
#include <core/input_manager.h>

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

    void SetOnDebugLog(std::function<void()> callback) { m_OnDebugLog = callback; }
    void SetOnPhysicsDebugToggle(std::function<void()> callback) { m_OnPhysicsDebugToggle = callback; }

private:
    std::unique_ptr<KeyboardManager> m_KeyboardManager;
    std::unique_ptr<MouseManager> m_MouseManager;
    std::unique_ptr<InputManager> m_InputManager;

    std::function<void()> m_OnDebugLog;
    std::function<void()> m_OnPhysicsDebugToggle;
    bool m_F2Pressed = false;
    bool m_F1Pressed = false;
};
