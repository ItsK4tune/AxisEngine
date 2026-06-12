#pragma once

#include <audio/interface/i_audio_device.h>

class IAudioEngine : public IAudioDevice
{
public:
    ~IAudioEngine() override = default;
};
