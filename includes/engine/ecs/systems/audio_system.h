#pragma once

#include <scene/scene.h>
#include <audio/sound_manager.h>

class AudioSystem
{
public:
    void Update(Scene &scene, SoundManager &soundManager);
    void StopAll(Scene &scene);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
