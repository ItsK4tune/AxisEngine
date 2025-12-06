#include "engine/core/mouse_manager.h"

MouseManager::MouseManager()
    : m_LastX(400.0), m_LastY(300.0),
      m_XOffset(0.0f), m_YOffset(0.0f), m_ScrollY(0.0f),
      m_FirstMouse(true)
{
}

void MouseManager::UpdatePosition(double xpos, double ypos) {
    if (m_FirstMouse) {
        m_LastX = xpos;
        m_LastY = ypos;
        m_FirstMouse = false;
    }

    m_XOffset = static_cast<float>(xpos - m_LastX);
    m_YOffset = static_cast<float>(m_LastY - ypos);

    m_LastX = xpos;
    m_LastY = ypos;
}

void MouseManager::UpdateScroll(double xoffset, double yoffset) {
    m_ScrollY = static_cast<float>(yoffset);
}

void MouseManager::EndFrame() {
    m_XOffset = 0.0f;
    m_YOffset = 0.0f;
    m_ScrollY = 0.0f;
}

float MouseManager::GetXOffset() const {
    return m_XOffset;
}

float MouseManager::GetYOffset() const {
    return m_YOffset;
}

float MouseManager::GetScrollY() const {
    return m_ScrollY;
}

void MouseManager::SetLastPosition(double x, double y) {
    m_LastX = x;
    m_LastY = y;
    m_FirstMouse = true;
}
