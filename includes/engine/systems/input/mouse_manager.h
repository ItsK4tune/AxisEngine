#pragma once

#include <systems/window/interfaces/i_window.h>
#include <systems/window/interfaces/input_codes.h>

class MouseManager
{
public:
    MouseManager(IWindow *window);

    void UpdatePosition(double xpos, double ypos);
    void UpdateScroll(double xoffset, double yoffset);
    void UpdateButton(Input::Mouse button, int action, int mods);
    void Update();
    void EndFrame();

    void SetCursorMode(Input::CursorMode mode);
    Input::CursorMode GetCursorMode() const;

    float GetXOffset() const;
    float GetYOffset() const;
    float GetScrollY() const;

    float GetLastX() const;
    float GetLastY() const;

    bool IsLeftButtonPressed() const;
    bool IsRightButtonPressed() const;
    bool IsMiddleButtonPressed() const;

    bool IsLeftMouseClicked() const;
    bool IsRightMouseClicked() const;
    bool IsMiddleMouseClicked() const;

    bool IsLeftMouseReleased() const;
    bool IsRightMouseReleased() const;
    bool IsMiddleMouseReleased() const;

    bool IsMouse4ButtonPressed() const;
    bool IsMouse4MouseClicked() const;
    bool IsMouse4MouseReleased() const;

    bool IsMouse5ButtonPressed() const;
    bool IsMouse5MouseClicked() const;
    bool IsMouse5MouseReleased() const;

    bool IsWheelUp() const;
    bool IsWheelDown() const;

    void SetWindowSize(int width, int height);
    void SetLastPosition(double x, double y);
    void SetLockPosition(double x, double y) { m_LockX = x; m_LockY = y; }

private:
    friend class IOHandler;
    void SetWindow(IWindow* window);

    IWindow *m_Window = nullptr;
    int m_WindowWidth = 800;
    int m_WindowHeight = 600;

    double m_LastX;
    double m_LastY;

    double m_LockX;
    double m_LockY;

    float m_XOffset;
    float m_YOffset;
    float m_ScrollY;

    bool m_FirstMouse;

    bool m_LeftButtonPressed;
    bool m_RightButtonPressed;
    bool m_MiddleButtonPressed;

    bool m_LeftMouseClicked;
    bool m_RightMouseClicked;
    bool m_MiddleMouseClicked;

    bool m_LeftMouseReleased;
    bool m_RightMouseReleased;
    bool m_MiddleMouseReleased;

    bool m_Mouse4ButtonPressed;
    bool m_Mouse5ButtonPressed;

    bool m_Mouse4MouseClicked;
    bool m_Mouse5MouseClicked;

    bool m_Mouse4MouseReleased;
    bool m_Mouse5MouseReleased;

    Input::CursorMode m_Mode;
};
