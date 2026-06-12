#pragma once

#include <string>

class IAudioEvent
{
public:
    virtual ~IAudioEvent() = default;

    virtual std::string GetName() const = 0;
    virtual void Play() = 0;
    virtual void Stop() = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual bool IsPlaying() const = 0;
    virtual void SetVolume(float volume) = 0;
    virtual float GetVolume() const = 0;
};
