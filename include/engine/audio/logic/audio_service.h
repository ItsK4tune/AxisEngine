#pragma once

#include <audio/interface/i_audio_engine.h>
#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <audio/unit/audio_pulse.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

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
    // Publishes a one-shot world-space pulse for post-process/gameplay effects.
    // Peak defaults to intensity because gameplay events do not have a sampled
    // microphone peak unless the caller supplies one explicitly.
    void EmitPulse(const glm::vec3& origin, float intensity = 1.0f, float duration = 0.6f, float peak = -1.0f);
    void UpdatePulses(float deltaTime);
    void ClearPulses();
    const std::vector<AudioPulse>& GetPulses() const
    {
        return m_Pulses;
    }
    void SetGlobalVolume(float volume);
    void StopAll();

    IAudioEngine* GetEngine() const
    {
        return m_Engine.get();
    }
    bool IsInitialized() const
    {
        return m_Engine != nullptr;
    }

private:
    std::unique_ptr<IAudioEngine> m_Engine;
    std::vector<AudioPulse> m_Pulses;
};
