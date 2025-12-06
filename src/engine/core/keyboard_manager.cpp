#include <engine/core/keyboard_manager.h>

void KeyboardManager::Init(GLFWwindow *window)
{
    m_Window = window;
}

bool KeyboardManager::GetKey(int key) const
{
    return glfwGetKey(m_Window, key) == GLFW_PRESS;
}

bool KeyboardManager::GetKeyUp(int key) const
{
    return glfwGetKey(m_Window, key) == GLFW_RELEASE;
}

bool KeyboardManager::IsKeyDown(int key)
{
    bool currentlyPressed = GetKey(key);
    bool down = currentlyPressed && !m_PreviousState[key];
    m_PreviousState[key] = currentlyPressed;
    return down;
}