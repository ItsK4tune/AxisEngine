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

void SoundCache::LoadSound(const std::string& name, const std::string& path, irrklang::ISoundEngine* engine)
{
    if (!engine)
    {
        LOGGER_ERROR("SoundCache") << "Sound engine is null";
        return;
    }
    
    m_SoundEngine = engine;
    std::string fullPath = FileSystem::getPath(path);
    
    irrklang::ISoundSource* source = engine->addSoundSourceFromFile(fullPath.c_str());
    
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

irrklang::ISoundSource* SoundCache::GetSound(const std::string& name)
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
