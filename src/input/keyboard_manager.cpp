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
    return !m_Window->GetKey(key);

}

bool KeyboardManager::IsKeyDown(Input::Key key)
{
    bool currentlyPressed = GetKey(key);
    bool down = currentlyPressed && !m_PreviousState[key];
    m_PreviousState[key] = currentlyPressed;
    return down;
}
