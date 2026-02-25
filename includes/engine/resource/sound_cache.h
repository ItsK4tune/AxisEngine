#pragma once

#include <interface/audio/i_audio_engine.h>
#include <interface/audio/i_audio_source.h>
#include <string>
#include <map>
#include <memory>

class SoundCache
{
public:
    SoundCache();
    ~SoundCache();

    void LoadSound(const std::string& name, const std::string& path, IAudioEngine* engine);
    std::shared_ptr<IAudioSource> GetSound(const std::string& name);
    void Remove(const std::string& name) { m_Sounds.erase(name); }
    void Clear();

private:
    std::map<std::string, std::shared_ptr<IAudioSource>> m_Sounds;
    IAudioEngine* m_SoundEngine;
};
