#pragma once

#include <irrKlang/irrKlang.h>
#include <glm/glm.hpp>
#include <iostream>

using namespace irrklang;

class SoundManager {
public:
    ~SoundManager();

    void Init();
    
    void Play2D(const char* path, bool loop = false);
    void Play2D(ISoundSource* source, bool loop = false);
    void Play3D(ISoundSource* source, glm::vec3 pos, bool loop = false);

    void UpdateListener(glm::vec3 position, glm::vec3 lookDir, glm::vec3 up);

    void StopAll();
    void SetVolume(float volume);

    ISoundEngine* GetEngine() const { return m_Engine; }

private:
    ISoundEngine* m_Engine = nullptr;
};