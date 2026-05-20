#include <platform/logic/mouse_manager.h>

MouseManager::MouseManager(IWindow *window)
    : m_LastX(400.0), m_LastY(300.0),
      m_XOffset(0.0f), m_YOffset(0.0f), m_ScrollY(0.0f),
      m_FirstMouse(true),
      m_ForceFree(false),
      m_Mode(CursorMode::Normal)
{
    SetWindow(window);
    m_CurrentButtons.reset();
    m_PreviousButtons.reset();
}

void MouseManager::SetWindow(IWindow* window)
{
    m_Window = window;
}

void MouseManager::UpdatePosition(double xpos, double ypos)
{
    if (m_FirstMouse)
    {
        m_LastX = xpos;
        m_LastY = ypos;
        m_FirstMouse = false;
    }

    if (m_Mode == CursorMode::Locked)
    {
        m_XOffset += static_cast<float>(xpos - m_LockX);
        m_YOffset += static_cast<float>(m_LockY - ypos);

        m_LastX = m_LockX;
        m_LastY = m_LockY;
    }
    else
    {
        m_XOffset += static_cast<float>(xpos - m_LastX);
        m_YOffset += static_cast<float>(m_LastY - ypos);
        m_LastX = xpos;
        m_LastY = ypos;
    }
}

void MouseManager::Update()
{
    if (m_Mode == CursorMode::Locked && m_Window)
    {
        m_Window->SetCursorPos(m_LockX, m_LockY);
    }
    // Disabled: cursor visible, no position lock, data simply not used by game
}

void MouseManager::SetWindowSize(int width, int height)
{
    m_WindowWidth = width;
    m_WindowHeight = height;
}

void MouseManager::UpdateScroll(double xoffset, double yoffset)
{
    m_ScrollY = static_cast<float>(yoffset);
}

void MouseManager::UpdateButton(Mouse button, int action, int mods)
{
    size_t btnIdx = (size_t)button;
    if (btnIdx >= 16) return;

    if (action == 1)
    {
        m_CurrentButtons.set(btnIdx, true);
    }
    else if (action == 0)
    {
        m_CurrentButtons.set(btnIdx, false);
    }
}

void MouseManager::EndFrame()
{
    m_XOffset = 0.0f;
    m_YOffset = 0.0f;
    m_ScrollY = 0.0f;

    m_PreviousButtons = m_CurrentButtons;
}

void MouseManager::SetCursorMode(CursorMode mode)
{
    if (!m_Window)
        return;

    if (m_ForceFree && (mode == CursorMode::Locked || mode == CursorMode::LockedHidden))
    {
#ifdef ENABLE_EDITOR
        mode = CursorMode::Disabled;
#else
        mode = CursorMode::Normal;
#endif
    }

    if (mode == CursorMode::Locked && m_Mode != CursorMode::Locked)
    {
        m_LockX = m_LastX;
        m_LockY = m_LastY;
    }

#ifdef ENABLE_EDITOR
    if (mode == CursorMode::Disabled) {
        // Disabled = cursor visible, normal movement, but game ignores mouse data
        // Treat like Normal from window perspective
        m_Window->SetCursorMode(CursorMode::Normal);
        m_Mode = CursorMode::Disabled;
        return;
    }
#endif

    m_Window->SetCursorMode(mode);
    m_Mode = mode;

    if (mode == CursorMode::Locked || mode == CursorMode::LockedHidden || mode == CursorMode::Disabled)
        m_FirstMouse = true;
}

CursorMode MouseManager::GetCursorMode() const
{
    return m_Mode;
}

float MouseManager::GetXOffset() const
{
    return m_XOffset;
}

float MouseManager::GetYOffset() const
{
    return m_YOffset;
}

float MouseManager::GetScrollY() const
{
    return m_ScrollY;
}

float MouseManager::GetLastX() const
{
    if (m_Mode == CursorMode::Locked)
        return static_cast<float>(m_LockX);

    if (m_Mode == CursorMode::LockedHidden)
    {
        return static_cast<float>(m_WindowWidth) / 2.0f;
    }
    return static_cast<float>(m_LastX);
}

float MouseManager::GetLastY() const
{
    if (m_Mode == CursorMode::Locked)
        return static_cast<float>(m_LockY);

    if (m_Mode == CursorMode::LockedHidden)
    {
        return static_cast<float>(m_WindowHeight) / 2.0f;
    }
    return static_cast<float>(m_LastY);
}

bool MouseManager::IsWheelUp() const
{
    return m_ScrollY > 0.0f;
}

bool MouseManager::IsWheelDown() const
{
    return m_ScrollY < 0.0f;
}

void MouseManager::SetLastPosition(double x, double y)
{
    m_LastX = x;
    m_LastY = y;
    m_FirstMouse = false;
}
