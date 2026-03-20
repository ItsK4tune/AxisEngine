#include <resource/logic/audio_asset_manager.h>
#include <core/logic/logger.h>

AudioAssetManager::AudioAssetManager(IAudioEngine& audioEngine) 
    : m_AudioEngine(audioEngine) {}

std::shared_ptr<IAudioSource> AudioAssetManager::Load(const std::string& name, const std::string& path) {
    if (auto existing = m_Cache.Get(name)) return existing;

    auto source = m_AudioEngine.AddSoundSourceFromFile(path);
    if (source) {
        m_Cache.Add(name, source);
        LOGGER_INFO("AudioManager") << "Loaded sound: " << name;
        return source;
    }
    
    LOGGER_ERROR("AudioManager") << "Failed to load sound: " << path;
    return nullptr;
}

std::shared_ptr<IAudioSource> AudioAssetManager::Get(const std::string& nameOrPath) {
    return m_Cache.Get(nameOrPath);
}

void AudioAssetManager::Unload(const std::string& nameOrPath) {
    m_Cache.Remove(nameOrPath);
}

void AudioAssetManager::Clear() {
    m_Cache.Clear();
}
