#pragma once

#include <audio/interface/i_audio_channel.h>
#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

class ISound : public IAudioChannel
{
public:
    virtual ~ISound() = default;
};
