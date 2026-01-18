#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

enum class CursorMode
{
    Normal,
    Hidden,
    LockedCenter,
    LockedHiddenCenter
};

class MouseManager
{
public:
    MouseManager(GLFWwindow *window);

    void UpdatePosition(double xpos, double ypos);
    void UpdateScroll(double xoffset, double yoffset);
    void UpdateButton(int button, int action, int mods);
    void Update();
    void EndFrame();

    void SetCursorMode(CursorMode mode);
    CursorMode GetCursorMode() const;

    float GetXOffset() const;
    float GetYOffset() const;
    float GetScrollY() const;

    float GetLastX() const;
    float GetLastY() const;
    
    bool IsLeftButtonPressed() const;
    bool IsRightButtonPressed() const;

    bool IsLeftMouseClicked() const;
    bool IsRightMouseClicked() const;

    void SetWindowSize(int width, int height);
    void SetLastPosition(double x, double y);

private:
    GLFWwindow *m_Window = nullptr;
    int m_WindowWidth = 800;
    int m_WindowHeight = 600;

    double m_LastX;
    double m_LastY;

    float m_XOffset;
    float m_YOffset;
    float m_ScrollY;

    bool m_FirstMouse;

    bool m_LeftButtonPressed;
    bool m_RightButtonPressed;

    bool m_LeftMouseClicked;
    bool m_RightMouseClicked;

    CursorMode m_Mode;
};
