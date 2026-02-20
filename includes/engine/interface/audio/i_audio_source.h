#pragma once

#include <string>

class IAudioSource
{
public:
    virtual ~IAudioSource() = default;

    virtual void SetDefaultVolume(float volume) = 0;
    virtual void SetDefaultMinDistance(float minDist) = 0;
    virtual void SetDefaultMaxDistance(float maxDist) = 0;

    virtual std::string GetName() const = 0;
};
