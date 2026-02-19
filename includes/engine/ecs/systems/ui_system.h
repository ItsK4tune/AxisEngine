#pragma once

#include <scene/scene.h>
#include <input/mouse_manager.h>

#include <interface/graphic/i_render_state_manager.h>

class UIRenderSystem
{
public:
    void Render(Scene &scene, float screenWidth, float screenHeight, IRenderStateManager& renderState);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
