#pragma once

#include <memory>
#include <string>
#include <interface/audio/i_sound.h>
#include <interface/audio/i_audio_source.h>

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
