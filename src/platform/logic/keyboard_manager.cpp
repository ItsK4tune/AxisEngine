#include <platform/logic/keyboard_manager.h>

KeyboardManager::KeyboardManager(IWindow* window)
{
    SetWindow(window);
}

void KeyboardManager::SetWindow(IWindow* window)
{
    m_Window = window;
}

void KeyboardManager::Update()
{
    if (!m_Window)
        return;

    m_PreviousState = m_CurrentState;

    for (auto& pair : m_PreviousState)
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
    const auto current = m_CurrentState.find(key);
    const auto previous = m_PreviousState.find(key);
    return current != m_CurrentState.end() && previous != m_PreviousState.end() && !current->second && previous->second;
}

bool KeyboardManager::IsKeyDown(Key key) const
{
    auto itCurr = m_CurrentState.find(key);
    auto itPrev = m_PreviousState.find(key);

    bool currentlyPressed = (itCurr != m_CurrentState.end()) ? itCurr->second : GetKey(key);
    bool previouslyPressed = (itPrev != m_PreviousState.end()) ? itPrev->second : false;

    if (itCurr == m_CurrentState.end() && m_Window)
    {
        m_CurrentState[key] = currentlyPressed;
    }

    return currentlyPressed && !previouslyPressed;
}
