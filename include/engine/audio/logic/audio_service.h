#pragma once

#include <audio/interface/i_audio_engine.h>
#include <audio/interface/i_audio_source.h>
#include <audio/interface/i_sound.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>

#define GLM_ENABLE_EXPERIMENTAL

/**
 * @brief High-level audio service that replaces SoundPlayer + AudioManager.
 *
 * Owns the IAudioEngine and provides convenient Play/Stop APIs.
 * Registers itself in ServiceLocator on Initialize, unregisters on Shutdown.
 */
class AudioService
{
public:
    AudioService() = default;
    ~AudioService();

    /**
     * @brief Takes ownership of the audio engine backend, initializes it,
     *        and registers this service in the ServiceLocator.
     */
    bool Initialize(std::unique_ptr<IAudioEngine> engine);
    void Shutdown();

    // --- High-Level API ---

    std::shared_ptr<ISound> Play2D(const std::string& path, bool loop = false);
    std::shared_ptr<ISound> Play3D(const std::string& path, glm::vec3 pos, bool loop = false);
    std::shared_ptr<ISound> Play2D(std::shared_ptr<IAudioSource> source, bool loop = false);

    void UpdateListener(glm::vec3 position, glm::vec3 lookDir);
    void SetGlobalVolume(float volume);
    void StopAll();

    // --- Engine Access ---

    IAudioEngine* GetEngine() const { return m_Engine.get(); }
    bool IsInitialized() const { return m_Engine != nullptr; }

private:
    std::unique_ptr<IAudioEngine> m_Engine;
};
