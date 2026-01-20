#include <resource/sound_cache.h>
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
        std::cerr << "[SoundCache] Sound engine is null" << std::endl;
        return;
    }
    
    m_SoundEngine = engine;
    std::string fullPath = FileSystem::getPath(path);
    
    irrklang::ISoundSource* source = engine->addSoundSourceFromFile(fullPath.c_str());
    
    if (source)
    {
        m_Sounds[name] = source;
        std::cout << "[SoundCache] Loaded sound: " << name << std::endl;
    }
    else
    {
        std::cerr << "[SoundCache] Failed to load sound: " << fullPath << std::endl;
    }
}

irrklang::ISoundSource* SoundCache::GetSound(const std::string& name)
{
    if (m_Sounds.find(name) != m_Sounds.end())
        return m_Sounds[name];
    
    std::cerr << "[SoundCache] Sound not found: " << name << std::endl;
    return nullptr;
}

void SoundCache::Clear()
{
    m_Sounds.clear();
}
