#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <systems/audio/interfaces/i_audio_engine.h>
#include <systems/audio/interfaces/i_audio_source.h>
#include <systems/audio/interfaces/i_sound.h>
#include <memory>
#include <string>

class SoundPlayer
{
public:
    SoundPlayer(IAudioEngine* engine);
    ~SoundPlayer();

    std::shared_ptr<ISound> Play2D(const std::string& path, bool loop = false);
    std::shared_ptr<ISound> Play3D(const std::string& path, glm::vec3 pos, bool loop = false);

    void UpdateListener(glm::vec3 position, glm::vec3 lookDir);

    void SetGlobalVolume(float volume);

    std::shared_ptr<ISound> Play2D(std::shared_ptr<IAudioSource> source, bool loop = false);

    void StopAll();

    IAudioEngine* GetEngine() const { return m_Engine; }

private:
    IAudioEngine* m_Engine = nullptr;
};
