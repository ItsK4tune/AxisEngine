#pragma once

#include <scene/scene.h>

class SkyboxRenderSystem
{
public:
    void Render(Scene &scene);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
