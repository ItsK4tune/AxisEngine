#pragma once

#include <scene/scene.h>

class AnimationSystem
{
public:
    void Update(Scene &scene, float dt);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
