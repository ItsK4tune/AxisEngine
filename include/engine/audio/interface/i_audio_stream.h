#pragma once

#include <cstdint>
#include <string>

class IAudioStream
{
public:
    virtual ~IAudioStream() = default;

    virtual std::string GetName() const = 0;
    virtual bool IsStreaming() const
    {
        return true;
    }
    virtual uint64_t GetLengthMs() const
    {
        return 0;
    }
};
