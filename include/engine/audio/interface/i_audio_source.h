#pragma once

#include <string>

class IAudioSource
{
public:
    virtual ~IAudioSource() = default;

    // Public audio volume uses a 0..100 range. Implementations map it to their native API scale.
    virtual void SetDefaultVolume(float volume) = 0;
    virtual float GetDefaultVolume() const = 0;

    virtual void SetDefaultPitch(float pitch) = 0;
    virtual float GetDefaultPitch() const = 0;

    virtual void SetDefaultPan(float pan) = 0;
    virtual float GetDefaultPan() const = 0;

    virtual void SetDefaultSpeed(float speed) = 0;
    virtual float GetDefaultSpeed() const = 0;

    virtual std::string GetName() const = 0;
};
