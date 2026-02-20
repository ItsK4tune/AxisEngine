#pragma once

#include <interface/window/i_window.h>
#include <interface/window/input_codes.h>

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

    bool IsLeftMouseClicked() const;
    bool IsRightMouseClicked() const;

    void SetWindow(IWindow* window) { m_Window = window; }
    void SetWindowSize(int width, int height);
    void SetLastPosition(double x, double y);
    void SetLockPosition(double x, double y) { m_LockX = x; m_LockY = y; }

private:
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

    bool m_LeftMouseClicked;
    bool m_RightMouseClicked;

    Input::CursorMode m_Mode;
};
