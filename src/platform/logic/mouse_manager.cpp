#include <platform/logic/input_system.h>

MouseManager::MouseManager(IWindow *window)
    : m_LastX(400.0), m_LastY(300.0),
      m_XOffset(0.0f), m_YOffset(0.0f), m_ScrollY(0.0f),
      m_FirstMouse(true),
      m_LeftButtonPressed(false),
      m_RightButtonPressed(false),
      m_MiddleButtonPressed(false),
      m_Mouse4ButtonPressed(false),
      m_Mouse5ButtonPressed(false),
      m_LeftMouseClicked(false),
      m_RightMouseClicked(false),
      m_MiddleMouseClicked(false),
      m_Mouse4MouseClicked(false),
      m_Mouse5MouseClicked(false),
      m_LeftMouseReleased(false),
      m_RightMouseReleased(false),
      m_MiddleMouseReleased(false),
      m_Mouse4MouseReleased(false),
      m_Mouse5MouseReleased(false),
      m_Mode(CursorMode::Normal)
{
    SetWindow(window);
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
    if (button == Mouse::Left)
    {
        if (action == 1)
        {
            m_LeftButtonPressed = true;
            m_LeftMouseClicked = true;
        }
        else if (action == 0)
        {
            m_LeftButtonPressed = false;
            m_LeftMouseReleased = true;
        }
    }

    else if (button == Mouse::Right)
    {
        if (action == 1)
        {
            m_RightButtonPressed = true;
            m_RightMouseClicked = true;
        }
        else if (action == 0)
        {
            m_RightButtonPressed = false;
            m_RightMouseReleased = true;
        }
    }
    else if (button == Mouse::Middle)
    {
        if (action == 1)
        {
            m_MiddleButtonPressed = true;
            m_MiddleMouseClicked = true;
        }
        else if (action == 0)
        {
            m_MiddleButtonPressed = false;
            m_MiddleMouseReleased = true;
        }
    }
    else if (button == Mouse::Button4)
    {
        if (action == 1)
        {
            m_Mouse4ButtonPressed = true;
            m_Mouse4MouseClicked = true;
        }
        else if (action == 0)
        {
            m_Mouse4ButtonPressed = false;
            m_Mouse4MouseReleased = true;
        }
    }
    else if (button == Mouse::Button5)
    {
        if (action == 1)
        {
            m_Mouse5ButtonPressed = true;
            m_Mouse5MouseClicked = true;
        }
        else if (action == 0)
        {
            m_Mouse5ButtonPressed = false;
            m_Mouse5MouseReleased = true;
        }
    }
}

void MouseManager::EndFrame()
{
    m_XOffset = 0.0f;
    m_YOffset = 0.0f;
    m_ScrollY = 0.0f;

    m_LeftMouseClicked = false;
    m_RightMouseClicked = false;
    m_MiddleMouseClicked = false;
    m_Mouse4MouseClicked = false;
    m_Mouse5MouseClicked = false;

    m_LeftMouseReleased = false;
    m_RightMouseReleased = false;
    m_MiddleMouseReleased = false;
    m_Mouse4MouseReleased = false;
    m_Mouse5MouseReleased = false;
}

void MouseManager::SetCursorMode(CursorMode mode)
{
    if (!m_Window)
        return;

    if (mode == CursorMode::Locked && m_Mode != CursorMode::Locked)
    {
        m_LockX = m_LastX;
        m_LockY = m_LastY;
    }

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

bool MouseManager::IsLeftButtonPressed() const
{
    return m_LeftButtonPressed;
}

bool MouseManager::IsLeftMouseClicked() const
{
    return m_LeftMouseClicked;
}

bool MouseManager::IsRightButtonPressed() const
{
    return m_RightButtonPressed;
}

bool MouseManager::IsRightMouseClicked() const
{
    return m_RightMouseClicked;
}

bool MouseManager::IsLeftMouseReleased() const
{
    return m_LeftMouseReleased;
}

bool MouseManager::IsRightMouseReleased() const
{
    return m_RightMouseReleased;
}

bool MouseManager::IsMiddleButtonPressed() const
{
    return m_MiddleButtonPressed;
}

bool MouseManager::IsMiddleMouseClicked() const
{
    return m_MiddleMouseClicked;
}

bool MouseManager::IsMiddleMouseReleased() const
{
    return m_MiddleMouseReleased;
}

bool MouseManager::IsMouse4ButtonPressed() const
{
    return m_Mouse4ButtonPressed;
}

bool MouseManager::IsMouse4MouseClicked() const
{
    return m_Mouse4MouseClicked;
}

bool MouseManager::IsMouse4MouseReleased() const
{
    return m_Mouse4MouseReleased;
}

bool MouseManager::IsMouse5ButtonPressed() const
{
    return m_Mouse5ButtonPressed;
}

bool MouseManager::IsMouse5MouseClicked() const
{
    return m_Mouse5MouseClicked;
}

bool MouseManager::IsMouse5MouseReleased() const
{
    return m_Mouse5MouseReleased;
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
