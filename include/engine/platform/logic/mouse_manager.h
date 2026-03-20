#pragma once

#include <platform/interface/i_window.h>
#include <platform/interface/mouse.h>
#include <bitset>

class MouseManager
{
public:
    MouseManager(IWindow *window);

    void UpdatePosition(double xpos, double ypos);
    void UpdateScroll(double xoffset, double yoffset);
    void UpdateButton(Mouse button, int action, int mods);
    void Update();
    void EndFrame();

    void SetCursorMode(CursorMode mode);
    CursorMode GetCursorMode() const;

    float GetXOffset() const;
    float GetYOffset() const;
    float GetScrollY() const;

    float GetLastX() const;
    float GetLastY() const;

    // Streamlined getters
    bool IsButtonPressed(Mouse button) const  { return m_CurrentButtons.test((size_t)button); }
    bool IsMouseClicked(Mouse button) const   { return m_CurrentButtons.test((size_t)button) && !m_PreviousButtons.test((size_t)button); }
    bool IsMouseReleased(Mouse button) const  { return !m_CurrentButtons.test((size_t)button) && m_PreviousButtons.test((size_t)button); }

    // Legacy wrappers for compatibility
    bool IsLeftButtonPressed() const { return IsButtonPressed(Mouse::Left); }
    bool IsRightButtonPressed() const { return IsButtonPressed(Mouse::Right); }
    bool IsMiddleButtonPressed() const { return IsButtonPressed(Mouse::Middle); }
    bool IsLeftMouseClicked() const { return IsMouseClicked(Mouse::Left); }
    bool IsRightMouseClicked() const { return IsMouseClicked(Mouse::Right); }
    bool IsMiddleMouseClicked() const { return IsMouseClicked(Mouse::Middle); }
    bool IsLeftMouseReleased() const { return IsMouseReleased(Mouse::Left); }
    bool IsRightMouseReleased() const { return IsMouseReleased(Mouse::Right); }
    bool IsMiddleMouseReleased() const { return IsMouseReleased(Mouse::Middle); }

    bool IsMouse4ButtonPressed() const { return IsButtonPressed(Mouse::Button4); }
    bool IsMouse4MouseClicked() const { return IsMouseClicked(Mouse::Button4); }
    bool IsMouse4MouseReleased() const { return IsMouseReleased(Mouse::Button4); }
    bool IsMouse5ButtonPressed() const { return IsButtonPressed(Mouse::Button5); }
    bool IsMouse5MouseClicked() const { return IsMouseClicked(Mouse::Button5); }
    bool IsMouse5MouseReleased() const { return IsMouseReleased(Mouse::Button5); }

    bool IsWheelUp() const;
    bool IsWheelDown() const;

    void SetWindowSize(int width, int height);
    void SetLastPosition(double x, double y);
    void SetLockPosition(double x, double y) { m_LockX = x; m_LockY = y; }

private:
    friend class IOHandler;
    void SetWindow(IWindow *window);

    IWindow *m_Window = nullptr;
    int m_WindowWidth = 800;
    int m_WindowHeight = 600;

    double m_LastX = 0;
    double m_LastY = 0;

    double m_LockX = 0;
    double m_LockY = 0;

    float m_XOffset = 0;
    float m_YOffset = 0;
    float m_ScrollY = 0;

    bool m_FirstMouse = true;
    CursorMode m_Mode;

    std::bitset<16> m_CurrentButtons;
    std::bitset<16> m_PreviousButtons;
};
