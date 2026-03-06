#pragma once

#include <audio/interfaces/i_audio_engine.h>
#include <audio/interfaces/i_audio_source.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>

class SoundCache
{
public:
    SoundCache();
    ~SoundCache();

    void LoadSound(const std::string& name, const std::string& path, IAudioEngine* engine);
    std::shared_ptr<IAudioSource> GetSound(const std::string& name);
    void Remove(const std::string& name);
    void Clear();

private:
    std::map<std::string, std::shared_ptr<IAudioSource>> m_Sounds;
    mutable std::mutex m_Mutex;
    IAudioEngine* m_SoundEngine;
};
