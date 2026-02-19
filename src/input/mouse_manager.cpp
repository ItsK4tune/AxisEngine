#include "engine/input/mouse_manager.h"

MouseManager::MouseManager(IWindow *window)
    : m_Window(window),
      m_LastX(400.0), m_LastY(300.0),
      m_XOffset(0.0f), m_YOffset(0.0f), m_ScrollY(0.0f),
      m_FirstMouse(true),
      m_LeftButtonPressed(false),
      m_RightButtonPressed(false),
      m_LeftMouseClicked(false),
      m_RightMouseClicked(false),
      m_Mode(Input::CursorMode::Normal)

{
}

void MouseManager::UpdatePosition(double xpos, double ypos)
{
    if (m_FirstMouse)
    {
        m_LastX = xpos;
        m_LastY = ypos;
        m_FirstMouse = false;
    }

    if (m_Mode == Input::CursorMode::Locked)
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
    if (m_Mode == Input::CursorMode::Locked && m_Window)
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

void MouseManager::UpdateButton(Input::Mouse button, int action, int mods)
{
    if (button == Input::Mouse::Left)
    {
        if (action == 1) 
        {
            m_LeftButtonPressed = true;
            m_LeftMouseClicked = true;
        }
        else if (action == 0) 
        {
            m_LeftButtonPressed = false;
        }
    }

    else if (button == Input::Mouse::Right)
    {
        if (action == 1) 
        {
            m_RightButtonPressed = true;
            m_RightMouseClicked = true;
        }
        else if (action == 0) 
        {
            m_RightButtonPressed = false;
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
}

void MouseManager::SetCursorMode(Input::CursorMode mode)
{
    if (!m_Window)
        return;

    if (mode == Input::CursorMode::Locked && m_Mode != Input::CursorMode::Locked)
    {
        m_LockX = m_LastX;
        m_LockY = m_LastY;
    }

    m_Window->SetCursorMode(mode);
    m_Mode = mode;
    
    if (mode == Input::CursorMode::Locked || mode == Input::CursorMode::LockedHidden || mode == Input::CursorMode::Disabled) 
        m_FirstMouse = true;
}

Input::CursorMode MouseManager::GetCursorMode() const
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
    if (m_Mode == Input::CursorMode::Locked)
        return static_cast<float>(m_LockX);

    if (m_Mode == Input::CursorMode::LockedHidden)
    {
        return static_cast<float>(m_WindowWidth) / 2.0f;
    }
    return static_cast<float>(m_LastX);
}

float MouseManager::GetLastY() const
{
    if (m_Mode == Input::CursorMode::Locked)
        return static_cast<float>(m_LockY);

    if (m_Mode == Input::CursorMode::LockedHidden)
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

void MouseManager::SetLastPosition(double x, double y)
{
    m_LastX = x;
    m_LastY = y;
    m_FirstMouse = false;
}
