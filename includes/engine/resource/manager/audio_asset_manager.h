#pragma once

#include <audio/interface/i_audio_engine.h>
#include <resource/logic/resource_cache.h>
#include <string>
#include <memory>

/**
 * @brief High-level manager for Audio assets (IAudioSource).
 */
class AudioAssetManager {
public:
    AudioAssetManager(IAudioEngine& audioEngine);
    ~AudioAssetManager() = default;

    /**
     * @brief Loads a sound file as an audio source.
     */
    std::shared_ptr<IAudioSource> Load(const std::string& name, const std::string& path);

    /**
     * @brief Retrieves an audio source from cache.
     */
    std::shared_ptr<IAudioSource> Get(const std::string& name);

    /**
     * @brief Unloads an audio source from memory.
     */
    void Unload(const std::string& name);

private:
    IAudioEngine& m_AudioEngine;
    ResourceCache<IAudioSource> m_Cache;
};
