#pragma once

#include <irrKlang/irrKlang.h>
#include <string>
#include <map>

class SoundCache
{
public:
    SoundCache();
    ~SoundCache();

    void LoadSound(const std::string& name, const std::string& path, irrklang::ISoundEngine* engine);
    irrklang::ISoundSource* GetSound(const std::string& name);
    
    void Clear();

private:
    std::map<std::string, irrklang::ISoundSource*> m_Sounds;
    irrklang::ISoundEngine* m_SoundEngine;
};
