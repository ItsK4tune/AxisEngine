#pragma once

#include <audio/interface/i_audio_engine.h>
#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

#define GLM_ENABLE_EXPERIMENTAL

class AudioService
{
public:
    AudioService() = default;
    ~AudioService();

    bool Initialize(std::unique_ptr<IAudioEngine> engine);
    void Shutdown();

    std::shared_ptr<ISound> Play2D(const std::string& path, bool loop = false);
    std::shared_ptr<ISound> Play3D(const std::string& path, glm::vec3 pos, bool loop = false);
    std::shared_ptr<ISound> Play2D(std::shared_ptr<IAudioSource> source, bool loop = false);

    void UpdateListener(glm::vec3 position, glm::vec3 lookDir);
    void SetGlobalVolume(float volume);
    void StopAll();

    IAudioEngine* GetEngine() const
    {
        return m_Engine.get();
    }
    IAudioDevice* GetDevice() const
    {
        return m_Engine.get();
    }
    bool IsInitialized() const
    {
        return m_Engine != nullptr;
    }

private:
    std::unique_ptr<IAudioEngine> m_Engine;
};
