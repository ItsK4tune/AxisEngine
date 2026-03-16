#pragma once

#include <string>

class IAudioSource
{
public:
    virtual ~IAudioSource() = default;

    virtual void SetDefaultVolume(float volume) = 0;
    virtual float GetDefaultVolume() const = 0;
    
    virtual void SetDefaultPitch(float pitch) = 0;
    virtual float GetDefaultPitch() const = 0;

    virtual void SetDefaultPan(float pan) = 0;
    virtual float GetDefaultPan() const = 0;

    virtual void SetDefaultMinDistance(float minDist) = 0;
    virtual float GetDefaultMinDistance() const = 0;

    virtual void SetDefaultMaxDistance(float maxDist) = 0;
    virtual float GetDefaultMaxDistance() const = 0;

    virtual void SetDefaultLoop(bool loop) = 0;
    virtual bool GetDefaultLoop() const = 0;

    virtual std::string GetName() const = 0;
};