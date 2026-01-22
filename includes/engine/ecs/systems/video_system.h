#pragma once

#include <scene/scene.h>
#include <resource/resource_manager.h>

class VideoSystem
{
public:
    void Update(Scene &scene, ResourceManager &res, float dt);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
