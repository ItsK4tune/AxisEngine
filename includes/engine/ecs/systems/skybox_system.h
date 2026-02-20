#pragma once

#include <scene/scene.h>
class IGraphicsContext;

class SkyboxRenderSystem
{
public:
    void Init(IGraphicsContext& context);
    void Render(Scene &scene);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    IGraphicsContext* m_Context = nullptr;
    bool m_Enabled = true;
};
