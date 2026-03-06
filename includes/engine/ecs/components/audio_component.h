#pragma once

#include <systems/audio/interfaces/i_audio_source.h>
#include <systems/audio/interfaces/i_sound.h>
#include <memory>
#include <string>

struct AudioSourceComponent
{
    std::shared_ptr<ISound> sound = nullptr;
    std::string filePath = "";
    bool playOnStart = false;
    bool playOnAwake = false;
    bool shouldPlay = false;
    bool loop = false;
    bool is3D = false;
    float volume = 1.0f;
    float pitch = 1.0f;
    float minDistance = 1.0f;
    float maxDistance = 100.0f;
};
