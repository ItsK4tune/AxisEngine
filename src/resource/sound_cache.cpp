#include <resource/sound_cache.h>
#include <utils/logger.h>
#include <utils/filesystem.h>
#include <iostream>

SoundCache::SoundCache()
    : m_SoundEngine(nullptr)
{
}

SoundCache::~SoundCache()
{
    Clear();
}

void SoundCache::LoadSound(const std::string& name, const std::string& path, IAudioEngine* engine)
{
    if (!engine)
    {
        LOGGER_ERROR("SoundCache") << "Sound engine is null";
        return;
    }
    
    m_SoundEngine = engine;
    std::string fullPath = FileSystem::getPath(path);
    
    std::shared_ptr<IAudioSource> source = engine->AddSoundSourceFromFile(fullPath);
    
    if (source)
    {
        m_Sounds[name] = source;
        LOGGER_INFO("SoundCache") << "Loaded sound: " << name;
    }
    else
    {
        LOGGER_ERROR("SoundCache") << "Failed to load sound: " << fullPath;
    }
}

std::shared_ptr<IAudioSource> SoundCache::GetSound(const std::string& name)
{
    if (m_Sounds.find(name) != m_Sounds.end())
        return m_Sounds[name];
    
    LOGGER_WARN("SoundCache") << "Sound not found: " << name;
    return nullptr;
}

void SoundCache::Clear()
{
    m_Sounds.clear();
}
