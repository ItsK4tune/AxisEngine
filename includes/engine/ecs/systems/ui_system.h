#pragma once

#include <scene/scene.h>
#include <input/mouse_manager.h>

class UIInteractSystem
{
public:
    void Update(Scene &scene, float dt, const MouseManager &mouse);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};

class UIRenderSystem
{
public:
    void Render(Scene &scene, float screenWidth, float screenHeight);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
