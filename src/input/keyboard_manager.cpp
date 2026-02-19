#include <input/keyboard_manager.h>

KeyboardManager::KeyboardManager(IWindow *window)
{
    m_Window = window;
}

bool KeyboardManager::GetKey(Input::Key key) const
{
    if (!m_Window) return false;
    return m_Window->GetKey(key);
}

bool KeyboardManager::GetKeyUp(Input::Key key) const
{
    if (!m_Window) return false;
    return !m_Window->GetKey(key); // Default GetKey checks Press. KeyUp logic might need refined checking of release event or state.
    // Wait, GetKeyUp usually means "Was Pressed, Now Released". 
    // Or just "Is currently up"? 
    // Original code: `return glfwGetKey(m_Window, key) == GLFW_RELEASE;`
    // So it returns true if NOT pressed.
    // So !GetKey(key) is correct.
}

bool KeyboardManager::IsKeyDown(Input::Key key)
{
    bool currentlyPressed = GetKey(key);
    bool down = currentlyPressed && !m_PreviousState[key];
    m_PreviousState[key] = currentlyPressed;
    return down;
}