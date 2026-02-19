#pragma once

#include <scene/scene.h>

class SoundPlayer;
#include <audio/sound_player.h>

class AudioSystem
{
public:
    void Update(Scene &scene, SoundPlayer &soundPlayer);
    void StopAll(Scene &scene);
    void SetEnabled(bool enable) { m_Enabled = enable; }
    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = true;
};
