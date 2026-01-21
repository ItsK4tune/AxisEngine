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

void MouseManager::Update()
{
    if (m_Mode == CursorMode::LockedCenter)
    {
        if (!m_Window)
            return;

        if (glfwGetWindowAttrib(m_Window, GLFW_FOCUSED))
        {
            int w = m_WindowWidth;
            int h = m_WindowHeight;
            double centerX = w / 2.0;
            double centerY = h / 2.0;

            glfwSetCursorPos(m_Window, centerX, centerY);

            m_LastX = centerX;
            m_LastY = centerY;
        }
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
    case CursorMode::LockedCenter:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_FirstMouse = true;
        break;
    case CursorMode::LockedHiddenCenter:
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        m_FirstMouse = true;
        break;
    }

    m_Mode = mode;
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
    if (m_Mode == CursorMode::LockedCenter || m_Mode == CursorMode::LockedHiddenCenter)
    {
        int w, h;
        glfwGetWindowSize(m_Window, &w, &h);
        return static_cast<float>(w) / 2.0f;
    }
    return static_cast<float>(m_LastX);
}

float MouseManager::GetLastY() const
{
    if (m_Mode == CursorMode::LockedCenter || m_Mode == CursorMode::LockedHiddenCenter)
    {
        int w, h;
        glfwGetWindowSize(m_Window, &w, &h);
        return static_cast<float>(h) / 2.0f;
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
