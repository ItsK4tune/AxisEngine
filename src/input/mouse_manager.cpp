#include "engine/input/mouse_manager.h"

MouseManager::MouseManager(GLFWwindow *window)
    : m_Window(window),
      m_LastX(400.0), m_LastY(300.0),
      m_XOffset(0.0f), m_YOffset(0.0f), m_ScrollY(0.0f),
      m_FirstMouse(true),
      m_LeftButtonPressed(false),
      m_RightButtonPressed(false),
      m_LeftMouseClicked(false),
      m_RightMouseClicked(false),
      m_Mode(CursorMode::Normal)

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

    m_XOffset += static_cast<float>(xpos - m_LastX);
    m_YOffset += static_cast<float>(m_LastY - ypos);

    m_LastX = xpos;
    m_LastY = ypos;
}

void MouseManager::UpdateScroll(double xoffset, double yoffset)
{
    m_ScrollY = static_cast<float>(yoffset);
}

void MouseManager::UpdateButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            m_LeftButtonPressed = true;
            m_LeftMouseClicked = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_LeftButtonPressed = false;
        }
    }

    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            m_RightButtonPressed = true;
            m_RightMouseClicked = true;
        }
        else if (action == GLFW_RELEASE)
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

void MouseManager::SetCursorMode(CursorMode mode)
{
    if (!m_Window)
        return;

    switch (mode)
    {
    case CursorMode::Normal:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        break;
    case CursorMode::Hidden:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        break;
    case CursorMode::Locked:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    case CursorMode::LockedCenter:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        break;
    }

    m_Mode = mode;

    if (mode == CursorMode::Locked)
    {
        m_FirstMouse = true;
    }
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
    return static_cast<float>(m_LastX);
}

float MouseManager::GetLastY() const
{
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
