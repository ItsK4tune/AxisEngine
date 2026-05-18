#pragma once

#include <memory>
#include <string>

class IAudioEngine;
class IAudioSource;

class ISoundLibrary
{
public:
    virtual ~ISoundLibrary() = default;
    virtual void LoadSound(const std::string& name, const std::string& path, IAudioEngine* engine) = 0;
    virtual void UnloadSound(const std::string& name) = 0;
    virtual std::shared_ptr<IAudioSource> GetSound(const std::string& name) = 0;
};
