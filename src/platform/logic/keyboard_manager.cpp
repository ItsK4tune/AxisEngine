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
    if (IsKeyConsumed(key))
        return false;
    return GetRawKey(key);
}

bool KeyboardManager::GetRawKey(Key key) const
{
    if (!m_Window)
        return false;
    return m_Window->GetKey(key);
}

bool KeyboardManager::GetKeyUp(Key key) const
{
    if (IsKeyConsumed(key))
        return false;
    const auto current = m_CurrentState.find(key);
    const auto previous = m_PreviousState.find(key);
    return current != m_CurrentState.end() && previous != m_PreviousState.end() && !current->second && previous->second;
}

bool KeyboardManager::IsKeyDown(Key key) const
{
    if (IsKeyConsumed(key))
        return false;
    auto itCurr = m_CurrentState.find(key);
    auto itPrev = m_PreviousState.find(key);

    bool currentlyPressed = (itCurr != m_CurrentState.end()) ? itCurr->second : GetRawKey(key);
    bool previouslyPressed = (itPrev != m_PreviousState.end()) ? itPrev->second : false;

    if (itCurr == m_CurrentState.end() && m_Window)
    {
        m_CurrentState[key] = currentlyPressed;
    }

    return currentlyPressed && !previouslyPressed;
}

void KeyboardManager::ConsumeKey(Key key)
{
    m_ConsumedKeys.insert(key);
    m_ReleasedConsumedKeys.erase(key);
}

void KeyboardManager::ReleaseConsumedKey(Key key)
{
    if (m_ConsumedKeys.contains(key))
        m_ReleasedConsumedKeys.insert(key);
}

bool KeyboardManager::IsKeyConsumed(Key key) const
{
    return m_ConsumedKeys.contains(key);
}

void KeyboardManager::EndFrame()
{
    for (const Key key : m_ReleasedConsumedKeys)
        m_ConsumedKeys.erase(key);
    m_ReleasedConsumedKeys.clear();
}
