#pragma once

#include <scene/scene.h>
#include <resource/resource_manager.h>

class IGraphicsContext;

class ParticleSystem
{
public:
    void Init(IGraphicsContext& context);
    void Update(Scene &scene, float dt);
    void Render(Scene &scene, ResourceManager &res);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    IGraphicsContext* m_Context = nullptr;
    bool m_Enabled = true;
};
