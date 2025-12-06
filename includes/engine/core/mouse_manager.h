#pragma once

class MouseManager {
public:
    MouseManager();

    void UpdatePosition(double xpos, double ypos);
    void UpdateScroll(double xoffset, double yoffset);
    void EndFrame();

    float GetXOffset() const;
    float GetYOffset() const;
    float GetScrollY() const;

    void SetLastPosition(double x, double y);

private:
    double m_LastX;
    double m_LastY;
    
    float m_XOffset;
    float m_YOffset;
    float m_ScrollY;

    bool m_FirstMouse;
};
