#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum class CursorMode
{
    Normal,
    Hidden,
    Locked,
    LockedCenter
};

class MouseManager
{
public:
    MouseManager(GLFWwindow *window);

    void UpdatePosition(double xpos, double ypos);
    void UpdateScroll(double xoffset, double yoffset);
    void UpdateButton(int button, int action, int mods);
    void EndFrame();

    void SetCursorMode(CursorMode mode);
    CursorMode GetCursorMode() const;

    float GetXOffset() const;
    float GetYOffset() const;
    float GetScrollY() const;

    float GetLastX() const;
    float GetLastY() const;
    bool IsLeftButtonPressed() const;

    void SetLastPosition(double x, double y);

private:
    GLFWwindow *m_Window = nullptr;

    double m_LastX;
    double m_LastY;

    float m_XOffset;
    float m_YOffset;
    float m_ScrollY;

    bool m_FirstMouse;
    bool m_LeftButtonPressed;

    CursorMode m_Mode;
};
