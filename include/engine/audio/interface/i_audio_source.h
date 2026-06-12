#pragma once

#include <audio/interface/i_audio_stream.h>
#include <string>

class IAudioSource : public IAudioStream
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

    virtual std::string GetName() const override = 0;
    virtual bool IsStreaming() const override
    {
        return false;
    }
    virtual uint64_t GetLengthMs() const override
    {
        return 0;
    }
};
