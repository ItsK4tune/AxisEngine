#pragma once

#include <scene/scene.h>

class Application;

class ScriptableSystem
{
public:
    void Update(Scene &scene, float dt, float unscaledDt, Application *app);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
