#include <platform/logic/input_system.h>
KeyboardManager::KeyboardManager(IWindow *window)
{
    SetWindow(window);
}

void KeyboardManager::SetWindow(IWindow *window)
{
    m_Window = window;
}

void KeyboardManager::Update()
{
    if (!m_Window)
        return;

    m_PreviousState = m_CurrentState;

    for (auto &pair : m_PreviousState)
    {
        m_CurrentState[pair.first] = m_Window->GetKey(pair.first);
    }
}

bool KeyboardManager::GetKey(Key key) const
{
    if (!m_Window)
        return false;
    return m_Window->GetKey(key);
}

bool KeyboardManager::GetKeyUp(Key key) const
{
    if (!m_Window)
        return false;
    return !m_Window->GetKey(key);
}

bool KeyboardManager::IsKeyDown(Key key) const
{
    auto itCurr = m_CurrentState.find(key);
    auto itPrev = m_PreviousState.find(key);

    bool currentlyPressed = (itCurr != m_CurrentState.end()) ? itCurr->second : GetKey(key);
    bool previouslyPressed = (itPrev != m_PreviousState.end()) ? itPrev->second : false;

    if (itCurr == m_CurrentState.end() && m_Window)
    {
        const_cast<KeyboardManager *>(this)->m_CurrentState[key] = currentlyPressed;
    }

    return currentlyPressed && !previouslyPressed;
}
